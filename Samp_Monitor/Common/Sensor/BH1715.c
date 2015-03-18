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

#include "sensor_driver.h"
#include "BH1715.h"
#include "SMBus.h"

#include "ccitt8.h"

#undef SERIAL_DEBUG
#ifdef SERIAL_DEBUG
# include <serial.h>
# include <fprintf.h>
extern tsFILE sDebugStream;
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define BH1715_ADDRESS     (0x23)

#define BH1715_TRIG        (0x23)

#define BH1715_SOFT_RST    (0x07)

#define BH1715_CONVTIME    (24+2) // 24ms MAX

#define BH1715_DATA_NOTYET  (-32768)
#define BH1715_DATA_ERROR   (-32767)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vProcessSnsObj_BH1715(void *pObj, teEvent eEvent);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
#ifdef BH1715_ALWAYS_RESET
static uint8 u8reset_flag = 0;
#endif

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
void vBH1715_Init(tsObjData_BH1715 *pData, tsSnsObj *pSnsObj) {
	vSnsObj_Init(pSnsObj);

	pSnsObj->pvData = (void*)pData;
	pSnsObj->pvProcessSnsObj = (void*)vProcessSnsObj_BH1715;

	memset((void*)pData, 0, sizeof(tsObjData_BH1715));
}

void vBH1715_Final(tsObjData_BH1715 *pData, tsSnsObj *pSnsObj) {
	pSnsObj->u8State = E_SNSOBJ_STATE_INACTIVE;
}

/****************************************************************************
 *
 * NAME: bBH1715reset
 *
 * DESCRIPTION:
 *   to reset BH1715 device
 *
 * RETURNS:
 * bool_t	fail or success
 *
 ****************************************************************************/
PUBLIC bool_t bBH1715reset()
{
	bool_t bOk = TRUE;

	bOk &= bSMBusWrite(BH1715_ADDRESS, BH1715_SOFT_RST, 0, NULL);
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
PUBLIC bool_t bBH1715startRead()
{
	bool_t bOk = TRUE;

	// start conversion (will take some ms according to bits accuracy)
	bOk &= bSMBusWrite(BH1715_ADDRESS, BH1715_TRIG, 0, NULL);

	return bOk;
}

/****************************************************************************
 *
 * NAME: u16BH1715readResult
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
PUBLIC int16 i16BH1715readResult()
{
	bool_t bOk = TRUE;
    int32 i32result;
    uint8 au8data[4];

    bOk &= bSMBusSequentialRead(BH1715_ADDRESS, 2, au8data);
    if (bOk == FALSE) {
    	i32result = SENSOR_TAG_DATA_ERROR;
    }

    i32result = ((au8data[0] << 8) | au8data[1]);
    i32result = (i32result * 10 + 30) / 60; // i32result/1.2/5

#ifdef SERIAL_DEBUG
vfPrintf(&sDebugStream, "\n\rBH1715 DATA %x", *((uint16*)au8data) );
#endif

    return (int16)i32result;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
// the Main loop
PRIVATE void vProcessSnsObj_BH1715(void *pvObj, teEvent eEvent) {
	tsSnsObj *pSnsObj = (tsSnsObj *)pvObj;
	tsObjData_BH1715 *pObj = (tsObjData_BH1715 *)pSnsObj->pvData;

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
vfPrintf(&sDebugStream, "\n\rBH1715 WAKEUP");
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
			vfPrintf(&sDebugStream, "\n\rBH1715 KICKED");
			#endif

			break;

		default:
			break;
		}
		break;

	case E_SNSOBJ_STATE_MEASURING:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			pObj->i16Result = SENSOR_TAG_DATA_ERROR;
			pObj->u8TickWait = BH1715_CONVTIME;

			pObj->bBusy = TRUE;
#ifdef BH1715_ALWAYS_RESET
			u8reset_flag = TRUE;
			if (!bBH1715reset()) {
				vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
			}
#else
			if (!bBH1715startRead()) { // kick I2C communication
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
#ifdef BH1715_ALWAYS_RESET
			if (u8reset_flag) {
				u8reset_flag = 0;
				if (!bBH1715startRead()) {
					vBH1715_new_state(pObj, E_SNSOBJ_STATE_COMPLETE);
				}

				pObj->u8TickCount = 0;
				pObj->u8TickWait = BH1715_CONVTIME;
				break;
			}
#endif
			pObj->i16Result = i16BH1715readResult();

			// data arrival
			pObj->bBusy = FALSE;
			vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
		}
		break;

	case E_SNSOBJ_STATE_COMPLETE:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			#ifdef SERIAL_DEBUG
			vfPrintf(&sDebugStream, "\n\rBH1715_CP: %d", pObj->i16Result);
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
