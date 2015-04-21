/*
 * Test02.c
 *
 * TWE-Lite のデジタル入力 1 (DI1) の状態にデジタル出力 4 (DO4) を連動させる
 *
 * DI1: Lo -> DO4: Hi
 * DI1: Hi -> DO4: Lo
 *
 * DI1 にタクトスイッチ、DO4 に LED を接続して動作を観察する
 * ネットワーク通信は行わない
 *
 * 2015 KLab Inc.
 *
 */

#include <AppHardwareApi.h>  // NXP ペリフェラル API 用
#include "utils.h"           // ペリフェラル API のラッパなど
#include "ToCoNet.h"

#define DI1  12  // デジタル入力 1
#define DO4   9  // デジタル出力 4

// ハードウェア初期化
static void vInitHardware()
{
	// 使用ポートの設定
	vPortAsInput(DI1);
	vPortAsOutput(DO4);
	vPortSetLo(DO4);
}

// ユーザ定義のイベントハンドラ
static void vProcessEvCore(tsEvent *pEv, teEvent eEvent, uint32 u32evarg)
{
	return;
}

//
// 以下 ToCoNet 既定のイベントハンドラ群
//

// 割り込み発生後に随時呼び出される
void cbToCoNet_vMain(void)
{
	// DI1: Lo -> DO4: Hi,  DI1: Hi -> DO4: Lo
	vPortSet_TrueAsLo(DO4, !bPortRead(DI1));
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
