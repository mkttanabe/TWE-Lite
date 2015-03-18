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
#include <jendefs.h>
#include <string.h>
#include <AppHardwareApi.h>

#include "AppQueueApi_ToCoNet.h"

#include "config.h"
#include "ccitt8.h"
#include "Interrupt.h"

#include "Router.h"
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

/****************************************************************************/
/***        ToCoNet Definitions                                           ***/
/****************************************************************************/
// Select Modules (define befor include "ToCoNet.h")
#define ToCoNet_USE_MOD_NWK_LAYERTREE
#define ToCoNet_USE_MOD_NBSCAN // Neighbour scan module
#define ToCoNet_USE_MOD_NBSCAN_SLAVE
#define ToCoNet_USE_MOD_CHANNEL_MGR
#define ToCoNet_USE_MOD_NWK_MESSAGE_POOL
#define ToCoNet_USE_MOD_DUPCHK

// includes
#include "ToCoNet.h"
#include "ToCoNet_mod_prototype.h"

#include "app_event.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define TOCONET_DEBUG_LEVEL 0

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

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
// Local data used by the tag during operation
tsAppData_Ro sAppData_Ro;

tsFILE sSerStream;
tsSerialPortSetup sSerPort;
// tsSerCmd sSerCmd; // serial command parser

// Timer object
tsTimerContext sTimerApp;

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
static void vProcessEvCore(tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	switch (pEv->eState) {
	case E_STATE_IDLE:
		if (eEvent == E_EVENT_START_UP) {
			// フラッシュからの読み込みが成功すれば、その Layer を採用する
			sAppData.sNwkLayerTreeConfig.u8Layer = sAppData.sFlash.sData.u8layer;

			if (IS_APPCONF_OPT_SECURE()) {
				bool_t bRes = bRegAesKey(sAppData.sFlash.sData.u32EncKey);
				V_PRINTF(LB "*** Register AES key (%d) ***", bRes);
			}

			// Router として始動
			sAppData.sNwkLayerTreeConfig.u8Role = TOCONET_NWK_ROLE_ROUTER;
			sAppData.pContextNwk = ToCoNet_NwkLyTr_psConfig(&sAppData.sNwkLayerTreeConfig);
			if (sAppData.pContextNwk) {
				vfPrintf(&sSerStream, LB "* start router (layer %d)", sAppData.sFlash.sData.u8layer);
				ToCoNet_Nwk_bInit(sAppData.pContextNwk);
				ToCoNet_Nwk_bStart(sAppData.pContextNwk);
				ToCoNet_Event_SetState(pEv, E_STATE_RUNNING);
			} else {
				// fatal error
			}

			// start with verbose mode
			Interactive_vSetMode(TRUE, 0);
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
void cbAppColdStart(bool_t bAfterAhiInit) {
	if (!bAfterAhiInit) {
		// before AHI initialization (very first of code)

		// Module Registration
		ToCoNet_REG_MOD_ALL();
	} else {
		// clear application context
		memset(&sAppData, 0x00, sizeof(sAppData));

		// SPRINTF
		SPRINTF_vInit128();

		// Configuration
		// フラッシュメモリからの読み出し
		//   フラッシュからの読み込みが失敗した場合、ID=15 で設定する
		sAppData.bFlashLoaded = Config_bLoad(&sAppData.sFlash);

		// ToCoNet configuration
		sToCoNet_AppContext.u32AppId = sAppData.sFlash.sData.u32appid;
		sToCoNet_AppContext.u8Channel = sAppData.sFlash.sData.u8ch;
		sToCoNet_AppContext.u32ChMask = sAppData.sFlash.sData.u32chmask;

		sToCoNet_AppContext.u8TxMacRetry = 1;
		sToCoNet_AppContext.bRxOnIdle = TRUE;

		// event machine
		ToCoNet_Event_Register_State_Machine(vProcessEvCore); // main state machine

		// Other Hardware
		vSerialInit();
		ToCoNet_vDebugInit(&sSerStream);
		ToCoNet_vDebugLevel(TOCONET_DEBUG_LEVEL);

		vInitHardware(FALSE);
		Interactive_vInit();

		// START UP MESSAGE
		vfPrintf(&sSerStream, "\r\n*** ToCoTemp Parent %d.%02d-%d ***",
				VERSION_MAIN, VERSION_SUB, VERSION_VAR);
		vfPrintf(&sSerStream, LB "* App ID:%08x Long Addr:%08x Short Addr %04x",
				sToCoNet_AppContext.u32AppId, ToCoNet_u32GetSerial(),
				sToCoNet_AppContext.u16ShortAddress);
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
void cbAppWarmStart(bool_t bAfterAhiInit) {
	cbAppColdStart(bAfterAhiInit);
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
PUBLIC void cbToCoNet_vRxEvent(tsRxDataApp *pRx) {
	int i;

	// print coming payload
	V_PRINTF(
			LB "[PKT Ad:%04x,Ln:%03d,Seq:%03d,Lq:%03d,Tms:%05d %s\"",
			pRx->u32SrcAddr, pRx->u8Len, // Actual payload byte: the network layer uses additional 4 bytes.
			pRx->u8Seq, pRx->u8Lqi, pRx->u32Tick & 0xFFFF, pRx->bSecurePkt ? "Enc " : "");

	for (i = 0; i < pRx->u8Len; i++) {
		if (i < 32) {
			V_PUTCHAR((pRx->auData[i] >= 0x20 && pRx->auData[i] <= 0x7f) ?
							pRx->auData[i] : '.');
		} else {
			V_PRINTF( "..");
			break;
		}
	}
	V_PRINTF( "\"]");

	// 暗号化対応時に平文パケットは受信しない
	if (IS_APPCONF_OPT_SECURE()) {
		if (!pRx->bSecurePkt) {
			V_PRINTF( ".. skipped plain packet.");
			return;
		}
	}

	// 直接受信したパケットを上位へ転送する
	//
	// 直接親機宛(TOCONET_NWK_ADDR_PARENT指定で送信)に向けたパケットはここでは処理されない。
	// 本処理はアドレス指定がTOCONET_NWK_ADDR_NEIGHBOUR_ABOVEの場合で、一端中継機が受け取り
	// その中継機のアドレス、受信時のLQIを含めて親機に伝達する方式である。
	if (pRx->auData[0] == 'T') {
		tsTxDataApp sTx;
		memset(&sTx, 0, sizeof(sTx));
		uint8 *q = sTx.auData;

		S_OCTET('R'); // 1バイト目に中継機フラグを立てる
		S_BE_DWORD(pRx->u32SrcAddr); // 子機のアドレスを
		S_OCTET(pRx->u8Lqi); // 受信したLQI を保存する

		memcpy(sTx.auData + 6, pRx->auData + 1, pRx->u8Len - 1); // 先頭の１バイトを除いて５バイト先にコピーする
		q += pRx->u8Len - 1;

		sTx.u8Len = q - sTx.auData;

		sTx.u32DstAddr = TOCONET_NWK_ADDR_PARENT;
		sTx.u32SrcAddr = ToCoNet_u32GetSerial(); // Transmit using Long address
		sTx.u8Cmd = 0; // data packet.

		sTx.u8Seq = pRx->u8Seq;
		sTx.u8CbId = pRx->u8Seq;

		sTx.u16DelayMax = 300; // 送信開始の遅延を大きめに設定する


		if (IS_APPCONF_OPT_SECURE()) {
			sTx.bSecurePacket = TRUE;
		}

		ToCoNet_Nwk_bTx(sAppData.pContextNwk, &sTx);
	}
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
PUBLIC void cbToCoNet_vTxEvent(uint8 u8CbId, uint8 bStatus) {
	V_PRINTF( LB ">>> MacAck%s(tick=%d,req=#%d) <<<",
			bStatus ? "Ok" : "Ng",
			u32TickCount_ms & 0xFFFF,
			u8CbId
			);

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
	case E_EVENT_TOCONET_NWK_START:
		A_PRINTF( LB"[E_EVENT_TOCONET_NWK_START]");
		break;

	case E_EVENT_TOCONET_NWK_DISCONNECT:
		A_PRINTF( LB"[E_EVENT_TOCONET_NWK_DISCONNECT]");
		break;

	case E_EVENT_TOCONET_NWK_ROUTE_PKT:
		if (u32arg) {
			tsRoutePktInfo *pInfo = (void*)u32arg;

			if (pInfo->bUpstream) {
				sAppData.u32LedCt = u32TickCount_ms;
			}
		}
		break;

	case E_EVENT_TOCONET_NWK_MESSAGE_POOL_REQUEST:
		sAppData.u32LedCt = u32TickCount_ms;
		break;

	case E_EVENT_TOCONET_NWK_MESSAGE_POOL:
		if (u32arg) {
			tsToCoNet_MsgPl_Entity *pInfo = (void*)u32arg;
			int i;

			uint8 u8buff[TOCONET_MOD_MESSAGE_POOL_MAX_MESSAGE+1];
			memcpy(u8buff, pInfo->au8Message, pInfo->u8MessageLen);
			u8buff[pInfo->u8MessageLen] = 0;

			A_PRINTF( "[MSGPOOL sl=%d ln=%d msg=",
					pInfo->u8Slot,
					pInfo->u8MessageLen
					);

			for (i = 0; i < pInfo->u8MessageLen; i++) {
				A_PRINTF("%02X", u8buff[i]);
			}

			A_PRINTF("]"LB);
		}
		break;

	case E_EVENT_TOCONET_PANIC:
		if (u32arg) {
			tsPanicEventInfo *pInfo = (void*)u32arg;
			V_PRINTF( "PANIC DETECTED!");

			pInfo->bCancelReset = TRUE;
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
//
PUBLIC void cbToCoNet_vHwEvent(uint32 u32DeviceId, uint32 u32ItemBitmap) {
	switch (u32DeviceId) {
	case E_AHI_DEVICE_ANALOGUE:
		break;

	case E_AHI_DEVICE_SYSCTRL:
		break;

	case E_AHI_DEVICE_TICK_TIMER:
		// LED BLINK
		vPortSet_TrueAsLo(PORT_KIT_LED2, u32TickCount_ms & 0x400);

		// LED ON when receive
		if (u32TickCount_ms - sAppData.u32LedCt < 300) {
			vPortSetLo(PORT_KIT_LED1);
		} else {
			vPortSetHi(PORT_KIT_LED1);
		}
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
//
PUBLIC uint8 cbToCoNet_u8HwInt(uint32 u32DeviceId, uint32 u32ItemBitmap) {
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
//
PRIVATE void vInitHardware(int f_warm_start) {
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
	V_PRINTF(LB "*** " APP_NAME " (Router) %d.%02d-%d ***", VERSION_MAIN, VERSION_SUB, VERSION_VAR);
	V_PRINTF(LB "* App ID:%08x Long Addr:%08x Short Addr %04x LID %02d",
			sToCoNet_AppContext.u32AppId, ToCoNet_u32GetSerial(), sToCoNet_AppContext.u16ShortAddress,
			sAppData.sFlash.sData.u8id);
}

/**
 * コマンド受け取り時の処理
 * @param pCmd
 */
void vProcessSerialCmd(tsSerCmd_Context *pCmd) {
	return;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
