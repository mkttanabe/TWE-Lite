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

#include "ToCoNet.h"
#include "sensor_driver.h"
#include "utils.h"
#include "MPL115A2.h"
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
#define MPL115_ADDRESS     (0x60)
#define MPL115A2_CONVERT     (0x12)
#define MPL115A2_CONVTIME    (24+2) // 24ms MAX

//#define PORT_OUT1 18

//#define TEST

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE bool_t bMPL115startConvert(void);
PRIVATE void vProcessSnsObj_MPL115A2(void *pvObj, teEvent eEvent);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
void vMPL115A2_Init(tsObjData_MPL115A2 *pData, tsSnsObj *pSnsObj) {
	vSnsObj_Init(pSnsObj);

	pSnsObj->pvData = (void*)pData;
	pSnsObj->pvProcessSnsObj = (void*)vProcessSnsObj_MPL115A2;

	memset((void*)pData, 0, sizeof(tsObjData_MPL115A2));
}

void vMPL115A2_Final(tsObjData_MPL115A2 *pData, tsSnsObj *pSnsObj) {
	pSnsObj->u8State = E_SNSOBJ_STATE_INACTIVE;
}

/****************************************************************************
 *
 * NAME: bMPL115reset
 *
 * DESCRIPTION:
 *   to reset MPL115 device
 *
 * RETURNS:
 * bool_t	fail or success
 *
 ****************************************************************************/
PUBLIC bool_t bMPL115reset()
{
	bool_t	bOk = TRUE;
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
PUBLIC bool_t bMPL115startRead()
{
	bool_t bOk = TRUE;

	//	変換命令
	bOk &= bMPL115startConvert();

	return bOk;
}

/****************************************************************************
 *
 * NAME: u16MPL115readResult
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
PUBLIC int16 i16MPL115readResult()
{
	bool_t bOk = TRUE;		//	I2C通信が成功したかどうかのフラグ
	int32 i32result;		//	返戻値
	uint8 au8data[8];		//	補正値のバイナリデータを受け取るTemporary
	uint8 au8temp[4];		//	気圧・温度のバイナリデータを受け取るTemporary
	float a0, b1, b2, c12;					//	補正値
	uint16 u16temp, u16pascal;				//	気圧・温度のビット列

	/*	気圧・気温の読み込み命令	*/
	bOk &= bSMBusWrite(MPL115_ADDRESS, 0x00, 0, NULL);
	if (bOk == FALSE) {
		i32result = SENSOR_TAG_DATA_ERROR;
	}else{
		bOk &= bSMBusSequentialRead(MPL115_ADDRESS, 4, au8temp);
		if (bOk == FALSE) {
			i32result = SENSOR_TAG_DATA_ERROR;
		}else{
			//	10bitのデータに変換
			u16pascal = ((au8temp[0]<<8)|au8temp[1]) >> 6;
			u16temp = ((au8temp[2]<<8)|au8temp[3]) >> 6;

			/*	補正値の読み込み命令	*/
			bOk &= bSMBusWrite(MPL115_ADDRESS, 0x04, 0, NULL);
			bOk &= bSMBusSequentialRead(MPL115_ADDRESS, 8, au8data);
			if (bOk == FALSE) {
				i32result = SENSOR_TAG_DATA_ERROR;
			}else{
				/*	補正値の計算 詳しくはデータシート参照 もっときれいにしたい	*/
				a0 = (au8data[0] << 5) + (au8data[1] >> 3) + (au8data[1] & 0x07) / 8.0;
				b1 = ( ( ( (au8data[2] & 0x1F) * 0x100 ) + au8data[3] ) / 8192.0 ) - 3 ;
				b2 = ( ( ( ( au8data[4] - 0x80) << 8 ) + au8data[5] ) / 16384.0 ) - 2 ;
				c12 = ( ( ( au8data[6] * 0x100 ) + au8data[7] ) / 16777216.0 );

				/*	気圧の計算	*/
				i32result = (int32)(((a0+(b1+c12*u16temp)*u16pascal+b2*u16temp)*((115.0-50.0)/1023.0)+50)*10.0);
			}
		}
	}

#ifdef SERIAL_DEBUG
vfPrintf(&sDebugStream, "\n\rMPL115 DATA %x", *((uint16*)au8data) );
#endif

    return (int16)i32result;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
PRIVATE bool_t bMPL115startConvert()
{
	bool_t bOk = TRUE;
    uint8 tmp=0;

	bOk &= bSMBusWrite(MPL115_ADDRESS, MPL115A2_CONVERT, 1, &tmp);

	return bOk;
}

// the Main loop
PRIVATE void vProcessSnsObj_MPL115A2(void *pvObj, teEvent eEvent) {
	tsSnsObj *pSnsObj = (tsSnsObj *)pvObj;
	tsObjData_MPL115A2 *pObj = (tsObjData_MPL115A2 *)pSnsObj->pvData;

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
vfPrintf(&sDebugStream, "\n\rMPL115A2 WAKEUP");
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
			vfPrintf(&sDebugStream, "\n\rMPL115A2 KICKED");
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
			pObj->u8TickWait = MPL115A2_CONVTIME;

			pObj->bBusy = TRUE;
#ifdef MPL115A2_ALWAYS_RESET
			u8reset_flag = TRUE;
			if (!bMPL115A2reset()) {
				vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
			}
#else
			if (!bMPL115startRead()) { // kick I2C communication
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
#ifdef MPL115A2_ALWAYS_RESET
			if (u8reset_flag) {
				u8reset_flag = 0;
				if (!bMPL115startRead()) {
					vMPL115A2_new_state(pObj, E_SNSOBJ_STATE_COMPLETE);
				}

				pObj->u8TickCount = 0;
				pObj->u8TickWait = MPL115A2_CONVTIME;
				break;
			}
#endif
			pObj->i16Result = i16MPL115readResult();

			// data arrival
			pObj->bBusy = FALSE;
			vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
		}
		break;

	case E_SNSOBJ_STATE_COMPLETE:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			#ifdef SERIAL_DEBUG
			vfPrintf(&sDebugStream, "\n\rMPL115A2_CP: %d", pObj->i16Result);
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
