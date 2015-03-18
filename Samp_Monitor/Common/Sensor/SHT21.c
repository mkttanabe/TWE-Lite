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
#include "SHT21.h"
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
#define SHT21_ADDRESS     (0x40)

#define SHT21_TRIG_TEMP   (0xf3)
#define SHT21_TRIG_HUMID  (0xf5)

#define SHT21_WRITE_REG   (0xe6)
#define SHT21_READ_REG    (0xe6)

#define SHT21_SOFT_RST    (0xfe)

#define SHT21_CONVTIME_TEMP  (11+2) // 11ms
#define SHT21_CONVTIME_HUMID (15+2) // 15ms

#define SHT21_DATA_NOTYET  (-32768)
#define SHT21_DATA_ERROR   (-32767)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vProcessSnsObj_SHT21(void *pvObj, teEvent eEvent);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
const uint8 au8TickWait[SHT21_IDX_END] = {
	SHT21_CONVTIME_TEMP,
	SHT21_CONVTIME_HUMID
};
const uint8 au8TrigCmd[SHT21_IDX_END] = {
	SHT21_TRIG_TEMP,
	SHT21_TRIG_HUMID
};


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
void vSHT21_Init(tsObjData_SHT21 *pData, tsSnsObj *pSnsObj) {
	vSnsObj_Init(pSnsObj);

	pSnsObj->pvData = (void*)pData;
	pSnsObj->pvProcessSnsObj = (void*)vProcessSnsObj_SHT21;

	memset((void*)pData, 0, sizeof(tsObjData_SHT21));
}

void vSHT21_Final(tsObjData_SHT21 *pData, tsSnsObj *pSnsObj) {
	pSnsObj->u8State = E_SNSOBJ_STATE_INACTIVE;
}

/****************************************************************************
 *
 * NAME: vSHT21reset
 *
 * DESCRIPTION:
 *   to reset SHT21 device
 *
 * RETURNS:
 * bool_t	fail or success
 *
 ****************************************************************************/
PUBLIC bool_t bSHT21reset()
{
	bool_t bOk = TRUE;

	bOk &= bSMBusWrite(SHT21_ADDRESS, SHT21_SOFT_RST, 0, NULL);
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
PUBLIC bool_t bSHT21startRead(uint8 u8trig)
{
	bool_t bOk = TRUE;
	uint8 u8reg;

	// read user register setting
	bOk &= bSMBusWrite(SHT21_ADDRESS, SHT21_READ_REG, 0, NULL);
	if (!bOk) return FALSE;
	bOk &= bSMBusSequentialRead(SHT21_ADDRESS, 1, &u8reg);
	if (!bOk) return FALSE;
#ifdef SERIAL_DEBUG
//vfPrintf(&sDebugStream, "\n\rSHT21 READ REG %x", u8reg);
#endif

	if ((u8reg & 0x81) != 0x81) {
		u8reg &= 0x7E; // mask bit1 ... 6
		u8reg |= 0x81; // write bit0, 7
					   // 0x81:11bit, 0x80:13bit, 0x01:12bit, 0x00:14bit

		// set register mode
		bOk &= bSMBusWrite(SHT21_ADDRESS, SHT21_WRITE_REG, 1, &u8reg);
		if (!bOk) return FALSE;
#ifdef SERIAL_DEBUG
//vfPrintf(&sDebugStream, "\n\rSHT21 WRITE REG %x", u8reg);
#endif
	}

	// start conversion (will take some ms according to bits accuracy)
	bOk &= bSMBusWrite(SHT21_ADDRESS, u8trig, 0, NULL);
#ifdef SERIAL_DEBUG
//vfPrintf(&sDebugStream, "\n\rSHT21 TRIG %x", u8trig);
#endif

	return bOk;
}

/****************************************************************************
 *
 * NAME: u16SHT21readResult
 *
 * DESCRIPTION:
 * Wrapper to read a measurement, followed by a conversion function to work
 * out the value in degrees Celcius.
 *
 * RETURNS:
 * int16: temperature in degrees Celcius x 100 (-4685 to 12886)
 *        0x8000, error
 *
 * NOTES:
 * the data conversion fomula is :
 *      TEMP:  -46.85+175.72*ReadValue/65536
 *      HUMID: -6+125*ReadValue/65536
 *
 *    where the 14bit ReadValue is scaled up to 16bit
 *
 ****************************************************************************/
PUBLIC int16 i16SHT21readResult(int16 *pi16Temp, int16 *pi16Humid)
{
	bool_t bOk = TRUE;
    int32 i32result;
    uint8 au8data[4];

    bOk &= bSMBusSequentialRead(SHT21_ADDRESS, 3, au8data);
    if(!bOk) return SHT21_DATA_NOTYET; // error
#ifdef SERIAL_DEBUG
vfPrintf(&sDebugStream, "\n\rSHT_DT %x %x %x", au8data[0], au8data[1], au8data[2]);
#endif

	// CRC8 check
	au8data[3] = u8CCITT8(au8data, 2);
	if (au8data[2] != au8data[3]) return SHT21_DATA_ERROR;

    i32result = (au8data[1] & 0xfc) | (au8data[0] << 8);
    if (au8data[1] & 0x02) {
    	// bit1:1 --> humid
		i32result = ((12500*i32result + 32768) >> 16) - 600;
        if (pi16Temp) *pi16Humid = (int16)i32result;
    } else {
    	// bit1:0 --> temp
        i32result = ((17572*i32result + 32768) >> 16) - 4685;
        if (pi16Temp) *pi16Temp = (int16)i32result;
    }
#ifdef SERIAL_DEBUG
//vfPrintf(&sDebugStream, "\n\rSHT21 CORRECT DATA %s=%d",
//		(au8data[1] & 0x02) ? "HUMID" : "TEMP", i32result);
#endif

    return (int16)i32result;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
// the Main loop
void vProcessSnsObj_SHT21(void *pvObj, teEvent eEvent) {
	tsSnsObj *pSnsObj = (tsSnsObj *)pvObj;
	tsObjData_SHT21 *pObj = (tsObjData_SHT21 *)pSnsObj->pvData;

	// general process
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
vfPrintf(&sDebugStream, "\n\rSHT21 WAKEUP");
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
			pObj->u8IdxMeasuruing = SHT21_IDX_BEGIN;
			break;

		case E_ORDER_KICK:
			pObj->ai16Result[SHT21_IDX_TEMP] = SENSOR_TAG_DATA_ERROR;
			pObj->ai16Result[SHT21_IDX_HUMID] = SENSOR_TAG_DATA_ERROR;

			vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_MEASURING);

			#ifdef SERIAL_DEBUG
			vfPrintf(&sDebugStream, "\n\rSHT21 KICKED");
			#endif
			break;

		default:
			break;
		}
		break;

	case E_SNSOBJ_STATE_MEASURING:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
#ifdef SERIAL_DEBUG
vfPrintf(&sDebugStream, "\n\rSHT_ST:%d", pObj->u8IdxMeasuruing);
#endif

			pObj->ai16Result[pObj->u8IdxMeasuruing] = SENSOR_TAG_DATA_ERROR;
			pObj->u8TickWait = au8TickWait[pObj->u8IdxMeasuruing];

			// kick I2C communication
			if (!bSHT21startRead(au8TrigCmd[pObj->u8IdxMeasuruing])) {
				vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE); // error
			}

			pObj->u8TickCount = 0;
			break;

		default:
			break;
		}

		// wait until completion
		if (pObj->u8TickCount > pObj->u8TickWait) {
			int16 i16ret;
			i16ret = i16SHT21readResult(
					&(pObj->ai16Result[SHT21_IDX_TEMP]),
					&(pObj->ai16Result[SHT21_IDX_HUMID]) );

			if (i16ret == SENSOR_TAG_DATA_ERROR) {
				vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE); // error
			} else
			if (i16ret == SENSOR_TAG_DATA_NOTYET) {
				// still conversion
				#ifdef SERIAL_DEBUG
				vfPrintf(&sDebugStream, "\r\nSHT_ND");
				#endif

				pObj->u8TickCount /= 2; // wait more...
			} else {
				// data arrival
				vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_MEASURE_NEXT);
			}
		}
		break;

	case E_SNSOBJ_STATE_MEASURE_NEXT:
		switch (eEvent) {
			case E_EVENT_NEW_STATE:
				pObj->u8IdxMeasuruing++;

				if (pObj->u8IdxMeasuruing < SHT21_IDX_END) {
					// complete all data
					vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_MEASURING);
				} else {
					// complete all data
					vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
				}
				break;

			default:
				break;
		}
		break;

	case E_SNSOBJ_STATE_COMPLETE:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			#ifdef SERIAL_DEBUG
			vfPrintf(&sDebugStream, "\n\rSHT_CP: T%d H%d",
					pObj->ai16Result[SHT21_IDX_TEMP],
					pObj->ai16Result[SHT21_IDX_HUMID]);
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
