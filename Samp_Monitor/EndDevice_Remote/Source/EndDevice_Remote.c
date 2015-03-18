/****************************************************************************
 * (C) Tokyo Cosmos Electric, Inc. (TOCOS) - 2013 all rights reserved.
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
#include <string.h>

#include <jendefs.h>
#include <AppHardwareApi.h>

#include "AppQueueApi_ToCoNet.h"

#include "config.h"
#include "ccitt8.h"
#include "Interrupt.h"

#include "EndDevice_Remote.h"
#include "Version.h"

#include "utils.h"
#include "flash.h"

#include "common.h"

// Serial options
#include "serial.h"
#include "fprintf.h"
#include "sprintf.h"

#include "Interactive.h"
#include "sercmd_gen.h"

#ifdef USE_LCD
#include "LcdDriver.h"
#include "LcdDraw.h"
#include "LcdPrint.h"
#endif

/****************************************************************************/
/***        ToCoNet Definitions                                           ***/
/****************************************************************************/
// Select Modules (define befor include "ToCoNet.h")
#define ToCoNet_USE_MOD_NWK_LAYERTREE
#define ToCoNet_USE_MOD_NBSCAN // Neighbour scan module
#define ToCoNet_USE_MOD_CHANNEL_MGR
#define ToCoNet_USE_MOD_NWK_MESSAGE_POOL

// includes
#include "ToCoNet.h"
#include "ToCoNet_mod_prototype.h"

#include "app_event.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifdef USE_LCD
#define V_PRINTF_LCD(...) vfPrintf(&sLcdStream, __VA_ARGS__)
#define V_PRINTF_LCD_BTM(...) vfPrintf(&sLcdStreamBtm, __VA_ARGS__)
#endif

#define TOCONET_DEBUG_LEVEL 5

/**
 * メッセージプールの複数スロットのテストを行うコードを有効化する。
 * - 起動ごとにスロット番号を１ずつ上げてアクセスします。
 */
#undef MSGPL_SLOT_TEST

/**
 * スリープせずに動作させる
 */
#undef NO_SLEEP

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
//
static void vProcessEvCore(tsEvent *pEv, teEvent eEvent, uint32 u32evarg);
static void vInitHardware(int f_warm_start);

static void vSerialInit();
void vSerInitMessage();
void vProcessSerialCmd(tsSerCmd_Context *pCmd);

#ifdef USE_LCD
static void vLcdInit(void);
static void vLcdRefresh(void);
#endif

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
// Local data used by the tag during operation
tsAppData_Re sAppData_Re;

PUBLIC tsFILE sSerStream;
tsSerialPortSetup sSerPort;
//tsSerCmd sSerCmd; // serial command parser

// Timer object
tsTimerContext sTimerApp;

#ifdef USE_LCD
static tsFILE sLcdStream, sLcdStreamBtm;
#endif

/****************************************************************************/
/***        FUNCTIONS                                                     ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vProcessEvent
 *
 * DESCRIPTION:
 *   The Application Main State Machine.
 *
 * RETURNS:
 *
 ****************************************************************************/
//
static void vProcessEvCore(tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	switch (pEv->eState) {

	/*
	 * 起動時の状態
	 *   ToCoNetの接続を行う。
	 *   - スリープ復帰時は直前のアドレスに対して復帰をかける。失敗した場合は再探索を行う。
	 */
	case E_STATE_IDLE:
		// start up message
		V_PRINTF(LB "*** ToCoSamp IO Monitor %d.%02d-%d %s ***", VERSION_MAIN, VERSION_SUB, VERSION_VAR, sAppData.bWakeupByButton ? "BTN" : "OTR");
		V_PRINTF(LB "* App ID:%08x Long Addr:%08x Short Addr %04x",
				sToCoNet_AppContext.u32AppId, ToCoNet_u32GetSerial(), sToCoNet_AppContext.u16ShortAddress);

		SERIAL_vFlush(UART_PORT);

		if (eEvent == E_EVENT_START_UP) {
			if (IS_APPCONF_OPT_SECURE()) {
				bool_t bRes = bRegAesKey(sAppData.sFlash.sData.u32EncKey);
				V_PRINTF(LB "*** Register AES key (%d) ***", bRes);
			}
#ifdef USE_LCD
			vLcdInit(); // register sLcd

			// 最下行を表示する
			V_PRINTF_LCD_BTM("*** ToCoSamp IO Monitor %d.%02d-%d ***", VERSION_MAIN, VERSION_SUB, VERSION_VAR);
			vLcdRefresh();
#endif
		}

		if (eEvent == E_EVENT_START_UP && (u32evarg & EVARG_START_UP_WAKEUP_RAMHOLD_MASK)) {
			V_PRINTF(LB "* Warm starting woke by %s.", sAppData.bWakeupByButton ? "DIO" : "WakeTimer");

			/*
			 * 接続復帰（スリープ後）
			 */
			sAppData.eLedState = E_LED_WAIT;

			ToCoNet_Nwk_bResume(sAppData.pContextNwk); // ネットワークの復元

			ToCoNet_Event_SetState(pEv, E_STATE_APP_WAIT_NW_START);

		} else
		if (eEvent == E_EVENT_START_UP) {
			V_PRINTF(LB "* start end device[%d]", u32TickCount_ms & 0xFFFF);

			/*
			 * ネットワークの接続
			 */
			sAppData.sNwkLayerTreeConfig.u8Role = TOCONET_NWK_ROLE_ENDDEVICE; // EndDevice として構成する
			sAppData.sNwkLayerTreeConfig.u8ScanDur_10ms = 4; // 探索時間(中継機の数が数個程度なら 40ms 程度で十分)

			sAppData.pContextNwk = ToCoNet_NwkLyTr_psConfig(&sAppData.sNwkLayerTreeConfig); // 設定
			if (sAppData.pContextNwk) {
				ToCoNet_Nwk_bInit(sAppData.pContextNwk); // ネットワーク初期化
				ToCoNet_Nwk_bStart(sAppData.pContextNwk); // ネットワーク開始
			}

			sAppData.eLedState = E_LED_WAIT;
			ToCoNet_Event_SetState(pEv, E_STATE_APP_WAIT_NW_START);

			break;
		}

		break;

	/*
	 * ネットワークの開始待ち
	 */
	case E_STATE_APP_WAIT_NW_START:
		if (eEvent == E_EVENT_TOCONET_NWK_START) {
			ToCoNet_Event_SetState(pEv, E_STATE_RUNNING);
		}
		if (ToCoNet_Event_u32TickFrNewState(pEv) > 500) {
			ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP);
		}
		break;

	/*
	 * 実行時の状態
	 *   - ネットワーク開始後、メッセージプール要求を行う
	 *   - ネットワークがエラーになったり、タイムアウトが発生した場合はエラー状態とし、
	 *     再びスリープする
	 */
	case E_STATE_RUNNING:
		if (eEvent == E_EVENT_NEW_STATE) {
			// ネットワーク開始
			V_PRINTF(LB"[NWK STARTED and REQUEST IO STATE:%d]", u32TickCount_ms & 0xFFFF);
			static uint8 u8Ct;
#ifdef MSGPL_SLOT_TEST
			// メッセージプールの複数スロットのテストを行う
			// 起動ごとにスロット番号を変更して取得する
			u8Ct++;
#else
			u8Ct = 0;
#endif
			// メッセージプールの要求
			if (ToCoNet_MsgPl_bRequest(u8Ct % TOCONET_MOD_MESSAGE_POOL_MAX_ENTITY)) {
				ToCoNet_Event_SetState(pEv, E_STATE_APP_IO_WAIT_RX);
			} else {
				ToCoNet_Event_SetState(pEv, E_STATE_APP_IO_RECV_ERROR);
			}
		} else
		if (eEvent == E_EVENT_TOCONET_NWK_DISCONNECT) {
			ToCoNet_Event_SetState(pEv, E_STATE_APP_IO_RECV_ERROR);
		} else {
			if (ToCoNet_Event_u32TickFrNewState(pEv) > ENDD_TIMEOUT_CONNECT_ms) {
				ToCoNet_Event_SetState(pEv, E_STATE_APP_IO_RECV_ERROR);
			}
		}
		break;

	/*
	 * メッセージプールからの伝達待ち。
	 * - メッセージプールは cbToCoNet_vNwkEvent() に通知され、
	 *   その関数中でE_EVENT_APP_GET_IC_INFOイベントを発行する。
	 */
	case E_STATE_APP_IO_WAIT_RX:
		if (eEvent == E_EVENT_NEW_STATE) {
			V_PRINTF(LB"[E_STATE_APP_IO_WAIT_RX:%d]", u32TickCount_ms & 0xFFFF);
		} if (eEvent == E_EVENT_APP_GET_IC_INFO) {
			// メッセージが届いた
			if (u32evarg) {
				V_PRINTF(LB"[E_STATE_APP_IO_WAIT_RX:GOTDATA:%d]", u32TickCount_ms & 0xFFFF);
				ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP);
			} else {
				ToCoNet_Event_SetState(pEv, E_STATE_APP_IO_RECV_ERROR);
			}
		} else {
			// タイムアウト
			if (ToCoNet_Event_u32TickFrNewState(pEv) > ENDD_TIMEOUT_WAIT_MSG_ms) {
				V_PRINTF(LB"[E_STATE_APP_IO_WAIT_RX:TIMEOUT:%d]", u32TickCount_ms & 0xFFFF);
				ToCoNet_Event_SetState(pEv, E_STATE_APP_IO_RECV_ERROR);
			}
		}
		break;

	/*
	 * 送信完了の処理
	 * - スリープ実行
	 */
	case E_STATE_APP_IO_RECV_ERROR:
	case E_STATE_APP_SLEEP:
		if (eEvent == E_EVENT_NEW_STATE) {
			if (pEv->eState == E_STATE_APP_SLEEP) {
				V_PRINTF(LB"[E_STATE_APP_SLEEP_LED:%d]", u32TickCount_ms & 0xFFFF);
				sAppData.eLedState = E_LED_RESULT;
			} else {
				V_PRINTF(LB"[E_STATE_APP_IO_RECV_ERROR:%d]", u32TickCount_ms & 0xFFFF);
				sAppData.eLedState = E_LED_ERROR;
			}

			vPortSetHi(PORT_KIT_LED1);
			vPortSetHi(PORT_KIT_LED2);
#ifdef NO_SLEEP
			ToCoNet_Event_SetState(pEv, E_STATE_APP_PREUDO_SLEEP);
#else
			ToCoNet_Nwk_bPause(sAppData.pContextNwk);

			SERIAL_vFlush(UART_PORT);
			vSleep(0, FALSE, FALSE);
#endif
		}
		break;

	case E_STATE_APP_PREUDO_SLEEP:
		// 指定時間スリープ相当の時間待ちを行う
		if (ToCoNet_Event_u32TickFrNewState(pEv) > ENDD_SLEEP_PERIOD_s * 1000UL) {
			ToCoNet_Event_SetState(pEv, E_STATE_RUNNING);
		}
		break;

	default:
		break;
	}
}

/**
 * 設定処理用の状態マシン（未使用）
 */
static void vProcessEvCoreConfig(tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	switch (pEv->eState) {
		case E_STATE_IDLE:
			if (eEvent == E_EVENT_START_UP) {
				Interactive_vSetMode(TRUE, 0);
				vSerInitMessage();
				vfPrintf(&sSerStream, LB LB "*** Entering Config Mode ***");
				ToCoNet_Event_SetState(pEv, E_STATE_RUNNING);
			}
			break;

		case E_STATE_RUNNING:
			break;

		default:
			break;
	}
}


/****************************************************************************
 *
 * NAME: AppColdStart
 *
 * DESCRIPTION:
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void cbAppColdStart(bool_t bAfterAhiInit) {
	if (!bAfterAhiInit) {
		// before AHI initialization (very first of code)

		// check DIO source
		sAppData.bWakeupByButton = FALSE;
		if(u8AHI_WakeTimerFiredStatus()) {
		} else
    	if(u32AHI_DioWakeStatus() & u32DioPortWakeUp) {
			// woke up from DIO events
    		sAppData.bWakeupByButton = 1;
		}

		// Module Registration
		ToCoNet_REG_MOD_ALL();
	} else {
		// clear application context
		memset(&sAppData, 0x00, sizeof(sAppData));

		// SPRINTF
		SPRINTF_vInit128();

		// フラッシュメモリからの読み出し
		//   フラッシュからの読み込みが失敗した場合、ID=15 で設定する
		sAppData.bFlashLoaded = Config_bLoad(&sAppData.sFlash);

		// ToCoNet configuration
		sToCoNet_AppContext.u32AppId = sAppData.sFlash.sData.u32appid;
		sToCoNet_AppContext.u8Channel = sAppData.sFlash.sData.u8ch;
		sToCoNet_AppContext.u32ChMask = sAppData.sFlash.sData.u32chmask;

		sToCoNet_AppContext.u8TxMacRetry = 1;
		sToCoNet_AppContext.bRxOnIdle = TRUE;

		// Other Hardware
		vSerialInit();
		ToCoNet_vDebugInit(&sSerStream);
		ToCoNet_vDebugLevel(TOCONET_DEBUG_LEVEL);

		vInitHardware(FALSE);

		// event machine
		if (sAppData.bConfigMode) {
			ToCoNet_Event_Register_State_Machine(vProcessEvCoreConfig); // デバッグ用の動作マシン
		} else {
			ToCoNet_Event_Register_State_Machine(vProcessEvCore); // main state machine
		}
	}
}

/****************************************************************************
 *
 * NAME: AppWarmStart
 *
 * DESCRIPTION:
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void cbAppWarmStart(bool_t bAfterAhiInit) {
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
		// Other Hardware
		vSerialInit(TOCONET_DEBUG_LEVEL);
		ToCoNet_vDebugInit(&sSerStream);
		ToCoNet_vDebugLevel(TOCONET_DEBUG_LEVEL);

		vInitHardware(FALSE);

		if (!sAppData.bWakeupByButton) {
			// タイマーで起床した
		} else {
			// ボタンで起床した
		}

		// アプリケーション処理は vProcessEvCore で実行するので、ここでは
		// 処理は行っていない。
	}
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vMain
 *
 * DESCRIPTION:
 *
 * RETURNS:
 *
 ****************************************************************************/
//
void cbToCoNet_vMain(void) {
	/* handle serial input */
	vHandleSerialInput();
}

/****************************************************************************
 *
 * NAME: cbvMcRxHandler
 *
 * DESCRIPTION:
 *
 * RETURNS:
 *
 ****************************************************************************/
void cbToCoNet_vRxEvent(tsRxDataApp *pRx) {
	// uint8 *p = pRx->auData;
	return;
}

/****************************************************************************
 *
 * NAME: cbvMcEvTxHandler
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
void cbToCoNet_vTxEvent(uint8 u8CbId, uint8 bStatus) {
	return;
}

/****************************************************************************
 *
 * NAME: cbToCoNet_vNwkEvent
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
void cbToCoNet_vNwkEvent(teEvent eEvent, uint32 u32arg) {
	switch(eEvent) {
	/*
	 * ネットワークが開始された
	 */
	case E_EVENT_TOCONET_NWK_START:
		V_PRINTF(LB"[E_EVENT_TOCONET_NWK_START:%d,Ch:%d]", u32TickCount_ms & 0xFFFF, sToCoNet_AppContext.u8Channel);
		vDispInfo(&sSerStream, (void*)sAppData.pContextNwk);

		// pass this event to the event machine
		ToCoNet_Event_Process(E_EVENT_TOCONET_NWK_START, u32arg, vProcessEvCore);
		break;

	/*
	 * ネットワークが切断された
	 */
	case E_EVENT_TOCONET_NWK_DISCONNECT:
		V_PRINTF(LB"[E_EVENT_TOCONET_NWK_DISCONNECT]");

		// pass this event to the event machine
		ToCoNet_Event_Process(E_EVENT_TOCONET_NWK_DISCONNECT, u32arg, vProcessEvCore);
		break;

	/*
	 * メッセージプールを受信
	 */
	case E_EVENT_TOCONET_NWK_MESSAGE_POOL:
		if (u32arg) {
			tsToCoNet_MsgPl_Entity *pInfo = (void*)u32arg;
			int i;
			//static uint8 u8seq = 0;

			uint8 u8buff[TOCONET_MOD_MESSAGE_POOL_MAX_MESSAGE+1];
			memcpy(u8buff, pInfo->au8Message, pInfo->u8MessageLen); // u8Message にデータ u8MessageLen にデータ長
			u8buff[pInfo->u8MessageLen] = 0;

			// UART にメッセージ出力
			if (pInfo->bGotData) { // empty なら FALSE になる
				V_PRINTF(LB"---MSGPOOL sl=%d ln=%d msg=",
						pInfo->u8Slot,
						pInfo->u8MessageLen
						);

				SPRINTF_vRewind();
				for (i = 0; i < pInfo->u8MessageLen; i++) {
					vfPrintf(SPRINTF_Stream, "%02X", u8buff[i]);
				}
				V_PRINTF("%s", SPRINTF_pu8GetBuff());

#ifdef USE_LCD
				V_PRINTF_LCD("%02x:%s\r\n", u8seq++, SPRINTF_pu8GetBuff());
				vLcdRefresh();
#endif

				V_PRINTF("---");
			} else {
				V_PRINTF(LB"---MSGPOOL sl=%d EMPTY ---",
						pInfo->u8Slot
						);
#ifdef USE_LCD
				V_PRINTF_LCD("%02x: EMPTY\r\n", u8seq++);
				vLcdRefresh();
#endif
			}

			ToCoNet_Event_Process(E_EVENT_APP_GET_IC_INFO, pInfo->bGotData, vProcessEvCore); // vProcessEvCore にイベント送信
		}
		break;

	default:
		break;
	}
}

/****************************************************************************
 *
 * NAME: cbToCoNet_vHwEvent
 *
 * DESCRIPTION:
 * Process any hardware events.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u32DeviceId
 *                  u32ItemBitmap
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
void cbToCoNet_vHwEvent(uint32 u32DeviceId, uint32 u32ItemBitmap) {
	switch (u32DeviceId) {
	case E_AHI_DEVICE_ANALOGUE:
		break;

	case E_AHI_DEVICE_SYSCTRL:
		break;

	case E_AHI_DEVICE_TICK_TIMER:
		break;

	case E_AHI_DEVICE_TIMER0:
		break;

	default:
		break;
	}
}

/****************************************************************************
 *
 * NAME: cbToCoNet_u8HwInt
 *
 * DESCRIPTION:
 *   called during an interrupt
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u32DeviceId
 *                  u32ItemBitmap
 *
 * RETURNS:
 *                  FALSE -  interrupt is not handled, escalated to further
 *                           event call (cbToCoNet_vHwEvent).
 *                  TRUE  -  interrupt is handled, no further call.
 *
 * NOTES:
 *   Do not put a big job here.
 ****************************************************************************/
uint8 cbToCoNet_u8HwInt(uint32 u32DeviceId, uint32 u32ItemBitmap) {
	uint8 u8handled = FALSE;

	switch (u32DeviceId) {
	case E_AHI_DEVICE_ANALOGUE:
		break;

	case E_AHI_DEVICE_SYSCTRL:
		break;

	case E_AHI_DEVICE_TIMER0:
		break;

	case E_AHI_DEVICE_TICK_TIMER:
		break;

	default:
		break;
	}

	return u8handled;
}

/****************************************************************************
 *
 * NAME: vInitHardware
 *
 * DESCRIPTION:
 *
 * RETURNS:
 *
 ****************************************************************************/
static void vInitHardware(int f_warm_start) {
	// インタラクティブモードの初期化
	Interactive_vInit();

	// LED's
	vPortAsOutput(PORT_KIT_LED1);
	vPortAsOutput(PORT_KIT_LED2);
	vPortAsOutput(PORT_KIT_LED3);
	vPortAsOutput(PORT_KIT_LED4);
	vPortSetHi(PORT_KIT_LED1);
	vPortSetHi(PORT_KIT_LED2);
	vPortSetHi(PORT_KIT_LED3);
	vPortSetHi(PORT_KIT_LED4);

	if (!f_warm_start && bPortRead(PORT_CONF2)) {
		sAppData.bConfigMode = TRUE;
	}

	// activate tick timers
	memset(&sTimerApp, 0, sizeof(sTimerApp));
	sTimerApp.u8Device = E_AHI_DEVICE_TIMER0;
	sTimerApp.u16Hz = 1;
	sTimerApp.u8PreScale = 10;
	vTimerConfig(&sTimerApp);
	vTimerStart(&sTimerApp);
}

/****************************************************************************
 *
 * NAME: vSerialInit
 *
 * DESCRIPTION:
 *
 * RETURNS:
 *
 ****************************************************************************/
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
	V_PRINTF(LB "*** " APP_NAME " (EndDev Remote) %d.%02d-%d ***", VERSION_MAIN, VERSION_SUB, VERSION_VAR);
	V_PRINTF(LB "* App ID:%08x Long Addr:%08x Short Addr %04x LID %02d",
			sToCoNet_AppContext.u32AppId, ToCoNet_u32GetSerial(), sToCoNet_AppContext.u16ShortAddress,
			sAppData.sFlash.sData.u8id);
}

/**
 * コマンド受け取り時の処理
 * @param pCmd
 */
void vProcessSerialCmd(tsSerCmd_Context *pCmd) {
	V_PRINTF(LB "! cmd len=%d data=", pCmd->u16len);
	int i;
	for (i = 0; i < pCmd->u16len && i < 8; i++) {
		V_PRINTF("%02X", pCmd->au8data[i]);
	}
	if (i < pCmd->u16len) {
		V_PRINTF("...");
	}

	return;
}

#ifdef USE_LCD
/**
 * LCDの初期化
 */
static void vLcdInit(void) {
	/* Initisalise the LCD */
	vLcdReset(3, 0);

	/* register for vfPrintf() */
	sLcdStream.bPutChar = LCD_bTxChar;
	sLcdStream.u8Device = 0xFF;
	sLcdStreamBtm.bPutChar = LCD_bTxBottom;
	sLcdStreamBtm.u8Device = 0xFF;
}

/**
 * LCD を描画する
 */
static void vLcdRefresh(void) {
	vLcdClear();
	vDrawLcdDisplay(0, TRUE); /* write to lcd module */
	vLcdRefreshAll(); /* display new data */
}
#endif

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
