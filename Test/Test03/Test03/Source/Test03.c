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
 * Test03.c
 *
 * Test02.c の内容にシリアルポート経由でのデバッグメッセージ出力処理を追加
 * ネットワーク通信は行わない
 *
 * 2015 KLab Inc.
 *
 */

#include <AppHardwareApi.h>  // NXP ペリフェラル API 用
#include "utils.h"           // ペリフェラル API のラッパなど
#include "serial.h"          // シリアル用
#include "sprintf.h"         // SPRINTF 用
#include "ToCoNet.h"

#define DI1  12          // デジタル入力 1
#define DO4   9          // デジタル出力 4
#define UART_BAUD 115200 // シリアルのボーレート

static tsFILE sSerStream;          // シリアル用ストリーム
static tsSerialPortSetup sSerPort; // シリアルポートデスクリプタ

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

// ユーザ定義のイベントハンドラ
static void vProcessEvCore(tsEvent *pEv, teEvent eEvent, uint32 u32evarg)
{
	// 起動時にデバッグメッセージを出力
	if (eEvent == E_EVENT_START_UP) {
		dbg("** Test03 **");
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

	// DI1: Lo -> DO4: Hi,  DI1: Hi -> DO4: Lo
	bool_t sts = bPortRead(DI1);
	vPortSet_TrueAsLo(DO4, !sts);

	// DI1 の状態が変化していればデバッグメッセージを出力
	if (sts != stsPrev) {
		stsPrev = sts;
		dbg("DI1 is [%s]", (sts) ? "Lo" : "Hi");
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
	} else {
		// ユーザ定義のイベントハンドラを登録
		ToCoNet_Event_Register_State_Machine(vProcessEvCore);
		vInitHardware();
	}
}

// ウォームスタート時
void cbAppWarmStart(bool_t bAfterAhiInit)
{
	return;
}
