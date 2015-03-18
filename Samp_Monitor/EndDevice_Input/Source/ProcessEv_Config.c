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
 * 設定処理用の状態マシン(特に何もしない。SERIAL の入力で何かする)
 */
static void vProcessEvCore(tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	switch (pEv->eState) {
		case E_STATE_IDLE:
			if (eEvent == E_EVENT_START_UP) {
				Interactive_vSetMode(TRUE,0);
				vSerInitMessage();
				vfPrintf(&sSerStream, LB LB "*** Entering Config Mode ***");

				sAppData.sFlash.sData.u16RcClock = ToCoNet_u16RcCalib(0);

				ToCoNet_Event_SetState(pEv, E_STATE_RUNNING);
			}
			break;

		case E_STATE_RUNNING:
			break;

		default:
			break;
	}
}

/**
 * メイン処理
 */
static void cbAppToCoNet_vMain() {
	/* handle serial input */
	vHandleSerialInput();
}

/**
 * アプリケーションハンドラー定義
 *
 */
static tsCbHandler sCbHandler = {
	NULL, // cbAppToCoNet_u8HwInt,
	NULL, // cbAppToCoNet_vHwEvent,
	cbAppToCoNet_vMain,
	NULL, //cbAppToCoNet_vNwkEvent,
	NULL, // cbAppToCoNet_vRxEvent,
	NULL, //cbAppToCoNet_vTxEvent
};

/**
 * アプリケーション初期化
 */
void vInitAppConfig() {
	psCbHandler = &sCbHandler;
	pvProcessEv1 = vProcessEvCore;
}
