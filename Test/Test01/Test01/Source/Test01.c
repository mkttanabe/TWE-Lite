/*
 * Test01.c
 *
 * TWE-Lite のデジタル出力 4 (DO4) ポートの Lo, Hi を一定周期でトグルする
 * いわゆる "Lチカ" 用
 * ネットワーク通信は行わない
 *
 * 2015 KLab Inc.
 *
 */

#include <AppHardwareApi.h>  // NXP ペリフェラル API 用
#include "utils.h"           // ペリフェラル API のラッパなど
#include "ToCoNet.h"

#define DO4  9 // デジタル出力 4

// ユーザ定義のイベントハンドラ
static void vProcessEvCore(tsEvent *pEv, teEvent eEvent, uint32 u32evarg)
{
	 // 1 秒周期のシステムタイマ通知
	if (eEvent == E_EVENT_TICK_SECOND) {
		// DO4 の Lo / Hi をトグル
		bPortRead(DO4) ? vPortSetHi(DO4) : vPortSetLo(DO4);
	}
}

//
// 以下 ToCoNet 既定のイベントハンドラ群
//

// 割り込み発生後に随時呼び出される
void cbToCoNet_vMain(void)
{
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
		vPortAsOutput(DO4);
	}
}

// ウォームスタート時
void cbAppWarmStart(bool_t bAfterAhiInit)
{
	return;
}
