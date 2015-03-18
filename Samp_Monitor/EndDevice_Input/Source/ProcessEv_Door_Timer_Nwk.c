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

static void vProcessEvCore(tsEvent *pEv, teEvent eEvent, uint32 u32evarg);

/**
 * FIREイベントを受けとたら、動作開始
 * @param E_STATE_IDLE
 * @param pEv
 * @param eEvent
 * @param u32evarg
 */
PRSEV_HANDLER_DEF(E_STATE_IDLE, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	if (eEvent == E_EVENT_START_UP) {
		if (u32evarg & EVARG_START_UP_WAKEUP_RAMHOLD_MASK) {
			ToCoNet_Nwk_bResume(sAppData.pContextNwk);
		} else {
			V_PRINTF(LB "* N/W start request");
			sAppData.sNwkLayerTreeConfig.u8Role = TOCONET_NWK_ROLE_ENDDEVICE;
			// ネットワークの初期化
			sAppData.pContextNwk = ToCoNet_NwkLyTr_psConfig_MiniNodes(&sAppData.sNwkLayerTreeConfig);

			if (sAppData.pContextNwk) {
				ToCoNet_Nwk_bInit(sAppData.pContextNwk);
				ToCoNet_Nwk_bStart(sAppData.pContextNwk);
				sAppData.u8NwkStat = E_IO_TIMER_NWK_NWK_START; // mininode の初期化は関数終了をもって完了する
			}
		}
	}

	if (sAppData.u8NwkStat == E_IO_TIMER_NWK_FIRE_REQUEST) { // FIRE
		ToCoNet_Event_SetState(pEv, E_STATE_APP_WAIT_TX);
	}
}

/**
 * 送信要求と送信完了待ちを行う
 * - 完了待ちは、TICKTIMER 毎に本関数が呼び出され、フラグがセットされたら完了とする
 *
 * @param E_STATE_APP_WAIT_TX
 * @param pEv
 * @param eEvent
 * @param u32evarg
 */
PRSEV_HANDLER_DEF(E_STATE_APP_WAIT_TX, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	if (eEvent == E_EVENT_NEW_STATE) {
		V_PRINTF(LB"** RF transmit request - ");
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

		S_OCTET(sAppData.sFlash.sData.u8mode); // パケット識別子

		S_OCTET(sAppData.bDI1_Now_Opened); // 開閉状態
		S_BE_DWORD(sAppData.u32DI1_Dur_Opened_ms); // 連続開時間

		sTx.u8Len = q - sTx.auData; // パケットのサイズ
		sTx.u8CbId = sAppData.u16frame_count & 0xFF; // TxEvent で通知される番号、送信先には通知されない
		sTx.u8Seq = sAppData.u16frame_count & 0xFF; // シーケンス番号(送信先に通知される)
		sTx.u8Cmd = 0; // 0..7 の値を取る。パケットの種別を分けたい時に使用する
		sTx.u8Retry = 0x81; // 強制２回送信

		if (IS_APPCONF_OPT_SECURE()) {
			sTx.bSecurePacket = TRUE;
		}

		if (ToCoNet_Nwk_bTx(sAppData.pContextNwk, &sTx)) {
			V_PRINTF(LB "Tx Start...");
			//extern void ToCoNet_Tx_vProcessQueue();
			//ToCoNet_Tx_vProcessQueue(); // 送信処理をタイマーを待たずに実行する
		} else {
			V_PRINTF(LB "Tx Fail.");
			sAppData.u8NwkStat = E_IO_TIMER_NWK_COMPLETE_TX_FAIL;
		}

		V_PRINTF(" FR=%04X ", sAppData.u16frame_count);
	}

	// 送信完了
	if (sAppData.u8NwkStat & E_IO_TIMER_NWK_COMPLETE_MASK) {
		V_PRINTF(LB" %s", sAppData.u8NwkStat == E_IO_TIMER_NWK_COMPLETE_TX_SUCCESS ? "Complete" : "Fail");

		ToCoNet_Nwk_bPause(sAppData.pContextNwk);
		ToCoNet_Event_SetState(pEv, E_STATE_IDLE);
	}
}


/**
 * イベント処理関数リスト
 */
static const tsToCoNet_Event_StateHandler asStateFuncTbl[] = {
	PRSEV_HANDLER_TBL_DEF(E_STATE_IDLE),
	PRSEV_HANDLER_TBL_DEF(E_STATE_APP_WAIT_TX),
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

/**
 * 初期化２
 */
void vInitAppDoorTimerSub() {
	pvProcessEv2 = vProcessEvCore;
}
