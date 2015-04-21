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
#include <math.h>

#include "sensor_driver.h"
#include "TSL2561.h"
#include "SMBus.h"

#include "utils.h"
#include "Interactive.h"

#include "ccitt8.h"

#include <serial.h>
#include <fprintf.h>

#undef SERIAL_DEBUG
#ifdef SERIAL_DEBUG
# include <serial.h>
# include <fprintf.h>
extern tsFILE sDebugStream;
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define TSL2561_ADDRESS     (0x39)

#define TSL2561_DATA0        (0x0C)
#define TSL2561_DATA1        (0x0E)

#define TSL2561_CONVTIME    (24+2) // 24ms MAX

#define CMD 0x80
#define CREAR 0x40
#define WORD 0x20
#define BLOCK 0x10

#define u32CalcLux(c) u32CalclateLux( GAIN_1X, INTG_13MS, c )

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vProcessSnsObj_TSL2561(void *pvObj, teEvent eEvent);
PRIVATE uint32 u32CalclateLux( uint8 u8Gain, uint8 u8Intg, uint16* pu16Data );
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
extern tsFILE sSerStream;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
void vTSL2561_Init(tsObjData_TSL2561 *pData, tsSnsObj *pSnsObj) {
	vSnsObj_Init(pSnsObj);

	pSnsObj->pvData = (void*)pData;
	pSnsObj->pvProcessSnsObj = (void*)vProcessSnsObj_TSL2561;

	memset((void*)pData, 0, sizeof(tsObjData_TSL2561));

	uint8 opt = 0x03;
	bSMBusWrite(TSL2561_ADDRESS, CMD | 0x00, 1, &opt );
}

void vTSL2561_Final(tsObjData_TSL2561 *pData, tsSnsObj *pSnsObj) {
	pSnsObj->u8State = E_SNSOBJ_STATE_INACTIVE;
}


/****************************************************************************
 *
 * NAME: bTSL2561reset
 *
 * DESCRIPTION:
 *   to reset TSL2561 device
 *
 * RETURNS:
 * bool_t	fail or success
 *
 ****************************************************************************/
PUBLIC bool_t bTSL2561reset( bool_t bMode16 )
{
	bool_t bOk = TRUE;

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
PUBLIC bool_t bTSL2561startRead()
{
	bool_t bOk = TRUE;

	return bOk;
}

/****************************************************************************
 *
 * NAME: u16TSL2561readResult
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
PUBLIC uint32 u32TSL2561readResult( void )
{
	bool_t bOk = TRUE;
	uint32 u32result = 0x00;
	uint16 data[2];
	uint8 au8data[2];
	uint8 command;
	uint32 Lux;

	command = CMD | WORD | TSL2561_DATA0;
	bOk &= bSMBusWrite(TSL2561_ADDRESS, command, 0, NULL );
	bOk &= bSMBusSequentialRead(TSL2561_ADDRESS, 2, au8data);
	if (bOk == FALSE) {
		u32result = (uint16)SENSOR_TAG_DATA_ERROR;
	}else{
		data[0] = ((au8data[1] << 8) | au8data[0]);	//	読み込んだ数値を代入

		command = CMD | WORD | TSL2561_DATA1;
		bOk &= bSMBusWrite(TSL2561_ADDRESS, command, 0, NULL );
		bOk &= bSMBusSequentialRead(TSL2561_ADDRESS, 2, au8data);
		if (bOk == FALSE) {
			u32result = SENSOR_TAG_DATA_ERROR;
		}
		data[1] = ((au8data[1] << 8) | au8data[0]);	//	読み込んだ数値を代入

		//	TSL2561FN 照度の計算
		Lux = u32CalcLux(data);

		u32result = Lux;
	}
#ifdef SERIAL_DEBUG
vfPrintf(&sDebugStream, "\n\rTSL2561 DATA %x", *((uint16*)au8data) );
#endif

    return u32result;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
	lux equation approximation without floating point calculations
	Routine: unsigned int CalculateLux(unsigned int ch0, unsigned int ch0, int iType)

	Description: Calculate the approximate illuminance (lux) given the raw
	channel values of the TSL2560. The equation if implemented
	as a piece−wise linear approximation.

	Arguments: unsigned int iGain − gain, where 0:1X, 1:16X
	unsigned int tInt − integration time, where 0:13.7mS, 1:100mS, 2:402mS, 3:Manual
	unsigned int ch0 − raw channel value from channel 0 of TSL2560
	unsigned int ch1 − raw channel value from channel 1 of TSL2560

	Return: unsigned int − the approximate illuminance (lux)

****************************************************************************/
uint32 u32CalclateLux( uint8 u8Gain, uint8 u8Intg, uint16* pu16Data )
{
	uint16 chScale;
	uint32 channel0, channel1;

	switch(u8Intg){
		case INTG_13MS:
			chScale = CHSCALE_TINT0;
			break;
		case INTG_101MS:
			chScale = CHSCALE_TINT1;
			break;
		default:
			chScale = (1 << CH_SCALE);
			break;
	}

	if (!u8Gain)
		chScale = chScale << 4;		// scale 1X to 16X

	// scale the channel values
	channel0 = (pu16Data[0] * chScale) >> CH_SCALE;
	channel1 = (pu16Data[1] * chScale) >> CH_SCALE;

	//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
	// find the ratio of the channel values (Channel1/Channel0)
	// protect against divide by zero
	uint32	ratio1 = 0;
	if (channel0 != 0)
		ratio1 = (channel1 << (RATIO_SCALE+1)) / channel0;

	// round the ratio value
	uint32	ratio = (ratio1 + 1) >> 1;
	// is ratio <= eachBreak ?
	uint16	b, m;
//	A_PRINTF("\r\nratio = %d", ratio);

	if ((ratio >= 0) && (ratio <= K1T)){
		b=B1T;
		m=M1T;
	}else if (ratio <= K2T){
		b=B2T;
		m=M2T;
	}else if (ratio <= K3T){
		b=B3T;
		m=M3T;
	}else if (ratio <= K4T){
		b=B4T;
		m=M4T;
	}else if (ratio <= K5T){
		b=B5T;
		m=M5T;
	}else if (ratio <= K6T){
		b=B6T;
		m=M6T;
	}else if (ratio <= K7T){
		b=B7T;
		m=M7T;
	}else if (ratio > K8T){
		b=B8T;
		m=M8T;
	}

	uint32	temp;
	temp = ((channel0 * b) - (channel1 * m));
	// do not allow negative lux value
	if (temp < 0){
		temp = 0;
	}
	// round lsb (2^(LUX_SCALE−1))
	temp += (1 << (LUX_SCALE-1));
	// strip off fractional portion
	uint32 u32lux = temp >> LUX_SCALE;

	return u32lux;
}


// the Main loop
PRIVATE void vProcessSnsObj_TSL2561(void *pvObj, teEvent eEvent) {
	tsSnsObj *pSnsObj = (tsSnsObj *)pvObj;
	tsObjData_TSL2561 *pObj = (tsObjData_TSL2561 *)pSnsObj->pvData;

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
vfPrintf(&sDebugStream, "\n\rTSL2561 WAKEUP");
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
			vfPrintf(&sDebugStream, "\n\rTSL2561 KICKED");
			#endif

			break;

		default:
			break;
		}
		break;

	case E_SNSOBJ_STATE_MEASURING:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			pObj->u32Result = SENSOR_TAG_DATA_ERROR;
			pObj->u8TickWait = TSL2561_CONVTIME;

			pObj->bBusy = TRUE;
#ifdef TSL2561_ALWAYS_RESET
			u8reset_flag = TRUE;
			if (!bTSL2561reset(TURE)) {
				vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
			}
#else
			if (!bTSL2561startRead()) { // kick I2C communication
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
#ifdef TSL2561_ALWAYS_RESET
			if (u8reset_flag) {
				u8reset_flag = 0;
				if (!bTSL2561startRead()) {
					vTSL2561_new_state(pObj, E_SNSOBJ_STATE_COMPLETE);
				}

				pObj->u8TickCount = 0;
				pObj->u8TickWait = TSL2561_CONVTIME;
				break;
			}
#endif
			pObj->u32Result = u32TSL2561readResult();

			// data arrival
			pObj->bBusy = FALSE;
			vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
		}
		break;

	case E_SNSOBJ_STATE_COMPLETE:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			#ifdef SERIAL_DEBUG
			vfPrintf(&sDebugStream, "\n\rTSL2561_CP: %d", pObj->i16Result);
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
