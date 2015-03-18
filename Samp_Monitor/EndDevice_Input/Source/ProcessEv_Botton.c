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
static void vStoreSensorValue();
static uint8 readInput();

/*
 * 最初に遷移してくる状態
 */
PRSEV_HANDLER_DEF(E_STATE_IDLE, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	//	この状態から始まったときモジュールIDなどを表示する
	if (eEvent == E_EVENT_START_UP) {
		// 起床メッセージ
		vSerInitMessage();

		// 暗号化鍵の登録
		if (IS_APPCONF_OPT_SECURE()) {
			bool_t bRes = bRegAesKey(sAppData.sFlash.sData.u32EncKey);
			V_PRINTF(LB "*** Register AES key (%d) ***", bRes);
		}

		//	初回起動(リセット)かスリープからの復帰かで表示するメッセージを変える
		if (u32evarg & EVARG_START_UP_WAKEUP_RAMHOLD_MASK) {
			// Warm start message
			V_PRINTF(LB "*** Warm starting woke by %s. ***", sAppData.bWakeupByButton ? "DIO" : "WakeTimer");

			// RESUME
			ToCoNet_Nwk_bResume(sAppData.pContextNwk);

			// RUNNING状態へ遷移
			ToCoNet_Event_SetState(pEv, E_STATE_RUNNING);
		} else {
			// 開始する
			// start up message
			V_PRINTF(LB "*** Cold starting");
			V_PRINTF(LB "* start end device[%d]", u32TickCount_ms & 0xFFFF);

			sAppData.sNwkLayerTreeConfig.u8Role = TOCONET_NWK_ROLE_ENDDEVICE;
			// ネットワークの初期化
			sAppData.pContextNwk = ToCoNet_NwkLyTr_psConfig_MiniNodes(&sAppData.sNwkLayerTreeConfig);

			if (sAppData.pContextNwk) {
				ToCoNet_Nwk_bInit(sAppData.pContextNwk);
				ToCoNet_Nwk_bStart(sAppData.pContextNwk);
			}

			// 最初にパケットを送りたくないのでチャタリング対策状態へ遷移後、割り込みがあるまでスリープ
			ToCoNet_Event_SetState(pEv, E_STATE_APP_CHAT_SLEEP);
		}

	}
}

/*	パケットを送信する状態	*/
PRSEV_HANDLER_DEF(E_STATE_RUNNING, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	if (eEvent == E_EVENT_NEW_STATE) {
		// ADC の開始
		vADC_WaitInit();
		vSnsObj_Process(&sAppData.sADC, E_ORDER_KICK);
	}
	if (eEvent == E_ORDER_KICK) {
		V_PRINTF(LB"[E_STATE_RUNNING/SNS_COMP]");
		sAppData.u16frame_count++; // シリアル番号を更新する

		tsTxDataApp sTx;
		memset(&sTx, 0, sizeof(sTx)); // 必ず０クリアしてから使う！
		uint8 *q =  sTx.auData;

		sTx.u32SrcAddr = ToCoNet_u32GetSerial();

		sTx.u16DelayMin = 0;
		sTx.u16DelayMax = 0;
		sTx.u16RetryDur = 0;
		sTx.u8Retry = 0;

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

		S_OCTET(sAppData.sFlash.sData.u8mode);	// パケット識別子

		S_OCTET(sAppData.sSns.u8Batt);

		S_BE_WORD(sAppData.sSns.u16Adc1);
		S_BE_WORD(sAppData.sSns.u16Adc2);

		//	立ち上がりで起動 or 立ち下がりで起動
		if( sAppData.sFlash.sData.i16param == 1 ){
			S_OCTET(0x01);
		} else {
			S_OCTET(0x00);
		}

		/*	DIの入力状態を取得	*/
		uint8 DI_Bitmap = readInput();
		S_OCTET( DI_Bitmap );


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
			ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP);
		}
		V_PRINTF(" FR=%04X", sAppData.u16frame_count);
	}
}

/*	送信完了状態	*/
PRSEV_HANDLER_DEF(E_STATE_APP_WAIT_TX, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	if (eEvent == E_ORDER_KICK) { // 送信完了イベントが来たのでスリープする
		ToCoNet_Event_SetState(pEv, E_STATE_APP_CHAT_SLEEP); // スリープ状態へ遷移
	}
}

/*
 * 送信後のチャタリング対策を行う状態
 */
PRSEV_HANDLER_DEF(E_STATE_APP_CHAT_SLEEP, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {

		/*	遷移してきたとき一旦眠る	*/
		if(eEvent == E_EVENT_NEW_STATE){
			V_PRINTF(LB"Safe Chattering...");
			V_FLUSH();
			// Mininode の場合、特別な処理は無いのだが、ポーズ処理を行う
			ToCoNet_Nwk_bPause(sAppData.pContextNwk);

			//	割り込み禁止でスリープ
			pEv->bKeepStateOnSetAll = TRUE;		//	この状態から起床
			vAHI_DioWakeEnable(0, PORT_INPUT_MASK); // DISABLE DIO WAKE SOURCE

			ToCoNet_vSleep(E_AHI_WAKE_TIMER_1, 200UL, FALSE, FALSE);

		/*	起床後すぐこの状態になったときずっと眠る状態へ	*/
		}else if(eEvent == E_EVENT_START_UP){
			pEv->bKeepStateOnSetAll = FALSE;
			ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP); // スリープ状態へ遷移
		}
}

/*	スリープをする状態	*/
PRSEV_HANDLER_DEF(E_STATE_APP_SLEEP, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	if (eEvent == E_EVENT_NEW_STATE) {
		// Sleep は必ず E_EVENT_NEW_STATE 内など１回のみ呼び出される場所で呼び出す。
		V_PRINTF(LB"Sleeping...");
		V_FLUSH();

		// Mininode の場合、特別な処理は無いのだが、ポーズ処理を行う
		ToCoNet_Nwk_bPause(sAppData.pContextNwk);

		// print message.
		vAHI_UartDisable(UART_PORT); // UART を解除してから(このコードは入っていなくても動作は同じ)

		// set UART Rx port as interrupt source
		vAHI_DioSetDirection(PORT_INPUT_MASK, 0); // set as input

		(void)u32AHI_DioInterruptStatus(); // clear interrupt register

		vAHI_DioWakeEnable(PORT_INPUT_MASK, 0); // also use as DIO WAKE SOURCE

		if(sAppData.sFlash.sData.i16param == 1){
			vAHI_DioWakeEdge(0, PORT_INPUT_MASK); // 割り込みエッジ（立下りに設定）
		} else {
			vAHI_DioWakeEdge(PORT_INPUT_MASK, 0); // 割り込みエッジ（立上がりに設定）
		}

		// wake up using wakeup timer as well.
		ToCoNet_vSleep(E_AHI_WAKE_TIMER_0, 0, FALSE, FALSE ); // PERIODIC RAM OFF SLEEP USING WK0

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
	PRSEV_HANDLER_TBL_DEF(E_STATE_APP_CHAT_SLEEP),
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
		/*
		 * ADC完了割り込み
		 */
		V_PUTCHAR('@');
		vSnsObj_Process(&sAppData.sADC, E_ORDER_KICK);
		if (bSnsObj_isComplete(&sAppData.sADC)) {
			// 全チャネルの処理が終わったら、次の処理を呼び起こす
			vStoreSensorValue();
			ToCoNet_Event_Process(E_ORDER_KICK, 0, vProcessEvCore);
		}
		break;

	case E_AHI_DEVICE_SYSCTRL:
		break;

	case E_AHI_DEVICE_TIMER0:
		break;

	default:
		break;
	}
}

#if 0
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
 * アプリケーションハンドラー定義
 *
 */
static tsCbHandler sCbHandler = {
	NULL, // cbAppToCoNet_u8HwInt,
	cbAppToCoNet_vHwEvent,
	NULL, // cbAppToCoNet_vMain,
	NULL, // cbAppToCoNet_vNwkEvent,
	NULL, // cbAppToCoNet_vRxEvent,
	cbAppToCoNet_vTxEvent
};

/**
 * アプリケーション初期化
 */
void vInitAppBotton() {
	psCbHandler = &sCbHandler;
	pvProcessEv1 = vProcessEvCore;
}

/**
 * センサー値を格納する
 */
static void vStoreSensorValue() {
	// パルス数の読み込み
	bAHI_Read16BitCounter(E_AHI_PC_0, &sAppData.sSns.u16PC1); // 16bitの場合
	// パルス数のクリア
	bAHI_Clear16BitPulseCounter(E_AHI_PC_0); // 16bitの場合

	// パルス数の読み込み
	bAHI_Read16BitCounter(E_AHI_PC_1, &sAppData.sSns.u16PC2); // 16bitの場合
	// パルス数のクリア
	bAHI_Clear16BitPulseCounter(E_AHI_PC_1); // 16bitの場合

	// センサー値の保管
	sAppData.sSns.u16Adc1 = sAppData.sObjADC.ai16Result[TEH_ADC_IDX_ADC_1];
#ifdef USE_TEMP_INSTDOF_ADC2
	sAppData.sSns.u16Adc2 = sAppData.sObjADC.ai16Result[TEH_ADC_IDX_TEMP];
#else
	sAppData.sSns.u16Adc2 = sAppData.sObjADC.ai16Result[TEH_ADC_IDX_ADC_2];
#endif
	sAppData.sSns.u8Batt = ENCODE_VOLT(sAppData.sObjADC.ai16Result[TEH_ADC_IDX_VOLT]);

	// ADC1 が 1300mV 以上(SuperCAP が 2600mV 以上)である場合は SUPER CAP の直結を有効にする
	if (sAppData.sSns.u16Adc1 >= VOLT_SUPERCAP_CONTROL) {
		vPortSetLo(DIO_SUPERCAP_CONTROL);
	}

	// センサー用の電源制御回路を Hi に戻す
	vPortSetSns(FALSE);
}

/*
 * DIの状態をBitMapにして返す
 */
static uint8 readInput(void)
{
	uint8	bitmap = 0;
	bool_t	btn;
	uint8	i;

	uint8 au8PortTbl_DIn[4] = {
		PORT_INPUT1,
		PORT_INPUT2,
		PORT_INPUT3,
		PORT_INPUT4
	};

	for( i=0; i<4; i++ ){
		btn = bPortRead(au8PortTbl_DIn[i]);
		if( btn ){
			bitmap |= ( 1UL<<i );
		}
	}
	return bitmap;
}
