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
 * Test04.c
 *
 * TWE-Lite のデジタル入力 1 (DI1) が Lo に変化したらブロードキャストを行い
 * パケットを受信したら一定時間 デジタル出力 4 (DO4) を Hi にする
 *
 * ふたつの TWE-Lite モジュールを用意してそれぞれ DI1 にタクトスイッチ、
 * DO4 に LED を接続して動作を観察する
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
#define DO4   9          // デジタル出力 4
#define UART_BAUD 115200 // シリアルのボーレート

// ToCoNet 用パラメータ
#define APP_ID   0x33333333
#define CHANNEL  18

static tsFILE sSerStream;          // シリアル用ストリーム
static tsSerialPortSetup sSerPort; // シリアルポートデスクリプタ
static uint32 u32Seq;              // 送信パケットのシーケンス番号

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
	vPortAsOutput(DO4);
	vPortSetLo(DO4);
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
	static bool_t count = 0;

	// 起動時
	if (eEvent == E_EVENT_START_UP) {
		dbg("** Test04 **");
	}
	// データ受信時に DO4 を Hi に
	if (eEvent == E_ORDER_KICK) {
		count = 100;
		vPortSetHi(DO4);
	}
	 // 4ms 周期のシステムタイマ通知
	if (eEvent == E_EVENT_TICK_TIMER) {
		if (count > 0) {
			count--;
		} else {
			// DO4 が Hi なら Lo に
			if (!bPortRead(DO4)) {
				vPortSetLo(DO4);
			}
		}
	}
	return;
}

//
// 以下 ToCoNet 既定のイベントハンドラ群
//

// 割り込み発生後に随時呼び出される
void cbToCoNet_vMain(void)
{
	static bool_t stsPrev = TRUE;

	bool_t sts = bPortRead(DI1);
	if (sts != stsPrev) {
		// DI1 が Hi -> Lo に変化したら送信を行う
		if (sts) {
			sendBroadcast();
		}
		stsPrev = sts;
		dbg("DI1 is [%s]", (sts) ? "Lo" : "Hi");
	}
	return;
}

// パケット受信時
void cbToCoNet_vRxEvent(tsRxDataApp *pRx)
{
	static uint32 u32SrcAddrPrev = 0;
	static uint8 u8seqPrev = 0xFF;

	// 前回と同一の送信元＋シーケンス番号のパケットなら受け流す
	if (pRx->u32SrcAddr == u32SrcAddrPrev && pRx->u8Seq == u8seqPrev) {
		return;
	}
	// ペイロードを切り出してデバッグ出力
	char buf[64];
	int len = (pRx->u8Len < sizeof(buf)) ? pRx->u8Len : sizeof(buf)-1;
	memcpy(buf, pRx->auData, len);
	buf[len] = '\0';
	dbg("RECV << [%s]", buf);

	u32SrcAddrPrev = pRx->u32SrcAddr;
	u8seqPrev = pRx->u8Seq;

	// DO4 を一定時間 Hi にする
	ToCoNet_Event_Process(E_ORDER_KICK, 0, vProcessEvCore);
	return;
}

// パケット送信完了時
void cbToCoNet_vTxEvent(uint8 u8CbId, uint8 bStatus)
{
	dbg(">> SEND %s seq=%u", bStatus ? "OK" : "NG", u32Seq);
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
		// SPRINTF 初期化
		SPRINTF_vInit128();

		// ToCoNet パラメータ
		sToCoNet_AppContext.u32AppId = APP_ID;
		sToCoNet_AppContext.u8Channel = CHANNEL;
		sToCoNet_AppContext.bRxOnIdle = TRUE; // アイドル時にも受信
		u32Seq = 0;

		// ユーザ定義のイベントハンドラを登録
		ToCoNet_Event_Register_State_Machine(vProcessEvCore);

		// ハードウェア初期化
		vInitHardware();

		// MAC 層開始
		ToCoNet_vMacStart();
	}
}

// ウォームスタート時
void cbAppWarmStart(bool_t bAfterAhiInit)
{
	if (!bAfterAhiInit) {
	} else {
		vInitHardware();
		ToCoNet_vMacStart();
	}
}

