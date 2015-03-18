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
#include "LIS3DH.h"
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
#define LIS3DH_ADDRESS		(0x18)

#define LIS3DH_OPT			(0xA7)
#define LIS3DH_SET_OPT		(0x20)

#define LIS3DH_HR			(0x08)
#define LIS3DH_SET_HR		(0x23)

#define LIS3DH_SOFT_RST		(0x24)
#define LIS3DH_SOFT_COM		(0x80)

#define LIS3DH_X			(0x28)
#define LIS3DH_Y			(0x2A)
#define LIS3DH_Z			(0x2C)

#define LIS3DH_WHO			(0x0F)
#define LIS3DH_NAME			(0x33)

#define LIS3DH_CONVTIME    (24+2) // 24ms MAX

#define LIS3DH_DATA_NOTYET  (-32768)
#define LIS3DH_DATA_ERROR   (-32767)

const uint8 LIS3DH_AXIS[] = {
		LIS3DH_X,
		LIS3DH_Y,
		LIS3DH_Z
};

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE bool_t bGetAxis( uint8 u8axis, uint8* au8data );
PRIVATE void vProcessSnsObj_LIS3DH(void *pvObj, teEvent eEvent);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
void vLIS3DH_Init(tsObjData_LIS3DH *pData, tsSnsObj *pSnsObj) {
	vSnsObj_Init(pSnsObj);

	pSnsObj->pvData = (void*)pData;
	pSnsObj->pvProcessSnsObj = (void*)vProcessSnsObj_LIS3DH;

	memset((void*)pData, 0, sizeof(tsObjData_LIS3DH));
}

void vLIS3DH_Final(tsObjData_LIS3DH *pData, tsSnsObj *pSnsObj) {
	pSnsObj->u8State = E_SNSOBJ_STATE_INACTIVE;
}



/****************************************************************************
 *
 * NAME: bLIS3DHreset
 *
 * DESCRIPTION:
 *   to reset LIS3DH device
 *
 * RETURNS:
 * bool_t	fail or success
 *
 ****************************************************************************/
PUBLIC bool_t bLIS3DHreset()
{
	bool_t bOk = TRUE;
	uint8 command = LIS3DH_SOFT_COM;

	bOk &= bSMBusWrite(LIS3DH_ADDRESS, LIS3DH_SOFT_RST, 1, &command );
	// then will need to wait at least 15ms

	return bOk;
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
PUBLIC bool_t bLIS3DHstartRead()
{
	bool_t bOk = TRUE;
//	uint8 u8name;
	uint8 u8com = LIS3DH_OPT;
#ifdef HR_MODE
	uint8 u8hr = LIS3DH_HR;
#endif

	//	Who am I?
//	bOk &= bSMBusWrite(LIS3DH_ADDRESS, LIS3DH_WHO, 0, NULL);
//	bOk &= bSMBusSequentialRead(LIS3DH_ADDRESS, 1, &u8name);
//	if( !bOk && u8name != LIS3DH_NAME ){
//		return FALSE;
//	}

	// Set Option
	bOk &= bSMBusWrite( LIS3DH_ADDRESS, LIS3DH_SET_OPT, 1, &u8com );

#ifdef HR_MODE
	bOk &= bSMBusWrite( LIS3DH_ADDRESS, LIS3DH_SET_HR, 1, &u8hr );
#endif

	return bOk;
}

/****************************************************************************
 *
 * NAME: u16LIS3DHreadResult
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
PUBLIC int16 i16LIS3DHreadResult( uint8 u8axis )
{
	bool_t	bOk = TRUE;
	int32	i32result=0;
	uint8	au8data[2];

	//	各軸の読み込み
	switch( u8axis ){
		case LIS3DH_IDX_X:
			bOk &= bGetAxis( LIS3DH_IDX_X, au8data );
			break;
		case LIS3DH_IDX_Y:
			bOk &= bGetAxis( LIS3DH_IDX_Y, au8data );
			break;
		case LIS3DH_IDX_Z:
			bOk &= bGetAxis( LIS3DH_IDX_Z, au8data );
			break;
		default:
			bOk = FALSE;
	}
#ifdef HR_MODE
	i32result = ((int16)((au8data[1] << 8) | au8data[0])/32);
#else
	i32result = ((int16)((au8data[1] << 8) | au8data[0])/128);
#endif

	if (bOk == FALSE) {
		i32result = SENSOR_TAG_DATA_ERROR;
	}
#ifdef SERIAL_DEBUG
vfPrintf(&sDebugStream, "\n\rLIS3DH DATA %x", *((uint16*)au8data) );
#endif

    return (int16)i32result;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
PRIVATE bool_t bGetAxis( uint8 u8axis, uint8* au8data )
{
	uint8 i;
	bool_t bOk = TRUE;

	for( i=0; i<2; i++ ){
		bOk &= bSMBusWrite( LIS3DH_ADDRESS, LIS3DH_AXIS[u8axis]+i, 0, NULL );
		bOk &= bSMBusSequentialRead( LIS3DH_ADDRESS, 1, &au8data[i] );
	}

	return bOk;
}

// the Main loop
PRIVATE void vProcessSnsObj_LIS3DH(void *pvObj, teEvent eEvent) {
	tsSnsObj *pSnsObj = (tsSnsObj *)pvObj;
	tsObjData_LIS3DH *pObj = (tsObjData_LIS3DH *)pSnsObj->pvData;

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
vfPrintf(&sDebugStream, "\n\rLIS3DH WAKEUP");
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
			vfPrintf(&sDebugStream, "\n\rLIS3DH KICKED");
			#endif

			break;

		default:
			break;
		}
		break;

	case E_SNSOBJ_STATE_MEASURING:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			pObj->ai16Result[LIS3DH_IDX_X] = SENSOR_TAG_DATA_ERROR;
			pObj->ai16Result[LIS3DH_IDX_Y] = SENSOR_TAG_DATA_ERROR;
			pObj->ai16Result[LIS3DH_IDX_Z] = SENSOR_TAG_DATA_ERROR;
			pObj->u8TickWait = LIS3DH_CONVTIME;

			pObj->bBusy = TRUE;
#ifdef LIS3DH_ALWAYS_RESET
			u8reset_flag = TRUE;
			if (!bLIS3DHreset()) {
				vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
			}
#else
			if (!bLIS3DHstartRead()) { // kick I2C communication
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
#ifdef LIS3DH_ALWAYS_RESET
			if (u8reset_flag) {
				u8reset_flag = 0;
				if (!bLIS3DHstartRead()) {
					vLIS3DH_new_state(pObj, E_SNSOBJ_STATE_COMPLETE);
				}

				pObj->u8TickCount = 0;
				pObj->u8TickWait = LIS3DH_CONVTIME;
				break;
			}
#endif
			pObj->ai16Result[LIS3DH_IDX_X] = i16LIS3DHreadResult(LIS3DH_IDX_X);
			pObj->ai16Result[LIS3DH_IDX_Y] = i16LIS3DHreadResult(LIS3DH_IDX_Y);
			pObj->ai16Result[LIS3DH_IDX_Z] = i16LIS3DHreadResult(LIS3DH_IDX_Z);

			// data arrival
			pObj->bBusy = FALSE;
			vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
		}
		break;

	case E_SNSOBJ_STATE_COMPLETE:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			#ifdef SERIAL_DEBUG
			vfPrintf(&sDebugStream, "\n\rLIS3DH_CP: %d", pObj->i16Result);
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
