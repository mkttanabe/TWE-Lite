/****************************************************************************
 * (C) Tokyo Cosmos Electric, Inc. (TOCOS) - 2012 all rights reserved.
 *
 * Condition to use:
 *   - The full or part of source code is limited to use for TWE (TOCOS
 *     Wireless Engine) as compiled and flash programmed.
 *   - The full or part of source code is prohibited to distribute without
 *     permission from TOCOS.
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "jendefs.h"
#include "AppHardwareApi.h"
#include "string.h"
#include "fprintf.h"

#include "sensor_driver.h"
#include "S1105902.h"
#include "SMBus.h"

#include "ccitt8.h"

#include "utils.h"

#undef SERIAL_DEBUG
#ifdef SERIAL_DEBUG
# include <serial.h>
# include <fprintf.h>
extern tsFILE sDebugStream;
#endif
tsFILE sSerStream;

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define S1105902_ADDRESS	(0x2A)

#define S1105902_CTRL		(0x00)
#define S1105902_TIMING		(0x01)

#define S1105902_R			(0x03)
#define S1105902_G			(0x05)
#define S1105902_B			(0x07)
#define S1105902_I			(0x09)

#define S1105902_CONVTIME    (24+2) // 24ms MAX

#define S1105902_DATA_NOTYET  (-32768)
#define S1105902_DATA_ERROR   (-32767)

const uint8 S1105902_AXIS[] = {
		S1105902_R,
		S1105902_G,
		S1105902_B,
		S1105902_I
};

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE bool_t bGetComp( uint8 u8comp, uint8* au8data );
PRIVATE void vProcessSnsObj_S1105902(void *pvObj, teEvent eEvent);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
void vS1105902_Init(tsObjData_S1105902 *pData, tsSnsObj *pSnsObj) {
	vSnsObj_Init(pSnsObj);

	pSnsObj->pvData = (void*)pData;
	pSnsObj->pvProcessSnsObj = (void*)vProcessSnsObj_S1105902;

	memset((void*)pData, 0, sizeof(tsObjData_S1105902));
}

void vS1105902_Final(tsObjData_S1105902 *pData, tsSnsObj *pSnsObj) {
	pSnsObj->u8State = E_SNSOBJ_STATE_INACTIVE;
}



/****************************************************************************
 *
 * NAME: bS1105902reset
 *
 * DESCRIPTION:
 *   to reset S1105902 device
 *
 * RETURNS:
 * bool_t	fail or success
 *
 ****************************************************************************/
PUBLIC bool_t bS1105902reset()
{
	return TRUE;
}

/****************************************************************************
 *
 * NAME: vHTSstartReadTemp
 *
 * DESCRIPTION:
 * Wrapper to start a read of the temperature sensor.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC bool_t bS1105902startRead()
{
	bool_t bOk = TRUE;
	uint8 u8com;

	// Set Option
//	u8com = 0x00;	// ADC起動、スリープ解除、Lowゲイン、固定時間、87.5us
//	u8com = 0x08;	// ADC起動、スリープ解除、Highゲイン、固定時間、87.5us
//	u8com = 0x01;	// ADC起動、スリープ解除、Lowゲイン、固定時間、1.4ms
	u8com = 0x09;	// ADC起動、スリープ解除、Highゲイン、固定時間、1.4ms
//	u8com = 0x03;	// ADC起動、スリープ解除、Lowゲイン、固定時間、22.4ms
//	u8com = 0x0B;	// ADC起動、スリープ解除、Highゲイン、固定時間、22.4ms
	bOk &= bSMBusWrite( S1105902_ADDRESS, S1105902_CTRL, 1, &u8com );

	return bOk;
}

/****************************************************************************
 *
 * NAME: u16S1105902readResult
 *
 * DESCRIPTION:
 * Wrapper to read a measurement, followed by a conversion function to work
 * out the value in degrees Celcius.
 *
 * RETURNS:
 * int16: 0~10000 [1 := 5Lux], 100 means 500 Lux.
 *        0x8000, error
 *
 * NOTES:
 * the data conversion fomula is :
 *      ReadValue / 1.2 [LUX]
 *
 ****************************************************************************/
PUBLIC uint16 u16S1105902readResult( uint8 u8comp )
{
	bool_t	bOk = TRUE;
	uint16	u16result=0;
	uint8	au8data[2];

	//	各成分の読み込み
	switch( u8comp ){
		case S1105902_IDX_R:	//	赤
			bOk &= bGetComp( S1105902_IDX_R, au8data );
			break;
		case S1105902_IDX_G:	//	緑
			bOk &= bGetComp( S1105902_IDX_G, au8data );
			break;
		case S1105902_IDX_B:	//	青
			bOk &= bGetComp( S1105902_IDX_B, au8data );
			break;
		case S1105902_IDX_I:	//	赤外線
			bOk &= bGetComp( S1105902_IDX_I, au8data );
			break;
		default:
			bOk = FALSE;
	}
	u16result = (au8data[0] << 8) | au8data[1];

	if (bOk == FALSE) {
		u16result = SENSOR_TAG_DATA_ERROR;
	}

    return u16result;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
PRIVATE bool_t bGetComp( uint8 u8comp, uint8* au8data )
{
	bool_t bOk = TRUE;

	bOk &= bSMBusWrite( S1105902_ADDRESS, S1105902_AXIS[u8comp], 0, NULL );
	bOk &= bSMBusSequentialRead( S1105902_ADDRESS, 2, au8data );

	return bOk;
}

// the Main loop
PRIVATE void vProcessSnsObj_S1105902(void *pvObj, teEvent eEvent) {
	tsSnsObj *pSnsObj = (tsSnsObj *)pvObj;
	tsObjData_S1105902 *pObj = (tsObjData_S1105902 *)pSnsObj->pvData;

	// general process (independent from each state)
	switch (eEvent) {
		case E_EVENT_TICK_TIMER:
			if (pObj->u8TickCount < 100) {
				pObj->u8TickCount += pSnsObj->u8TickDelta;
#ifdef SERIAL_DEBUG
vfPrintf(&sDebugStream, "+");
#endif
			}
			break;
		case E_EVENT_START_UP:
			pObj->u8TickCount = 100; // expire immediately
#ifdef SERIAL_DEBUG
vfPrintf(&sDebugStream, "\n\rS1105902 WAKEUP");
#endif
			break;
		default:
			break;
	}

	// state machine
	switch(pSnsObj->u8State)
	{
	case E_SNSOBJ_STATE_INACTIVE:
		// do nothing until E_ORDER_INITIALIZE event
		break;

	case E_SNSOBJ_STATE_IDLE:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			break;

		case E_ORDER_KICK:
			vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_MEASURING);

			#ifdef SERIAL_DEBUG
			vfPrintf(&sDebugStream, "\n\rS1105902 KICKED");
			#endif

			break;

		default:
			break;
		}
		break;

	case E_SNSOBJ_STATE_MEASURING:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			pObj->au16Result[S1105902_IDX_R] = SENSOR_TAG_DATA_ERROR;
			pObj->au16Result[S1105902_IDX_G] = SENSOR_TAG_DATA_ERROR;
			pObj->au16Result[S1105902_IDX_B] = SENSOR_TAG_DATA_ERROR;
			pObj->au16Result[S1105902_IDX_I] = SENSOR_TAG_DATA_ERROR;
			pObj->u8TickWait = S1105902_CONVTIME;

			pObj->bBusy = TRUE;
#ifdef S1105902_ALWAYS_RESET
			u8reset_flag = TRUE;
			if (!bS1105902reset()) {
				vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
			}
#else
			if (!bS1105902startRead()) { // kick I2C communication
				vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
			}
#endif
			pObj->u8TickCount = 0;
			break;

		default:
			break;
		}

		// wait until completion
		if (pObj->u8TickCount > pObj->u8TickWait) {
#ifdef S1105902_ALWAYS_RESET
			if (u8reset_flag) {
				u8reset_flag = 0;
				if (!bS1105902startRead()) {
					vS1105902_new_state(pObj, E_SNSOBJ_STATE_COMPLETE);
				}

				pObj->u8TickCount = 0;
				pObj->u8TickWait = S1105902_CONVTIME;
				break;
			}
#endif
			pObj->au16Result[S1105902_IDX_R] = u16S1105902readResult(S1105902_IDX_R);
			pObj->au16Result[S1105902_IDX_G] = u16S1105902readResult(S1105902_IDX_G);
			pObj->au16Result[S1105902_IDX_B] = u16S1105902readResult(S1105902_IDX_B);
			pObj->au16Result[S1105902_IDX_I] = u16S1105902readResult(S1105902_IDX_I);

			// data arrival
			pObj->bBusy = FALSE;
			vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
		}
		break;

	case E_SNSOBJ_STATE_COMPLETE:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			#ifdef SERIAL_DEBUG
			vfPrintf(&sDebugStream, "\n\rS1105902_CP: %d", pObj->i16Result);
			#endif

			break;

		case E_ORDER_KICK:
			// back to IDLE state
			vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_IDLE);
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
