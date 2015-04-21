/****************************************************************************
 * (C) Tokyo Cosmos Electric, Inc. (TOCOS) - 2013 all rights reserved.
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


#include "jendefs.h"
#include <string.h>

#include <AppHardwareApi.h>

#include "ToCoNet.h"
#include "ToCoNet_event.h"
#include "sensor_driver.h"

/** @ingroup SNSDRV
 * センサー状態マシンを初期化する
 *
 * @param pObj 管理構造体
 */
void vSnsObj_Init(tsSnsObj *pObj) {
	memset(pObj, 0, sizeof(tsSnsObj));
	pObj->u8TickDelta = 1000 / sToCoNet_AppContext.u16TickHz;
}

/** @ingroup SNSDRV
 *
 * 状態を遷移し、遷移後の状態が同じであれば終了。
 * 状態に変化が有れば E_EVENT_NEW_STATE イベントにて、再度関数を呼び出す。
 *
 * @param pObj センサ状態構造体
 * @param eEv イベント
 */
void vSnsObj_Process(tsSnsObj *pObj, teEvent eEv) {
	uint8 u8status_init;

	u8status_init = pObj->u8State;
	pObj->pvProcessSnsObj(pObj, eEv);

	// 連続的に状態遷移させるための処理。pvProcessSnsObj() 処理後に状態が前回と変化が有れば、
	// 再度 E_EVENT_NEW_STATE により実行する。
	while (pObj->u8State != u8status_init) {
		u8status_init = pObj->u8State;
		pObj->pvProcessSnsObj(pObj, E_EVENT_NEW_STATE);
	}
}

