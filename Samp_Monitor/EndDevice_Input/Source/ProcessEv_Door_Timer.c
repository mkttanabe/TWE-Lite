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

static void vSleep_IO_Timer(uint32 u32SleepDur_ms, bool_t bPeriodic, bool_t bDeep, bool_t bInt);
extern tsTimerContext sTimerPWM[1]; //!< タイマー管理構造体  @ingroup MASTER

/**
 * 起動直後の初期状態。
 * - 初期化処理
 * - 速やかに E_STATE_RUNNING に遷移する
 *
 * @param E_STATE_IDLE
 * @param pEv
 * @param eEvent
 * @param u32evarg
 */
PRSEV_HANDLER_DEF(E_STATE_IDLE, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	if (eEvent == E_EVENT_START_UP) {
		sAppData.u8NwkStat = E_IO_TIMER_NWK_IDLE;
		vSerInitMessage();

		sAppData.bDI1_Now_Opened = FALSE;

		vPortSetLo(PORT_KIT_LED1);
		vPortAsOutput(PORT_KIT_LED1);
		// 暗号化鍵の登録
		if (IS_APPCONF_OPT_SECURE()) {
			bool_t bRes = bRegAesKey(sAppData.sFlash.sData.u32EncKey);
			V_PRINTF(LB "*** Register AES key (%d) ***", bRes);
		}

		if (sAppData.bWakeupByButton) {
			sAppData.u32DI1_Dur_Opened_ms = 0;
			sAppData.u16DI1_Ct_PktFired = 0;
			sAppData.bDI1_Now_Opened = TRUE; // 割り込み起床時なので Open
		} else {
			// IOの再チェック
			sAppData.bDI1_Now_Opened = (bPortRead(PORT_INPUT1) == FALSE);
		}

		if (u32evarg & EVARG_START_UP_WAKEUP_RAMHOLD_MASK) {
			// Warm start message
			V_PRINTF(LB "*** Warm starting woke by %s. ***", sAppData.bWakeupByButton ? "DIO" : "WakeTimer");
		} else {
			// 開始する
			// start up message
			V_PRINTF(LB "*** Cold starting");
			V_PRINTF(LB "* start end device[%d]", u32TickCount_ms & 0xFFFF);
		}

		V_PRINTF(LB "* IO=%d, t=%d, ct=%d", sAppData.bDI1_Now_Opened, sAppData.u32DI1_Dur_Opened_ms, sAppData.u16DI1_Ct_PktFired);
	}

	ToCoNet_Event_SetState(pEv, E_STATE_RUNNING);
}

/**
 * IO の状態を確認し、
 * - ブザーを鳴らす
 * - 無線送信
 * を実施する。完了後スリープ状態に遷移する。
 *
 * @param E_STATE_RUNNING
 * @param pEv
 * @param eEvent
 * @param u32evarg
 */
PRSEV_HANDLER_DEF(E_STATE_RUNNING, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	static bool_t bBuzz;
	static bool_t bTxCmp;

	/*
	 * ブザーを鳴らす処理
	 */
	if (eEvent == E_EVENT_NEW_STATE) {
		bBuzz = FALSE;
		bTxCmp = TRUE;

		if (sAppData.bDI1_Now_Opened) {
			// スイッチが開いていて、.u32Slp が経過した時点でブザーを鳴らす
			if (!sAppData.u32DI1_Dur_Opened_ms ||  sAppData.u32DI1_Dur_Opened_ms >= sAppData.sFlash.sData.u32Slp) {
				sTimerPWM[0].u16duty = 512;
				vTimerChangeDuty(&sTimerPWM[0]);

				bBuzz = TRUE;
				V_PRINTF(LB"* Buzz Start");
			} else {
				// 直ぐにスリープ
				ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP);
			}
		}

		// 送信
		if (sAppData.u32DI1_Dur_Opened_ms == 0 // 初回
			|| (sAppData.u32DI1_Dur_Opened_ms % sAppData.sFlash.sData.u32Slp) < u16_IO_Timer_mini_sleep_dur // .u32Slp 周期
			|| !sAppData.bDI1_Now_Opened // 閉じたとき
		) {
			// 送信要求
			sAppData.u8NwkStat = E_IO_TIMER_NWK_FIRE_REQUEST;
			sAppData.u16DI1_Ct_PktFired++;

			bTxCmp = FALSE;
		}

		return;
	}

	if (bBuzz && ToCoNet_Event_u32TickFrNewState(pEv) > u16_IO_Timer_buzz_dur) {
		// u16_IO_Timer_buzz_dur [ms] を超えたら終了
		if (sTimerPWM[0].u16duty != 1024) {
			sTimerPWM[0].u16duty = 1024;
			vTimerChangeDuty(&sTimerPWM[0]);

			V_PRINTF(LB"* Buzz End");
		}

		bBuzz = FALSE;
	}

	if (sAppData.u8NwkStat) { // 送信が実施された時
		bool_t bCond = FALSE;

		bCond |= (sAppData.u8NwkStat & E_IO_TIMER_NWK_COMPLETE_MASK) != 0;
		bCond |= (ToCoNet_Event_u32TickFrNewState(pEv) > u16_IO_Timer_buzz_dur + 50); // 50ms 余分にタイムアウトを設定

		if (bCond) {
			// 送信完了を待ってスリープ
			bTxCmp = TRUE;
			V_PRINTF(LB"* TxCmp %d", eEvent);
		}
	}

	if (!bBuzz && bTxCmp) { // ブザーが鳴り終わった&&送信し終わった
		// 終了条件
		ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP);
	}
}

/**
 * スリープ処理を行う。
 * IO状態が元に戻れば、周期スリープをやめて割り込み起床のみ
 *
 * @param E_STATE_APP_SLEEP
 * @param pEv
 * @param eEvent
 * @param u32evarg
 */
PRSEV_HANDLER_DEF(E_STATE_APP_SLEEP, tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	if (eEvent == E_EVENT_NEW_STATE) {
		// センサー用の電源制御回路を Hi に戻す
		vPortSetSns(FALSE);

		// Sleep は必ず E_EVENT_NEW_STATE 内など１回のみ呼び出される場所で呼び出す。
		V_PRINTF(LB"Sleeping...");

		// 念のため状態を再チェック
		bool_t bOpen = (bPortRead(PORT_INPUT1) == FALSE);

		// 周期スリープに入る
		//  - 初回は一定時間秒あけて、次回以降はスリープ復帰を基点に５秒
		if (sAppData.bDI1_Now_Opened || bOpen) {
			// IO が LO の間は周期スリープする
			V_PRINTF("%dms", u16_IO_Timer_mini_sleep_dur);
			V_FLUSH();

			bool_t bPeriodic = (sAppData.u32DI1_Dur_Opened_ms != 0); // 初回はこの時点から 1000ms, それ以外は周期的に。

			sAppData.u32DI1_Dur_Opened_ms += u16_IO_Timer_mini_sleep_dur;
			vSleep_IO_Timer(u16_IO_Timer_mini_sleep_dur, bPeriodic, FALSE, FALSE);
		} else {
			V_PRINTF("no timer");
			V_FLUSH();

			sAppData.u32DI1_Dur_Opened_ms = 0;
			vSleep_IO_Timer(60*1000UL, FALSE, FALSE, TRUE); // １分に１回は起きて送信する&割り込み有効
		}
	}
}

/** @ingroup MASTER
 * スリープの実行
 * @param u32SleepDur_ms スリープ時間[ms]
 * @param bPeriodic TRUE:前回の起床時間から次のウェイクアップタイミングを計る
 * @param bDeep TRUE:RAM OFF スリープ
 */
static void vSleep_IO_Timer(uint32 u32SleepDur_ms, bool_t bPeriodic, bool_t bDeep, bool_t bInt) {
	// set UART Rx port as interrupt source
	vAHI_DioSetDirection(PORT_INPUT_MASK, 0); // set as input

	(void)u32AHI_DioInterruptStatus(); // clear interrupt register

	if (bInt) { // waketimer なしスリープ時に DIO 割り込みを設定
		vAHI_DioWakeEnable(PORT_INPUT_MASK, 0); // also use as DIO WAKE SOURCE
		// vAHI_DioWakeEdge(0, PORT_INPUT_MASK); // 割り込みエッジ(立ち上がり)
		vAHI_DioWakeEdge(PORT_INPUT_MASK, 0); // 割り込みエッジ(立ち上がり)
	} else {
		vAHI_DioWakeEnable(0, PORT_INPUT_MASK); // DISABLE DIO WAKE SOURCE
	}

	// wake up using wakeup timer as well.
	ToCoNet_vSleep(E_AHI_WAKE_TIMER_0, u32SleepDur_ms, bPeriodic, bDeep); // PERIODIC RAM OFF SLEEP USING WK0
}


/**
 * イベント処理関数リスト
 */
static const tsToCoNet_Event_StateHandler asStateFuncTbl[] = {
	PRSEV_HANDLER_TBL_DEF(E_STATE_IDLE),
	PRSEV_HANDLER_TBL_DEF(E_STATE_RUNNING),
	PRSEV_HANDLER_TBL_DEF(E_STATE_APP_SLEEP),
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
 * ハード割り込み
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
 * ハードイベント（遅延割り込み処理）
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
	sAppData.u8NwkStat = bStatus ? E_IO_TIMER_NWK_COMPLETE_TX_SUCCESS : E_IO_TIMER_NWK_COMPLETE_TX_FAIL;
}

/**
 * アプリケーションハンドラー定義
 *
 */
static tsCbHandler sCbHandler = {
	NULL, // cbAppToCoNet_u8HwInt,
	NULL, // cbAppToCoNet_vHwEvent,
	NULL, // cbAppToCoNet_vMain,
	NULL, //cbAppToCoNet_vNwkEvent,
	NULL, // cbAppToCoNet_vRxEvent,
	cbAppToCoNet_vTxEvent
};

/**
 * アプリケーション初期化
 */
void vInitAppDoorTimer() {
	psCbHandler = &sCbHandler;
	pvProcessEv1 = vProcessEvCore;

	extern void vInitAppDoorTimerSub();
	vInitAppDoorTimerSub();
}
