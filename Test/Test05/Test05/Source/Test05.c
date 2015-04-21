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
/*
 * Test05.c
 *
 * Test04.c の送信処理を独立させ 電池稼働を想定して以下の機能を加えたもの
 *
 * - TWE-Lite のモード設定ビット 1 (M1) が GND に接続されていれば
 *   デジタル入力 1 (DI1) の立ち上がり (Lo -> Hi) を送信のトリガーとする
 *   -> 典型的には平時導通状態の磁気リードスイッチが切断された状況
 *
 *   ※立ち上がりトリガーの場合 節電のため DI1 の内部プルアップは無効化
 *     される。TOCOS 社は外部プルアップとして 1MΩ 抵抗の設置を推奨。
 *     http://tocos-wireless.com/jp/products/TWE-EH-S/sw_html/mode_push.html
 *
 *   M1 が GND に接続されていなければ DI1 の立ち下がりをトリガーとする
 *   -> 典型的には平時非導通状態のタクトスイッチが押下された状況
 *
 * - 節電のため送信を終えたらすみやかに Sleep 状態へ移行し DI1 の状態が
 *   変化すると Sleep から復帰する
 *
 * 受信側は Test04 のバイナリと回路をそのまま使用する
 *
 * 2015 KLab Inc.
 *
 */

#include <string.h>          // C 標準ライブラリ用
#include <AppHardwareApi.h>  // NXP ペリフェラル API 用
#include "utils.h"           // ペリフェラル API のラッパなど
#include "serial.h"          // シリアル用
#include "sprintf.h"         // SPRINTF 用

#include "ToCoNet.h"
#include "ToCoNet_mod_prototype.h" // ToCoNet モジュール定義

#define DI1  12          // デジタル入力 1
#define M1   10          // モード設定ビット 1
#define UART_BAUD 115200 // シリアルのボーレート

// Sleep からの復帰に DI1 の状態変化を利用するための bitmap
#define PORT_BITMAP_DI1 (1UL << DI1)

// ToCoNet 用パラメータ
#define APP_ID   0x33333333
#define CHANNEL  18

// 状態定義（ユーザ）
typedef enum
{
	E_STATE_APP_BASE = ToCoNet_STATE_APP_BASE,
	E_STATE_APP_WAIT_TX,
	E_STATE_APP_SLEEP,
} teStateApp;

static tsFILE sSerStream;            // シリアル用ストリーム
static tsSerialPortSetup sSerPort;   // シリアルポートデスクリプタ
static uint32 u32Seq;                // 送信パケットのシーケンス番号
static bool_t triggerEdgeRising_DI1; // TRUE: 送信トリガーは DI1 の立ち上がり
                                     // FALSE:      〃       DI1 の立ち下がり
// デバッグメッセージ出力用
#define DBG
#ifdef DBG
#define dbg(...) vfPrintf(&sSerStream, LB __VA_ARGS__)
#else
#define dbg(...)
#endif

// デバッグ出力用に UART を初期化
static void vSerialInit() {
	static uint8 au8SerialTxBuffer[96];
	static uint8 au8SerialRxBuffer[32];

	sSerPort.pu8SerialRxQueueBuffer = au8SerialRxBuffer;
	sSerPort.pu8SerialTxQueueBuffer = au8SerialTxBuffer;
	sSerPort.u32BaudRate = UART_BAUD;
	sSerPort.u16AHI_UART_RTS_LOW = 0xffff;
	sSerPort.u16AHI_UART_RTS_HIGH = 0xffff;
	sSerPort.u16SerialRxQueueSize = sizeof(au8SerialRxBuffer);
	sSerPort.u16SerialTxQueueSize = sizeof(au8SerialTxBuffer);
	sSerPort.u8SerialPort = E_AHI_UART_0;
	sSerPort.u8RX_FIFO_LEVEL = E_AHI_UART_FIFO_LEVEL_1;
	SERIAL_vInit(&sSerPort);

	sSerStream.bPutChar = SERIAL_bTxChar;
	sSerStream.u8Device = E_AHI_UART_0;
}

// ハードウェア初期化
static void vInitHardware()
{
	// デバッグ出力用
	vSerialInit();
	ToCoNet_vDebugInit(&sSerStream);
	ToCoNet_vDebugLevel(0);

	// 使用ポートの設定
	vPortAsInput(DI1);
	vPortAsInput(M1);
}

// ブロードキャスト実行
static bool_t sendBroadcast ()
{
	tsTxDataApp tsTx;
	memset(&tsTx, 0, sizeof(tsTxDataApp));

	tsTx.u32SrcAddr = ToCoNet_u32GetSerial();
	tsTx.u32DstAddr = TOCONET_MAC_ADDR_BROADCAST;

	tsTx.bAckReq = FALSE;
	tsTx.u8Retry = 0x02; // 送信失敗時は 2回再送
	tsTx.u8CbId = u32Seq & 0xFF;
	tsTx.u8Seq = u32Seq & 0xFF;
	tsTx.u8Cmd = TOCONET_PACKET_CMD_APP_DATA;

	// SPRINTF でペイロードを作成
	SPRINTF_vRewind();
	vfPrintf(SPRINTF_Stream, "Hello! from %08X seq=%u", ToCoNet_u32GetSerial(), u32Seq);
	memcpy(tsTx.auData, SPRINTF_pu8GetBuff(), SPRINTF_u16Length());
	tsTx.u8Len = SPRINTF_u16Length();
	u32Seq++;

	// 送信
	return ToCoNet_bMacTxReq(&tsTx);
}

// ユーザ定義のイベントハンドラ
static void vProcessEvCore(tsEvent *pEv, teEvent eEvent, uint32 u32evarg)
{
	switch (pEv->eState) {
	// アイドル状態
	case E_STATE_IDLE:
		if (eEvent == E_EVENT_START_UP) { // 起動時
			dbg("** Test05 **");
			dbg("triggerEdgeRising_DI1=%d", triggerEdgeRising_DI1);
			// Sleep からの復帰なら稼働状態へ
			// 電源投入直後やリセット後ならそのまま Sleep へ
			if (u32evarg & EVARG_START_UP_WAKEUP_RAMHOLD_MASK) {
				ToCoNet_Event_SetState(pEv, E_STATE_RUNNING);
			} else {
				ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP);
			}
		}
		break;

	// 稼働状態
	case E_STATE_RUNNING:
		if (eEvent == E_ORDER_KICK) {
			if (u32evarg == TRUE) {
				// state を送信完了待ちにして送信を実行
				ToCoNet_Event_SetState(pEv, E_STATE_APP_WAIT_TX);
				sendBroadcast();
			} else {
				// Sleep へ
				ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP);
			}
		}
		break;

	// 送信完了待ち状態
	case E_STATE_APP_WAIT_TX:
		if (eEvent == E_ORDER_KICK) {
			// 送信完了につき Sleep 状態へ移行
			ToCoNet_Event_SetState(pEv, E_STATE_APP_SLEEP);
		}
		break;

	// Sleep への移行状態
	case E_STATE_APP_SLEEP:
		// vSleep は必ず E_EVENT_NEW_STATE 内など１回のみ呼び出される場所で呼び出す
		if (eEvent == E_EVENT_NEW_STATE) {
			dbg("Sleeping...\r\n");
			// UART 出力の完了を待つ
			WAIT_UART_OUTPUT(E_AHI_UART_0);
			// DI1 の状態変化を Sleep から復帰のトリガーとする
			vAHI_DioWakeEnable(PORT_BITMAP_DI1, 0);
			/*
			if (triggerEdgeRising_DI1) {
				vAHI_DioWakeEdge(PORT_BITMAP_DI1, 0); // Lo -> Hi に反応
			} else {
				vAHI_DioWakeEdge(0, PORT_BITMAP_DI1); // Hi -> Lo に反応
			}
			*/
			// RAM 状態保持のまま Sleep
			ToCoNet_vSleep(E_AHI_WAKE_TIMER_0, 0, FALSE, FALSE);
		}
		break;

	default:
		break;
	}
	return;
}

//
// 以下 ToCoNet 既定のイベントハンドラ群
//

// 割り込み発生後に随時呼び出される
void cbToCoNet_vMain(void)
{
	static unsigned char stsPrev = 0xFF;

	bool_t sts = bPortRead(DI1);
	// DI1 の状態が変化したら処理
	if (sts != stsPrev) {
		stsPrev = sts;
		dbg("DI1 is [%s]", (sts) ? "Lo" : "Hi");

		// DI1 の状態と 立ち上がり or 立ち下がりトリガーの設定が
		// 符合すれば送信処理へ   そうでなければ Sleep へ
		if ((sts && !triggerEdgeRising_DI1) ||
			(!sts && triggerEdgeRising_DI1)) {
			// E_ORDER_KICK イベントを引数 TRUE で通知 -> 送信
			ToCoNet_Event_Process(E_ORDER_KICK, TRUE, vProcessEvCore);
		} else {
			// E_ORDER_KICK イベントを引数 FALSE で通知 -> Sleep
			ToCoNet_Event_Process(E_ORDER_KICK, FALSE, vProcessEvCore);
		}
	}
	return;
}

// パケット受信時
void cbToCoNet_vRxEvent(tsRxDataApp *pRx)
{
	return;
}

// パケット送信完了時
void cbToCoNet_vTxEvent(uint8 u8CbId, uint8 bStatus)
{
	dbg(">> SEND %s seq=%u", bStatus ? "OK" : "NG", u32Seq);
	// E_ORDER_KICK イベントを通知
	ToCoNet_Event_Process(E_ORDER_KICK, 0, vProcessEvCore);
	return;
}

// ネットワークイベント発生時
void cbToCoNet_vNwkEvent(teEvent eEvent, uint32 u32arg)
{
	return;
}

// ハードウェア割り込み発生後（遅延呼び出し）
void cbToCoNet_vHwEvent(uint32 u32DeviceId, uint32 u32ItemBitmap)
{
	return;
}

// ハードウェア割り込み発生時
uint8 cbToCoNet_u8HwInt(uint32 u32DeviceId, uint32 u32ItemBitmap)
{
	return FALSE;
}

// コールドスタート時
void cbAppColdStart(bool_t bAfterAhiInit)
{
	if (!bAfterAhiInit) {
		// 必要モジュール登録手続き
		ToCoNet_REG_MOD_ALL();
	} else {
		// 電源電圧が 2.0V まで下降しても稼働を継続
		vAHI_BrownOutConfigure(0, FALSE, FALSE, FALSE, FALSE);

		// SPRINTF 初期化
		SPRINTF_vInit128();

		// ToCoNet パラメータ
		sToCoNet_AppContext.u32AppId = APP_ID;
		sToCoNet_AppContext.u8Channel = CHANNEL;
		u32Seq = 0;

		// ユーザ定義のイベントハンドラを登録
		ToCoNet_Event_Register_State_Machine(vProcessEvCore);

		// ハードウェア初期化
		vInitHardware();

		// M1 が GND に接続 (= Lo) されていれば
		// 送信のトリガーは DI1 の立ち上がり (Lo -> Hi) とする
		// そうでなければトリガーは DI1 の立ち下がり (Hi -> Lo) とする
		triggerEdgeRising_DI1 = bPortRead(M1);
		if (triggerEdgeRising_DI1) {
			// 立ち上がりトリガーの場合は節電のため内部プルアップを無効化
			vPortDisablePullup(DI1);
		}

		// MAC 層開始
		ToCoNet_vMacStart();
	}
}

// ウォームスタート時
void cbAppWarmStart(bool_t bAfterAhiInit)
{
	if (!bAfterAhiInit) {
	} else {
		vAHI_BrownOutConfigure(0, FALSE, FALSE, FALSE, FALSE);
		vInitHardware(bAfterAhiInit);
		ToCoNet_vMacStart();
	}
}
