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
#include<math.h>

#include "jendefs.h"
#include "AppHardwareApi.h"
#include "string.h"
#include "fprintf.h"

#include "sensor_driver.h"
#include "ADXL345.h"
#include "SMBus.h"

#include "ccitt8.h"

#include "utils.h"

#include "Interactive.h"

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
#ifdef LITE2525A
#define ADXL345_ADDRESS		(0x1D)
#else
#define ADXL345_ADDRESS		(0x53)
#endif

#define ADXL345_CONVTIME    (0)//(24+2) // 24ms MAX

#define ADXL345_DATA_NOTYET	(-32768)
#define ADXL345_DATA_ERROR	(-32767)

#define ADXL345_THRESH_TAP		0x1D
#define ADXL345_OFSX			0x1E
#define ADXL345_OFSY			0x1F
#define ADXL345_OFSZ			0x20
#define ADXL345_DUR				0x21
#define ADXL345_LATENT			0x22
#define ADXL345_WINDOW			0x23
#define ADXL345_THRESH_ACT		0x24
#define ADXL345_THRESH_INACT	0x25
#define ADXL345_TIME_INACT		0x26
#define ADXL345_ACT_INACT_CTL	0x27
#define ADXL345_THRESH_FF		0x28
#define ADXL345_TIME_FF			0x29
#define ADXL345_TAP_AXES		0x2A
#define ADXL345_ACT_TAP_STATUS	0x2B
#define ADXL345_BW_RATE			0x2C
#define ADXL345_POWER_CTL		0x2D
#define ADXL345_INT_ENABLE		0x2E
#define ADXL345_INT_MAP			0x2F
#define ADXL345_INT_SOURCE		0x30
#define ADXL345_DATA_FORMAT		0x31
#define ADXL345_DATAX0			0x32
#define ADXL345_DATAX1			0x33
#define ADXL345_DATAY0			0x34
#define ADXL345_DATAY1			0x35
#define ADXL345_DATAZ0			0x36
#define ADXL345_DATAZ1			0x37
#define ADXL345_FIFO_CTL		0x38
#define ADXL345_FIFO_STATUS		0x39

#define ADXL345_X	ADXL345_DATAX0
#define ADXL345_Y	ADXL345_DATAY0
#define ADXL345_Z	ADXL345_DATAZ0

const uint8 ADXL345_AXIS[] = {
		ADXL345_X,
		ADXL345_Y,
		ADXL345_Z
};


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE bool_t bGetAxis( uint8 u8axis, uint8* au8data );
PRIVATE uint8 u8IntSource( void );
PRIVATE void vProcessSnsObj_ADXL345(void *pvObj, teEvent eEvent);
void vRead_Settings( void );

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
static uint8 u8modeflag = 0x00;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
void vADXL345_Init(tsObjData_ADXL345 *pData, tsSnsObj *pSnsObj) {
	vSnsObj_Init(pSnsObj);

	pSnsObj->pvData = (void*)pData;
	pSnsObj->pvProcessSnsObj = (void*)vProcessSnsObj_ADXL345;

	memset((void*)pData, 0, sizeof(tsObjData_ADXL345));
}

void vADXL345_Final(tsObjData_ADXL345 *pData, tsSnsObj *pSnsObj) {
	pSnsObj->u8State = E_SNSOBJ_STATE_INACTIVE;
}

//	センサの設定を記述する関数
bool_t bADXL345_Setting( int16 i16mode, tsADXL345Param sParam )
{
	u8modeflag = (uint8)i16mode;

	uint8 com = 0x08;
	if( i16mode == ACTIVE ){
		com |= 0x20;
	}
	bool_t bOk = bSMBusWrite(ADXL345_ADDRESS, ADXL345_POWER_CTL, 1, &com );

	com = 0x0B;		//	Full Resolution Mode, +-16g
	bOk = bSMBusWrite(ADXL345_ADDRESS, ADXL345_DATA_FORMAT, 1, &com );

	//	タップを判別するための閾値
	if( i16mode == S_TAP || i16mode == D_TAP ){
		if( sParam.u16ThresholdTap != 0 ){
			com = (uint8)( sParam.u16ThresholdTap/625*10 );
			if( 0x00FF < ( sParam.u16ThresholdTap/625*10 ) ){
				com = 0xFF;
			}
		}else{
			com = 0x32;			//	threshold of tap
		}
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_THRESH_TAP, 1, &com );

	//	タップを認識するための時間
	if( i16mode == S_TAP || i16mode == D_TAP ){
		if( sParam.u16Duration != 0 ){
			com = (uint8)( sParam.u16Duration/625*10 );
			if( 0x00FF < ( sParam.u16Duration/625*10 ) ){
				com = 0xFF;
			}
		}else{
			com = 0x0F;
		}
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_DUR, 1, &com );

	//	次のタップまでの時間
	if( i16mode == S_TAP || i16mode == D_TAP ){
		if( sParam.u16Latency != 0 ){
			com = sParam.u16Latency*100/125;
			if( 0x00FF < ( sParam.u16Latency*100/125 ) ){
				com = 0xFF;
			}
		}else{
			com = 0x50;
		}
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_LATENT, 1, &com );

	//	ダブルタップを認識するための時間
	if( i16mode == D_TAP ){
		if( sParam.u16Window != 0 ){
			com = sParam.u16Window*100/125;
			if( 0x00FF < ( sParam.u16Window*100/125 ) ){
				com = 0xFF;
			}
		}else{
			com = 0xC8;			// Window Width
		}
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_WINDOW, 1, &com );

	//	タップを検知する軸の設定
	if( i16mode == S_TAP || i16mode == D_TAP ){
		com = 0x07;
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_TAP_AXES, 1, &com );

	//	自由落下を検知するための閾値
	if( i16mode == FREEFALL ){
		if( sParam.u16ThresholdFreeFall != 0 ){
			com = (uint8)sParam.u16ThresholdFreeFall/625*10;
			if( 0x00FF < ( sParam.u16ThresholdFreeFall/625*10 ) ){
				com = 0xFF;
			}
		}else{
			com = 0x07;			// threshold of freefall
		}
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_THRESH_FF, 1, &com );

	//	自由落下を検知するための時間
	if( i16mode == FREEFALL ){
		if( sParam.u16TimeFreeFall != 0 ){
			com = (uint8)sParam.u16TimeFreeFall/5;
			if( 0x00FF < ( sParam.u16TimeFreeFall/5 ) ){
				com = 0xFF;
			}
		}else{
			com = 0x2D;			// time of freefall
		}
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_TIME_FF, 1, &com );

	//	動いていることを判断するための閾値
	if( i16mode == ACTIVE ){
		if( sParam.u16ThresholdActive != 0 ){
			com = (uint8)sParam.u16ThresholdActive/625*10;
			if( 0x00FF < ( sParam.u16ThresholdActive/625*10 ) ){
				com = 0xFF;
			}
		}else{
			com = 0x20;
		}
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_THRESH_ACT, 1, &com );

	//	動いていないことを判断するための閾値
	if( i16mode == ACTIVE || i16mode == INACTIVE ){
		if( sParam.u16ThresholdInactive != 0 ){
			com = (uint8)sParam.u16ThresholdInactive/625*10;
			if( 0x00FF < ( sParam.u16ThresholdInactive/625*10 ) ){
				com = 0xFF;
			}
		}else if( i16mode == ACTIVE ){
			com = 0x1F;
		} else {
			com = 0x14;
		}
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_THRESH_INACT, 1, &com );

	//	動いていないことを判断するための時間(s)
	if( i16mode == ACTIVE || i16mode == INACTIVE ){
		if( sParam.u16TimeInactive != 0 ){
			com = (uint8)sParam.u16TimeInactive;
			if( 0x00FF < sParam.u16TimeInactive ){
				com = 0xFF;
			}
		} else if( i16mode == ACTIVE ){
			com = 0x02;
		} else if( i16mode == INACTIVE ){
			com = 0x05;
		}
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_TIME_INACT, 1, &com );

	//	動いている/いないことを判断するための軸
	if( i16mode == ACTIVE ){
		com = 0x77;
	}else if( i16mode == INACTIVE ){
		com = 0x07;
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_ACT_INACT_CTL, 1, &com );

	if( i16mode == ACTIVE ){
		com = 0x08;		//	INACTIVEをINT2にして割り込みを無視する
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_INT_MAP, 1, &com );

	//	有効にする割り込みの設定
	if( i16mode == S_TAP ){
		com = 0x40;
	}else if( i16mode == D_TAP ){
		com = 0x20;
	}else if( i16mode == FREEFALL ){
		com = 0x04;
	}else if( i16mode == ACTIVE ){
		com = 0x18;		//	INACTIVEも割り込みさせる
	}else if( i16mode == INACTIVE ){
		com = 0x08;
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_INT_ENABLE, 1, &com );

	if( i16mode == NEKOTTER ){
		com = 0xC0 | 0x20 | 31;
	}else{
		com = 0x00;
	}
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_FIFO_CTL, 1, &com );

//	vRead_Settings();

	return bOk;
}

/****************************************************************************
 *
 * NAME: bADXL345reset
 *
 * DESCRIPTION:
 *   to reset ADXL345 device
 *
 * RETURNS:
 * bool_t	fail or success
 *
 ****************************************************************************/
PUBLIC bool_t bADXL345reset()
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
PUBLIC bool_t bADXL345startRead()
{
	bool_t	bOk = TRUE;

	return bOk;
}

/****************************************************************************
 *
 * NAME: u16ADXL345readResult
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
PUBLIC int16 i16ADXL345readResult( uint8 u8axis )
{
	bool_t	bOk = TRUE;
	int16	i16result=0;
	uint8	au8data[2];

	//	各軸の読み込み
	switch( u8axis ){
		case ADXL345_IDX_X:
			bOk &= bGetAxis( ADXL345_IDX_X, au8data );
			break;
		case ADXL345_IDX_Y:
			bOk &= bGetAxis( ADXL345_IDX_Y, au8data );
			break;
		case ADXL345_IDX_Z:
			bOk &= bGetAxis( ADXL345_IDX_Z, au8data );
			break;
		default:
			bOk = FALSE;
	}
	i16result = (((au8data[1] << 8) | au8data[0]));
	i16result = i16result*4/10;			//	1bitあたり4mg  10^-2まで有効

	if (bOk == FALSE) {
		i16result = SENSOR_TAG_DATA_ERROR;
	}


	return i16result;
}

PUBLIC bool_t bNekotterreadResult( int16* ai16accel )
{
	bool_t	bOk = TRUE;
	int16	ai16result[3];
	uint8	au8data[2];
	int8	num;
	uint8	data;
	uint8	i, j;
	int16	ave;
	int16	sum[3] = { 0, 0, 0 };		//	サンプルの総和
	uint32	ssum[3] = { 0, 0, 0 };		//	サンプルの2乗和
	uint8	com;

	//	FIFOでたまった個数を読み込む
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_FIFO_STATUS, 0, NULL );
	bOk &= bSMBusSequentialRead( ADXL345_ADDRESS, 1, &data );

	num = (int8)(data&0x7f);
	for( i=0; i<num; i++ ){
		//	各軸の読み込み
		//	X軸
		bOk &= bGetAxis( ADXL345_IDX_X, au8data );
		ai16result[ADXL345_IDX_X] = (((au8data[1] << 8) | au8data[0]));
		//	Y軸
		bOk &= bGetAxis( ADXL345_IDX_Y, au8data );
		ai16result[ADXL345_IDX_Y] = (((au8data[1] << 8) | au8data[0]));
		//	Z軸
		bOk &= bGetAxis( ADXL345_IDX_Z, au8data );
		ai16result[ADXL345_IDX_Z] = (((au8data[1] << 8) | au8data[0]));

		//	総和と二乗和の計算
		for( j=0; j<3; j++ ){
			sum[j] += ai16result[j];
			ssum[j] += ai16result[j]*ai16result[j];
		}
	}

	for( i=0; i<3; i++ ){
	//	分散が評価値 分散の式を変形
		ave = sum[i]/num;
		ai16accel[i] = (int16)sqrt((double)(-1*(ave*ave)+(ssum[i]/num)));
	}

	//	ねこったーモードはじめ
	//	FIFOの設定をもう一度
	com = 0x00 | 0x20 | 31;
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_FIFO_CTL, 0, NULL );
	com = 0xc0 | 0x20 | 31;
	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_FIFO_CTL, 0, NULL );
	//	終わり

    return bOk;
}

void vRead_Settings( void ){
	uint8	u8source;
	bool_t bOk = TRUE;

	bOk = bSMBusWrite(ADXL345_ADDRESS, ADXL345_POWER_CTL, 0, NULL );
	bOk &= bSMBusSequentialRead( ADXL345_ADDRESS, 1, &u8source );
	A_PRINTF( "\r\nPOWER_CTL = %02X", u8source );

	bOk = bSMBusWrite(ADXL345_ADDRESS, ADXL345_DATA_FORMAT, 0, NULL );
	bOk &= bSMBusSequentialRead( ADXL345_ADDRESS, 1, &u8source );
	A_PRINTF( "\r\nDATA_FORMAT = %02X", u8source );

	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_THRESH_ACT, 0, NULL );
	bOk &= bSMBusSequentialRead( ADXL345_ADDRESS, 1, &u8source );
	A_PRINTF( "\r\nTHRESH_ACT = %02X", u8source );

	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_ACT_INACT_CTL, 0, NULL );
	bOk &= bSMBusSequentialRead( ADXL345_ADDRESS, 1, &u8source );
	A_PRINTF( "\r\nACT_INACT_CTL = %02X", u8source );

	bOk &= bSMBusWrite(ADXL345_ADDRESS, ADXL345_INT_ENABLE, 0, NULL );
	bOk &= bSMBusSequentialRead( ADXL345_ADDRESS, 1, &u8source );
	A_PRINTF( "\r\nINIT_ENABLE = %02X", u8source );

	bOk &= bSMBusWrite( ADXL345_ADDRESS, 0x30, 0, NULL );
	bOk &= bSMBusSequentialRead( ADXL345_ADDRESS, 1, &u8source );
	A_PRINTF( "\r\nINIT_SOURCE = %02X", u8source );
	A_PRINTF( "\r\n" );

}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
PRIVATE bool_t bGetAxis( uint8 u8axis, uint8* au8data )
{
	bool_t bOk = TRUE;

	bOk &= bSMBusWrite( ADXL345_ADDRESS, ADXL345_AXIS[u8axis], 0, NULL );
	bOk &= bSMBusSequentialRead( ADXL345_ADDRESS, 2, au8data );

	return bOk;
}

PRIVATE uint8 u8IntSource(void)
{
	bool_t	bOk = TRUE;
	uint8	u8source;

	bOk &= bSMBusWrite( ADXL345_ADDRESS, 0x30, 0, NULL );
	bOk &= bSMBusSequentialRead( ADXL345_ADDRESS, 1, &u8source );

	return bOk;
}

// the Main loop
PRIVATE void vProcessSnsObj_ADXL345(void *pvObj, teEvent eEvent) {
	tsSnsObj *pSnsObj = (tsSnsObj *)pvObj;
	tsObjData_ADXL345 *pObj = (tsObjData_ADXL345 *)pSnsObj->pvData;

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
vfPrintf(&sDebugStream, "\n\rADXL345 WAKEUP");
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
			vfPrintf(&sDebugStream, "\n\rADXL345 KICKED");
			#endif

			break;

		default:
			break;
		}
		break;

	case E_SNSOBJ_STATE_MEASURING:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			pObj->u8Interrupt = 0xFF;
			pObj->ai16Result[ADXL345_IDX_X] = SENSOR_TAG_DATA_ERROR;
			pObj->ai16Result[ADXL345_IDX_Y] = SENSOR_TAG_DATA_ERROR;
			pObj->ai16Result[ADXL345_IDX_Z] = SENSOR_TAG_DATA_ERROR;
			pObj->u8TickWait = ADXL345_CONVTIME;

			pObj->bBusy = TRUE;
#ifdef ADXL345_ALWAYS_RESET
			u8reset_flag = TRUE;
			if (!bADXL345reset()) {
				vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
			}
#else
			if (!bADXL345startRead()) { // kick I2C communication
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
#ifdef ADXL345_ALWAYS_RESET
			if (u8reset_flag) {
				u8reset_flag = 0;
				if (!bADXL345startRead()) {
					vADXL345_new_state(pObj, E_SNSOBJ_STATE_COMPLETE);
				}

				pObj->u8TickCount = 0;
				pObj->u8TickWait = ADXL345_CONVTIME;
				break;
			}
#endif
			pObj->u8Interrupt = u8IntSource();
			if( u8modeflag != NEKOTTER ){
				pObj->ai16Result[ADXL345_IDX_X] = i16ADXL345readResult(ADXL345_IDX_X);
				pObj->ai16Result[ADXL345_IDX_Y] = i16ADXL345readResult(ADXL345_IDX_Y);
				pObj->ai16Result[ADXL345_IDX_Z] = i16ADXL345readResult(ADXL345_IDX_Z);
//				vRead_Settings();
			}else{
				bNekotterreadResult(pObj->ai16Result);
			}

			// data arrival
			pObj->bBusy = FALSE;
			vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
		}
		break;

	case E_SNSOBJ_STATE_COMPLETE:
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			#ifdef SERIAL_DEBUG
			vfPrintf(&sDebugStream, "\n\rADXL345_CP: %d", pObj->i16Result);
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
