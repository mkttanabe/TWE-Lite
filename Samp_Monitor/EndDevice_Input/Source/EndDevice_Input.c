/****************************************************************************
 * (C) Tokyo Cosmos Electric, Inc. (TOCOS) - all rights reserved.
 *
 * Condition to use: (refer to detailed conditions in Japanese)
 *   - The full or part of source code is limited to use for TWE (TOCOS
 *     Wireless Engine) as compiled and flash programmed.
 *   - The full or part of source code is prohibited to distribute without
 *     permission from TOCOS.
 *
 * 利用条件:
 *   - 本ソースコードは、別途ソースコードライセンス記述が無い限り東京コスモス電機が著作権を
 *     保有しています。
 *   - 本ソースコードは、無保証・無サポートです。本ソースコードや生成物を用いたいかなる損害
 *     についても東京コスモス電機は保証致しません。不具合等の報告は歓迎いたします。
 *   - 本ソースコードは、東京コスモス電機が販売する TWE シリーズ上で実行する前提で公開
 *     しています。他のマイコン等への移植・流用は一部であっても出来ません。
 *
 ****************************************************************************/

/**
 * 概要
 *
 * 本パートは、IO状態を取得し、ブロードキャストにて、これをネットワーク層に
 * 送付する。
 *
 */

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <string.h>

#include <jendefs.h>
#include <AppHardwareApi.h>

#include "AppQueueApi_ToCoNet.h"

#include "config.h"
#include "ccitt8.h"
#include "Interrupt.h"

#include "EndDevice_Input.h"
#include "Version.h"

#include "utils.h"

#include "config.h"
#include "common.h"

#include "adc.h"
#include "SMBus.h"

// Serial options
#include "serial.h"
#include "fprintf.h"
#include "sprintf.h"

#include "input_string.h"
#include "Interactive.h"
#include "flash.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/**
 * DI1 の割り込み（立ち上がり）で起床後
 *   - PWM1 に DUTY50% で 100ms のブザー制御
 *
 * 以降１秒置きに起床して、DI1 が Hi (スイッチ開) かどうかチェックし、
 * Lo になったら、割り込みスリープに遷移、Hi が維持されていた場合は、
 * 一定期間 .u16Slp 経過後にブザー制御を 100ms 実施する。
 */
tsTimerContext sTimerPWM[1]; //!< タイマー管理構造体  @ingroup MASTER

/**
 * アプリケーションごとの振る舞いを記述するための関数テーブル
 */
tsCbHandler *psCbHandler = NULL;

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static void vInitHardware(int f_warm_start);
static void vInitPulseCounter();
static void vInitADC();

static void vSerialInit();
void vSerInitMessage();
void vProcessSerialCmd(tsSerCmd_Context *pCmd);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
// Local data used by the tag during operation
tsAppData_Ed sAppData_Ed;

tsFILE sSerStream;
tsSerialPortSetup sSerPort;

void *pvProcessEv1, *pvProcessEv2;
void (*pf_cbProcessSerialCmd)(tsSerCmd_Context *);

/****************************************************************************/
/***        Functions                                                     ***/
/****************************************************************************/

/**
 * 始動時の処理
 */
void cbAppColdStart(bool_t bAfterAhiInit) {
	if (!bAfterAhiInit) {
		// before AHI initialization (very first of code)
		pvProcessEv1 = NULL;
		pvProcessEv2 = NULL;
		pf_cbProcessSerialCmd = NULL;

		// check DIO source
		sAppData.bWakeupByButton = FALSE;
		if(u8AHI_WakeTimerFiredStatus()) {
		} else
    	if(u32AHI_DioWakeStatus() & u32DioPortWakeUp) {
			// woke up from DIO events
    		sAppData.bWakeupByButton = TRUE;
		}

		// Module Registration
		ToCoNet_REG_MOD_ALL();
	} else {
		// disable brown out detect
		vAHI_BrownOutConfigure(0,//0:2.0V 1:2.3V
				FALSE,
				FALSE,
				FALSE,
				FALSE);

		// リセットICの無効化
		vPortSetLo(DIO_VOLTAGE_CHECKER);
		vPortAsOutput(DIO_VOLTAGE_CHECKER);
		vPortDisablePullup(DIO_VOLTAGE_CHECKER);

		// １次キャパシタ(e.g. 220uF)とスーパーキャパシタ (1F) の直結制御用
		vPortSetHi(DIO_SUPERCAP_CONTROL);
		vPortAsOutput(DIO_SUPERCAP_CONTROL);
		vPortDisablePullup(DIO_SUPERCAP_CONTROL);

		//	入力ボタンのプルアップを停止する
		if ((sAppData.sFlash.sData.u8mode == PKT_ID_IO_TIMER)	// ドアタイマー
			|| (sAppData.sFlash.sData.u8mode == PKT_ID_BOTTON && sAppData.sFlash.sData.i16param == 1 ) ) {	// 押しボタンの立ち上がり検出時
			vPortDisablePullup(DIO_BUTTON); // 外部プルアップのため
		}

		// アプリケーション保持構造体の初期化
		memset(&sAppData, 0x00, sizeof(sAppData));

		// SPRINTFの初期化(128バイトのバッファを確保する)
		SPRINTF_vInit128();

		// フラッシュメモリからの読み出し
		//   フラッシュからの読み込みが失敗した場合、ID=15 で設定する
		sAppData.bFlashLoaded = Config_bLoad(&sAppData.sFlash);

		// センサー用の制御 (Lo:Active), OPTION による制御を行っているのでフラッシュ読み込み後の制御が必要
		vPortSetSns(TRUE);
		vPortAsOutput(DIO_SNS_POWER);
		vPortDisablePullup(DIO_SNS_POWER);

		// configure network
		sToCoNet_AppContext.u32AppId = sAppData.sFlash.sData.u32appid;
		sToCoNet_AppContext.u8Channel = sAppData.sFlash.sData.u8ch;

		//sToCoNet_AppContext.u8TxMacRetry = 1;
		sToCoNet_AppContext.bRxOnIdle = FALSE;

		sToCoNet_AppContext.u8CCA_Level = 1;
		sToCoNet_AppContext.u8CCA_Retry = 0;

		sToCoNet_AppContext.u8TxPower = sAppData.sFlash.sData.u8pow;

		// version info
		sAppData.u32ToCoNetVersion = ToCoNet_u32GetVersion();

		// M2がLoなら、設定モードとして動作する
		vPortAsInput(PORT_CONF2);
		if (bPortRead(PORT_CONF2)) {
			sAppData.bConfigMode = TRUE;
		}

		if (sAppData.bConfigMode) {
			// 設定モードで起動

			// Other Hardware
			vInitHardware(FALSE);

			// イベント処理の初期化
			vInitAppConfig();

			// インタラクティブモードの初期化
			Interactive_vInit();
		} else
		//	ボタン起動モード
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_BOTTON ) {
			sToCoNet_AppContext.u8MacInitPending = TRUE; // 起動時の MAC 初期化を省略する(送信する時に初期化する)
			sToCoNet_AppContext.bSkipBootCalib = TRUE; // 起動時のキャリブレーションを省略する(保存した値を確認)

			// Other Hardware
			vInitHardware(FALSE);

			// ADC の初期化
			vInitADC();

			// イベント処理の初期化
			vInitAppBotton();
		} else
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_UART ) {
			sToCoNet_AppContext.bSkipBootCalib = TRUE; // 起動時のキャリブレーションを省略する(保存した値を確認)

			// Other Hardware
			vInitHardware(FALSE);

			// UART 処理
			vInitAppUart();

			// インタラクティブモード
			Interactive_vInit();
		} else
		//	磁気スイッチなど
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_IO_TIMER ) {
			// ドアタイマーで起動
			// sToCoNet_AppContext.u8CPUClk = 1; // runs at 8MHz (Doze を利用するのであまり効果が無いかもしれない)
			sToCoNet_AppContext.bSkipBootCalib = FALSE; // 起動時のキャリブレーションを行う
			sToCoNet_AppContext.u8MacInitPending = TRUE; // 起動時の MAC 初期化を省略する(送信する時に初期化する)

			// Other Hardware
			vInitHardware(FALSE);

			// イベント処理の初期化
			vInitAppDoorTimer();
		} else
		// SHT21
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_SHT21 ) {
			sToCoNet_AppContext.bSkipBootCalib = FALSE; // 起動時のキャリブレーションを行う
			sToCoNet_AppContext.u8MacInitPending = TRUE; // 起動時の MAC 初期化を省略する(送信する時に初期化する)

			// ADC の初期化
			vInitADC();

			// Other Hardware
			vInitHardware(FALSE);

			// イベント処理の初期化
			vInitAppSHT21();
		} else
		// ADT7410
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_ADT7410 ) {
			sToCoNet_AppContext.bSkipBootCalib = FALSE; // 起動時のキャリブレーションを行う
			sToCoNet_AppContext.u8MacInitPending = TRUE; // 起動時の MAC 初期化を省略する(送信する時に初期化する)

			// ADC の初期化
			vInitADC();

			// Other Hardware
			vInitHardware(FALSE);

			// イベント処理の初期化
			vInitAppADT7410();
		} else
		// MPL115A2
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_MPL115A2 ) {
			sToCoNet_AppContext.bSkipBootCalib = FALSE; // 起動時のキャリブレーションを行う
			sToCoNet_AppContext.u8MacInitPending = TRUE; // 起動時の MAC 初期化を省略する(送信する時に初期化する)

			// ADC の初期化
			vInitADC();

			// Other Hardware
			vInitHardware(FALSE);

			// イベント処理の初期化
			vInitAppMPL115A2();
		} else
		// LIS3DH
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_LIS3DH ) {
			sToCoNet_AppContext.bSkipBootCalib = FALSE; // 起動時のキャリブレーションを行う
			sToCoNet_AppContext.u8MacInitPending = TRUE; // 起動時の MAC 初期化を省略する(送信する時に初期化する)

			// ADC の初期化
			vInitADC();

			// Other Hardware
			vInitHardware(FALSE);

			// イベント処理の初期化
			vInitAppLIS3DH();
		} else
		//	LM61等のアナログセンサ用
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_STANDARD	// アナログセンサ
			|| sAppData.sFlash.sData.u8mode == PKT_ID_LM61) {	// LM61
			// 通常アプリで起動
			sToCoNet_AppContext.u8CPUClk = 3; // runs at 32Mhz
			sToCoNet_AppContext.u8MacInitPending = TRUE; // 起動時の MAC 初期化を省略する(送信する時に初期化する)
			sToCoNet_AppContext.bSkipBootCalib = TRUE; // 起動時のキャリブレーションを省略する(保存した値を確認)

			// ADC の初期化
			vInitADC();

			// Other Hardware
			vInitHardware(FALSE);
			vInitPulseCounter();

			// イベント処理の初期化
			vInitAppStandard();
		}

		// イベント処理関数の登録
		if (pvProcessEv1) {
			ToCoNet_Event_Register_State_Machine(pvProcessEv1);
		}
		if (pvProcessEv2) {
			ToCoNet_Event_Register_State_Machine(pvProcessEv2);
		}

		// ToCoNet DEBUG
		ToCoNet_vDebugInit(&sSerStream);
		ToCoNet_vDebugLevel(TOCONET_DEBUG_LEVEL);
	}
}

/**
 * スリープ復帰時の処理
 * @param bAfterAhiInit
 */
void cbAppWarmStart(bool_t bAfterAhiInit) {
	if (!bAfterAhiInit) {
		// before AHI init, very first of code.
		//  to check interrupt source, etc.

		sAppData.bWakeupByButton = FALSE;
		if(u8AHI_WakeTimerFiredStatus()) {
		} else
		if(u32AHI_DioWakeStatus() & u32DioPortWakeUp) {
			// woke up from DIO events
			sAppData.bWakeupByButton = TRUE;
		}
	} else {
		// disable brown out detect
		vAHI_BrownOutConfigure(0,//0:2.0V 1:2.3V
				FALSE,
				FALSE,
				FALSE,
				FALSE);

		// センサー用の制御 (Lo:Active)
		vPortSetSns(TRUE);
		vPortAsOutput(DIO_SNS_POWER);

		// Other Hardware
		Interactive_vReInit();
		vSerialInit();

		// TOCONET DEBUG
		ToCoNet_vDebugInit(&sSerStream);
		ToCoNet_vDebugLevel(TOCONET_DEBUG_LEVEL);

		// センサ特有の初期化
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_BOTTON ) {
			// ADC の初期化
			vInitADC();
		} else
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_UART ) {
		} else
		//	磁気スイッチなど
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_IO_TIMER ) {
		} else
		// SHT21
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_SHT21 ) {
			// ADC の初期化
			vInitADC();
		} else
		// ADT7410
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_ADT7410 ) {
			// ADC の初期化
			vInitADC();
		} else
		// MPL115A2
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_MPL115A2 ) {
			// ADC の初期化
			vInitADC();
		} else
		// LIS3DH
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_LIS3DH ) {
			// ADC の初期化
			vInitADC();
		} else
		//	LM61等のアナログセンサ用
		if ( sAppData.sFlash.sData.u8mode == PKT_ID_STANDARD	// アナログセンサ
			|| sAppData.sFlash.sData.u8mode == PKT_ID_LM61) {	// LM61
			// ADC の初期化
			vInitADC();
		}

		// 他のハードの待ち
		vInitHardware(TRUE);

		if (!sAppData.bWakeupByButton) {
			// タイマーで起きた
		} else {
			// ボタンで起床した
		}
	}
}

/**
 * メイン処理
 */
void cbToCoNet_vMain(void) {
	if (psCbHandler && psCbHandler->pf_cbToCoNet_vMain) {
		(*psCbHandler->pf_cbToCoNet_vMain)();
	}
}

/**
 * 受信処理
 */
void cbToCoNet_vRxEvent(tsRxDataApp *pRx) {
	if (psCbHandler && psCbHandler->pf_cbToCoNet_vRxEvent) {
		(*psCbHandler->pf_cbToCoNet_vRxEvent)(pRx);
	}
}

/**
 * 送信完了イベント
 */
void cbToCoNet_vTxEvent(uint8 u8CbId, uint8 bStatus) {
	V_PRINTF(LB ">>> TxCmp %s(tick=%d,req=#%d) <<<",
			bStatus ? "Ok" : "Ng",
			u32TickCount_ms & 0xFFFF,
			u8CbId
			);

	if (psCbHandler && psCbHandler->pf_cbToCoNet_vTxEvent) {
		(*psCbHandler->pf_cbToCoNet_vTxEvent)(u8CbId, bStatus);
	}

	return;
}

/**
 * ネットワークイベント
 * @param eEvent
 * @param u32arg
 */
void cbToCoNet_vNwkEvent(teEvent eEvent, uint32 u32arg) {
	if (psCbHandler && psCbHandler->pf_cbToCoNet_vNwkEvent) {
		(*psCbHandler->pf_cbToCoNet_vNwkEvent)(eEvent, u32arg);
	}
}

/**
 * ハードウェアイベント処理（割り込み遅延実行）
 */
void cbToCoNet_vHwEvent(uint32 u32DeviceId, uint32 u32ItemBitmap) {
	if (psCbHandler && psCbHandler->pf_cbToCoNet_vHwEvent) {
		(*psCbHandler->pf_cbToCoNet_vHwEvent)(u32DeviceId, u32ItemBitmap);
	}
}

/**
 * ハードウェア割り込みハンドラ
 */
uint8 cbToCoNet_u8HwInt(uint32 u32DeviceId, uint32 u32ItemBitmap) {
	bool_t bRet = FALSE;
	if (psCbHandler && psCbHandler->pf_cbToCoNet_u8HwInt) {
		bRet = (*psCbHandler->pf_cbToCoNet_u8HwInt)(u32DeviceId, u32ItemBitmap);
	}
	return bRet;
}

/**
 * ADCの初期化
 */
static void vInitADC() {
	// ADC
	vADC_Init(&sAppData.sObjADC, &sAppData.sADC, TRUE);
	sAppData.u8AdcState = 0xFF; // 初期化中

#ifdef USE_TEMP_INSTDOF_ADC2
	sAppData.sObjADC.u8SourceMask =
			TEH_ADC_SRC_VOLT | TEH_ADC_SRC_ADC_1 | TEH_ADC_SRC_TEMP;
#else
	sAppData.sObjADC.u8SourceMask =
			TEH_ADC_SRC_VOLT | TEH_ADC_SRC_ADC_1 | TEH_ADC_SRC_ADC_2;
#endif
}

/**
 * パルスカウンタの初期化
 * - cold boot 時に1回だけ初期化する
 */
static void vInitPulseCounter() {
	// カウンタの設定
	bAHI_PulseCounterConfigure(
		E_AHI_PC_0,
		1,      // 0:RISE, 1:FALL EDGE
		0,      // Debounce 0:off, 1:2samples, 2:4samples, 3:8samples
		FALSE,   // Combined Counter (32bitカウンタ)
		FALSE);  // Interrupt (割り込み)

	// カウンタのセット
	bAHI_SetPulseCounterRef(
		E_AHI_PC_0,
		0x0); // 何か事前に値を入れておく

	// カウンタのスタート
	bAHI_StartPulseCounter(E_AHI_PC_0); // start it

	// カウンタの設定
	bAHI_PulseCounterConfigure(
		E_AHI_PC_1,
		1,      // 0:RISE, 1:FALL EDGE
		0,      // Debounce 0:off, 1:2samples, 2:4samples, 3:8samples
		FALSE,   // Combined Counter (32bitカウンタ)
		FALSE);  // Interrupt (割り込み)

	// カウンタのセット
	bAHI_SetPulseCounterRef(
		E_AHI_PC_1,
		0x0); // 何か事前に値を入れておく

	// カウンタのスタート
	bAHI_StartPulseCounter(E_AHI_PC_1); // start it
}

/**
 * ハードウェアの初期化を行う
 * @param f_warm_start TRUE:スリープ起床時
 */
static void vInitHardware(int f_warm_start) {
	// 入力ポートを明示的に指定する
	vPortAsInput(DIO_BUTTON);

	if( sAppData.sFlash.sData.u8mode == PKT_ID_LIS3DH &&
			sAppData.sFlash.sData.i16param == 1
	){
		vPortAsInput(PORT_INPUT2);
	}

	// Serial Port の初期化
	vSerialInit();

	// PWM の初期化
	if ( sAppData.sFlash.sData.u8mode == PKT_ID_IO_TIMER ) {
# ifndef JN516x
#  warning "IO_TIMER is not implemented on JN514x"
# endif
# ifdef JN516x
		memset(&sTimerPWM[0], 0, sizeof(tsTimerContext));
		vAHI_TimerFineGrainDIOControl(0x7); // bit 0,1,2 をセット (TIMER0 の各ピンを解放する, PWM1..4 は使用する)
		vAHI_TimerSetLocation(E_AHI_TIMER_1, TRUE, TRUE); // IOの割り当てを設定

		// PWM
		int i;
		for (i = 0; i < 1; i++) {
			const uint8 au8TimTbl[] = {
				E_AHI_DEVICE_TIMER1,
				//E_AHI_DEVICE_TIMER2
				//E_AHI_DEVICE_TIMER3,
				//E_AHI_DEVICE_TIMER4
			};
			sTimerPWM[i].u16Hz = 1000;
			sTimerPWM[i].u8PreScale = 0;
			sTimerPWM[i].u16duty = 1024; // 1024=Hi, 0:Lo
			sTimerPWM[i].bPWMout = TRUE;
			sTimerPWM[i].bDisableInt = TRUE; // 割り込みを禁止する指定
			sTimerPWM[i].u8Device = au8TimTbl[i];
			vTimerConfig(&sTimerPWM[i]);
			vTimerStart(&sTimerPWM[i]);
		}
# endif
	}

	// SMBUS の初期化

//	if (IS_APPCONF_OPT_SHT21()) {
	if ( sAppData.sFlash.sData.u8mode ==  PKT_ID_SHT21 ||
		 sAppData.sFlash.sData.u8mode ==  PKT_ID_ADT7410 ||
		 sAppData.sFlash.sData.u8mode ==  PKT_ID_LIS3DH ||
		 sAppData.sFlash.sData.u8mode ==  PKT_ID_MPL115A2	) {
		vSMBusInit();
	}
}

/**
 * シリアルポートの初期化
 */
void vSerialInit(void) {
	/* Create the debug port transmit and receive queues */
	static uint8 au8SerialTxBuffer[1024];
	static uint8 au8SerialRxBuffer[512];

	/* Initialise the serial port to be used for debug output */
	sSerPort.pu8SerialRxQueueBuffer = au8SerialRxBuffer;
	sSerPort.pu8SerialTxQueueBuffer = au8SerialTxBuffer;
	sSerPort.u32BaudRate = UART_BAUD;
	sSerPort.u16AHI_UART_RTS_LOW = 0xffff;
	sSerPort.u16AHI_UART_RTS_HIGH = 0xffff;
	sSerPort.u16SerialRxQueueSize = sizeof(au8SerialRxBuffer);
	sSerPort.u16SerialTxQueueSize = sizeof(au8SerialTxBuffer);
	sSerPort.u8SerialPort = UART_PORT;
	sSerPort.u8RX_FIFO_LEVEL = E_AHI_UART_FIFO_LEVEL_1;
	SERIAL_vInit(&sSerPort);

	sSerStream.bPutChar = SERIAL_bTxChar;
	sSerStream.u8Device = UART_PORT;
}

/**
 * 初期化メッセージ
 */
void vSerInitMessage() {
	V_PRINTF(LB LB "*** " APP_NAME " (ED_Inp) %d.%02d-%d ***", VERSION_MAIN, VERSION_SUB, VERSION_VAR);
	V_PRINTF(LB "* App ID:%08x Long Addr:%08x Short Addr %04x LID %02d Calib=%d",
			sToCoNet_AppContext.u32AppId, ToCoNet_u32GetSerial(), sToCoNet_AppContext.u16ShortAddress,
			sAppData.sFlash.sData.u8id,
			sAppData.sFlash.sData.u16RcClock);
}

/**
 * コマンド受け取り時の処理
 * @param pCmd
 */
void vProcessSerialCmd(tsSerCmd_Context *pCmd) {
	// アプリのコールバックを呼び出す
	if (pf_cbProcessSerialCmd) {
		(*pf_cbProcessSerialCmd)(pCmd);
	}

#if 0
	V_PRINTF(LB "! cmd len=%d data=", pCmd->u16len);
	int i;
	for (i = 0; i < pCmd->u16len && i < 8; i++) {
		V_PRINTF("%02X", pCmd->au8data[i]);
	}
	if (i < pCmd->u16len) {
		V_PRINTF("...");
	}
#endif

	return;
}

/**
 * センサーアクティブ時のポートの制御を行う
 *
 * @param bActive ACTIVE時がTRUE
 */
void vPortSetSns(bool_t bActive) {
	if (IS_APPCONF_OPT_INVERSE_SNS_ACTIVE()) {
		bActive = !bActive;
	}

	vPortSet_TrueAsLo(DIO_SNS_POWER, bActive);
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
