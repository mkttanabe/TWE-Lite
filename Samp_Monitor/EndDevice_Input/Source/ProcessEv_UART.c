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

#include <jendefs.h>

#include "utils.h"

#include "Interactive.h"
#include "EndDevice_Input.h"

#include "sercmd_gen.h"

tsSerCmd_Context sSerCmdOut; //!< シリアル出力用
static tsTxDataApp sTx_Retry; //!< 再送用
static bool_t bRetry;
static uint8 u8RetryCount = 0xff;
static uint16 u16RetryDur = 0;

static void vProcessEvCore(tsEvent *pEv, teEvent eEvent, uint32 u32evarg);

#define PORT_RTS0 (5)

/*
 * ADC 計測をしてデータ送信するアプリケーション制御
 */

/**
 * 始動時の処理
 *
 * @param E_STATE_IDLE
 * @param pEv
 * @param eEvent
 * @param u32evarg
 */
PRSEV_HANDLER_DEF(E_STATE_IDLE, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	if (eEvent == E_EVENT_START_UP) {
		// 起動メッセージ
		vSerInitMessage();

		// 暗号化鍵の登録
		if (IS_APPCONF_OPT_SECURE()) {
			bool_t bRes = bRegAesKey(sAppData.sFlash.sData.u32EncKey);
			V_PRINTF(LB "*** Register AES key (%d) ***", bRes);
		}

		if (u32evarg & EVARG_START_UP_WAKEUP_RAMHOLD_MASK) {
			// Warm start message
			V_PRINTF(LB "*** Warm starting woke by %s. ***", sAppData.bWakeupByButton ? "DIO" : "WakeTimer");

			ToCoNet_Nwk_bResume(sAppData.pContextNwk);
			ToCoNet_Event_SetState(pEv, E_STATE_RUNNING);

			// RTS を設定
			vPortSetLo(PORT_RTS0);
		} else {
			// 開始する
			// start up message
			V_PRINTF(LB "*** Cold starting(UART)");
			V_PRINTF(LB "* start end device[%d]", u32TickCount_ms & 0xFFFF);

			sAppData.sNwkLayerTreeConfig.u8Role = TOCONET_NWK_ROLE_ENDDEVICE;
			// ネットワークの初期化
			sAppData.pContextNwk = ToCoNet_NwkLyTr_psConfig_MiniNodes(&sAppData.sNwkLayerTreeConfig);

			if (sAppData.pContextNwk) {
				// とりあえず初期化だけしておく
				ToCoNet_Nwk_bInit(sAppData.pContextNwk);
				ToCoNet_Nwk_bStart(sAppData.pContextNwk);
			}
			// 直ぐにスリープ
			ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP);

			// RTS を設定
			vPortSetHi(PORT_RTS0);

			// 再送フラグ
			bRetry = ((sAppData.sFlash.sData.u8wait % 10) != 0);
			u16RetryDur = sAppData.sFlash.sData.u8wait * 10;
		}

		// ポート出力する
		vPortAsOutput(PORT_RTS0);

		// RC クロックのキャリブレーションを行う
		ToCoNet_u16RcCalib(sAppData.sFlash.sData.u16RcClock);
	}
}

/**
 * UART コマンド待ち
 * @param E_STATE_RUNNING
 * @param pEv
 * @param eEvent
 * @param u32evarg
 */
PRSEV_HANDLER_DEF(E_STATE_RUNNING, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	if (eEvent == E_EVENT_NEW_STATE) {
		uint8 au8msg[16], *q = au8msg;

		V_PRINTF(LB "[RUNNING]");
		V_FLUSH();

		// 起動メッセージとしてシリアル番号を出力する
		S_BE_DWORD(ToCoNet_u32GetSerial());
		sSerCmdOut.au8data = au8msg;
		sSerCmdOut.u16len = q - au8msg;

		sSerCmdOut.vOutput(&sSerCmdOut, &sSerStream);
	} else
#if 0
	if (!bPortRead(DIO_BUTTON)) {
		// DIO_BUTTON が Hi に戻ったらスリープに戻す
		ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP);
	} else
#endif
	if (eEvent == E_ORDER_KICK) {
		// UARTのコマンドが入力されるまで待って、送信
		tsSerCmd_Context *pCmd = NULL;
		if (u32evarg) {
			pCmd = (void*)u32evarg;
		}

		if (pCmd && pCmd->u16len <= 80) {
			; // この場合は処理する
		} else {
			ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP);
			return;
		}

		V_PRINTF(LB"[RUNNING/UARTRECV ln=%d]", pCmd->u16len);
		sAppData.u16frame_count++; // シリアル番号を更新する

		tsTxDataApp sTx;
		memset(&sTx, 0, sizeof(sTx)); // 必ず０クリアしてから使う！
		uint8 *q =  sTx.auData;

		sTx.u32SrcAddr = ToCoNet_u32GetSerial();

		if (IS_APPCONF_OPT_TO_ROUTER()) {
			// ルータがアプリ中で一度受信して、ルータから親機に再配送
			sTx.u32DstAddr = TOCONET_NWK_ADDR_NEIGHBOUR_ABOVE;
		} else {
			// ルータがアプリ中では受信せず、単純に中継する
			sTx.u32DstAddr = TOCONET_NWK_ADDR_PARENT;
		}

		// ペイロードの準備
		S_OCTET('T');
		S_OCTET(sAppData.sFlash.sData.u8id);
		S_BE_WORD(sAppData.u16frame_count);

		S_OCTET(PKT_ID_UART); // パケット識別子
		S_OCTET(pCmd->u16len); // ペイロードサイズ
		memcpy(q, pCmd->au8data, pCmd->u16len); // データ部
		q += pCmd->u16len;

		sTx.u8Len = q - sTx.auData; // パケットのサイズ
		sTx.u8CbId = sAppData.u16frame_count & 0xFF; // TxEvent で通知される番号、送信先には通知されない
		sTx.u8Seq = sAppData.u16frame_count & 0xFF; // シーケンス番号(送信先に通知される)
		sTx.u8Cmd = 0; // 0..7 の値を取る。パケットの種別を分けたい時に使用する
		//sTx.u8Retry = 0x81; // 強制２回送信

		if (IS_APPCONF_OPT_SECURE()) {
			sTx.bSecurePacket = TRUE;
		}

		if (ToCoNet_Nwk_bTx(sAppData.pContextNwk, &sTx)) {
			V_PRINTF(LB"TxOk");
			ToCoNet_Tx_vProcessQueue(); // 送信処理をタイマーを待たずに実行する
			ToCoNet_Event_SetState(pEv, E_STATE_APP_WAIT_TX);
		} else {
			V_PRINTF(LB"TxFl");
			ToCoNet_Event_SetState(pEv, E_STATE_APP_RETRY);
		}

		if (bRetry) {
			sTx_Retry = sTx;
		}

		V_PRINTF(" FR=%04X", sAppData.u16frame_count);
	}

	// タイムアウト（期待の時間までにデータが来なかった）
	if (ToCoNet_Event_u32TickFrNewState(pEv) > sAppData.sFlash.sData.u32Slp) {
		V_PRINTF(LB"! TIME OUT (E_STATE_RUNNING)");
		ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP); // スリープ状態へ遷移
	}
}

/**
 * 送信完了待ち
 *
 * @param E_STATE_APP_WAIT_TX
 * @param pEv
 * @param eEvent
 * @param u32evarg
 */
PRSEV_HANDLER_DEF(E_STATE_APP_WAIT_TX, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	if (eEvent == E_ORDER_KICK) { // 送信完了イベントが来たのでスリープする
		ToCoNet_Event_SetState(pEv, E_STATE_APP_RETRY); // 再送条件へ遷移
	}
	// タイムアウト
	if (ToCoNet_Event_u32TickFrNewState(pEv) > 100) {
		V_PRINTF(LB"! TIME OUT (E_STATE_APP_WAIT_TX)");
		ToCoNet_Event_SetState(pEv, E_STATE_APP_RETRY); // 再送条件へ遷移
	}
}

/**
 * アプリケーション再送の条件判定と短いスリープ処理
 *
 * @param E_SATET_APP_RETRY
 * @param pEv
 * @param eEvent
 * @param u32evarg
 */
PRSEV_HANDLER_DEF(E_STATE_APP_RETRY, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	V_PRINTF(LB"!E_STATE_APP_RETRY (%d)", u8RetryCount);
	if (!bRetry || u8RetryCount == 0) {
		ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP); // スリープ状態へ遷移
		return;
	}

	// 遷移してきたとき
	if (eEvent == E_EVENT_NEW_STATE) {
		if (u8RetryCount == 0xff) {
			u8RetryCount = sAppData.sFlash.sData.u8wait % 10;
		}

		// 短いスリープを行う (u8RetryCount > 0)
		pEv->bKeepStateOnSetAll = TRUE;
		V_FLUSH();

		uint16 u16dur = u16RetryDur / 2 + (ToCoNet_u32GetRand() % u16RetryDur); // +/-50% のランダム化
		ToCoNet_vSleep(E_AHI_WAKE_TIMER_1, u16dur, FALSE, FALSE);
		return;
	}

	// スリープ復帰
	if (eEvent == E_EVENT_START_UP) {
		V_PRINTF(LB"!woke from mini sleep(%d)", u8RetryCount);
		if (IS_APPCONF_OPT_SECURE()) {
			bRegAesKey(sAppData.sFlash.sData.u32EncKey);
		}
		ToCoNet_Nwk_bResume(sAppData.pContextNwk);
		ToCoNet_Event_SetState(pEv, E_STATE_APP_RETRY_TX);
		u8RetryCount--; // 再送回数を減らす

		return;
	}
}

/**
 * アプリケーション再送
 *
 * @param E_STATE_APP_RETRY_TX
 * @param pEv
 * @param eEvent
 * @param u32evarg
 */
PRSEV_HANDLER_DEF(E_STATE_APP_RETRY_TX, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	// 遷移してきたとき
	if (eEvent == E_EVENT_NEW_STATE) {
		if (ToCoNet_Nwk_bTx(sAppData.pContextNwk, &sTx_Retry)) {
			V_PRINTF(LB"TxOk");
			ToCoNet_Tx_vProcessQueue(); // 送信処理をタイマーを待たずに実行する
			ToCoNet_Event_SetState(pEv, E_STATE_APP_WAIT_TX);
		} else {
			V_PRINTF(LB"TxFl");
			ToCoNet_Event_SetState(pEv, E_STATE_APP_RETRY);
		}

		V_PRINTF(" FR=%04X", sAppData.u16frame_count);
	}
}

/**
 * スリープへの遷移
 *
 * @param E_STATE_APP_SLEEP
 * @param pEv
 * @param eEvent
 * @param u32evarg
 */
PRSEV_HANDLER_DEF(E_STATE_APP_SLEEP, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	if (eEvent == E_EVENT_NEW_STATE) {
		pEv->bKeepStateOnSetAll = FALSE;
		u8RetryCount = 0xff;

		// センサー用の電源制御回路を Hi に戻す
		vPortSetSns(FALSE);

		// RTS0 を通信禁止にする
		vPortSetHi(PORT_RTS0);

		// Sleep は必ず E_EVENT_NEW_STATE 内など１回のみ呼び出される場所で呼び出す。
		V_PRINTF(LB"Sleeping...");
		V_FLUSH();

		// Mininode の場合、特別な処理は無いのだが、ポーズ処理を行う
		ToCoNet_Nwk_bPause(sAppData.pContextNwk);

		// スリープ状態に遷移
		vSleep(0, FALSE, FALSE);
	}
}

/**
 * イベント処理関数リスト
 */
static const tsToCoNet_Event_StateHandler asStateFuncTbl[] = {
	PRSEV_HANDLER_TBL_DEF(E_STATE_IDLE),
	PRSEV_HANDLER_TBL_DEF(E_STATE_RUNNING),
	PRSEV_HANDLER_TBL_DEF(E_STATE_APP_WAIT_TX),
	PRSEV_HANDLER_TBL_DEF(E_STATE_APP_SLEEP),
	PRSEV_HANDLER_TBL_DEF(E_STATE_APP_RETRY),
	PRSEV_HANDLER_TBL_DEF(E_STATE_APP_RETRY_TX),
	PRSEV_HANDLER_TBL_TRM
};

/**
 * イベント処理関数
 * @param pEv
 * @param eEvent
 * @param u32evarg
 */
static void vProcessEvCore(tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	ToCoNet_Event_StateExec(asStateFuncTbl, pEv, eEvent, u32evarg);
}

#if 0
/**
 * ハードウェア割り込み
 * @param u32DeviceId
 * @param u32ItemBitmap
 * @return
 */
static uint8 cbAppToCoNet_u8HwInt(uint32 u32DeviceId, uint32 u32ItemBitmap) {
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
#endif

#if 0
/**
 * ハードウェアイベント（遅延実行）
 * @param u32DeviceId
 * @param u32ItemBitmap
 */
static void cbAppToCoNet_vHwEvent(uint32 u32DeviceId, uint32 u32ItemBitmap) {
	switch (u32DeviceId) {
	case E_AHI_DEVICE_TICK_TIMER:
		break;

	case E_AHI_DEVICE_ANALOGUE:
		break;

	case E_AHI_DEVICE_SYSCTRL:
		break;

	case E_AHI_DEVICE_TIMER0:
		break;

	default:
		break;
	}
}
#endif

#if 1
/**
 * メイン処理
 */
static void cbAppToCoNet_vMain() {
	/* handle serial input */
	vHandleSerialInput();
}
#endif

#if 0
/**
 * ネットワークイベント
 * @param eEvent
 * @param u32arg
 */
static void cbAppToCoNet_vNwkEvent(teEvent eEvent, uint32 u32arg) {
	switch(eEvent) {
	case E_EVENT_TOCONET_NWK_START:
		break;

	default:
		break;
	}
}
#endif


#if 0
/**
 * RXイベント
 * @param pRx
 */
static void cbAppToCoNet_vRxEvent(tsRxDataApp *pRx) {

}
#endif

/**
 * TXイベント
 * @param u8CbId
 * @param bStatus
 */
static void cbAppToCoNet_vTxEvent(uint8 u8CbId, uint8 bStatus) {
	// 送信完了
	ToCoNet_Event_Process(E_ORDER_KICK, 0, vProcessEvCore);
}

/**
 * UART コマンド
 */
static void cb_vProcessSerialCmd(tsSerCmd_Context *pCmd) {
	ToCoNet_Event_Process(E_ORDER_KICK, (uint32)(void *)pCmd, vProcessEvCore);
}

/**
 * アプリケーションハンドラー定義
 *
 */
static tsCbHandler sCbHandler = {
	NULL, // cbAppToCoNet_u8HwInt,
	NULL, // cbAppToCoNet_vHwEvent,
	cbAppToCoNet_vMain,
	NULL, // cbAppToCoNet_vNwkEvent,
	NULL, // cbAppToCoNet_vRxEvent,
	cbAppToCoNet_vTxEvent
};

/**
 * アプリケーション初期化
 */
void vInitAppUart() {
	psCbHandler = &sCbHandler;
	pvProcessEv1 = vProcessEvCore;
	pf_cbProcessSerialCmd = cb_vProcessSerialCmd;

	// シリアルの書式出力のため
	if (IS_APPCONF_OPT_UART_BIN()) {
		SerCmdBinary_vInit(&sSerCmdOut, NULL, 128); // バッファを指定せず初期化
	} else {
		SerCmdAscii_vInit(&sSerCmdOut, NULL, 128); // バッファを指定せず初期化
	}
}
