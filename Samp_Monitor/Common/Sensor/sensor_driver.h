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

/** @file
 *
 * @defgroup SNSDRV センサー入力処理
 * センサー入力時の入力状態処理およびセンサー間の抽象化を行う。
 */

#ifndef SENSOR_DRIVER_H_
#include "jendefs.h"
#include <AppHardwareApi.h>

#include "ToCoNet_event.h"

#define SENSOR_TAG_DATA_NOTYET (-32768) //!< センサー入力がまだ @ingroup SNSDRV
#define SENSOR_TAG_DATA_ERROR  (-32767)  //!< センサー入力がエラー @ingroup SNSDRV
#define IS_SENSOR_TAG_DATA_ERR(c) (c < -32760) //!< その他エラー @ingroup SNSDRV

/** @ingroup SNSDRV
 * センサーの処理状態
 */
typedef enum
{
	E_SNSOBJ_STATE_IDLE = 0x00,    //!< 開始前
	E_SNSOBJ_STATE_CALIBRATE,      //!< 調整中
	E_SNSOBJ_STATE_MEASURING,      //!< 測定中
	E_SNSOBJ_STATE_MEASURE_NEXT,   //!< 次の測定処理中
	E_SNSOBJ_STATE_COMPLETE = 0x80,//!< 測定完了
	E_SNSOBJ_STATE_INACTIVE = 0xff //!< 動作しない状態
} teState_SnsObj;

/**  @ingroup SNSDRV
 * 管理構造体
 */
typedef struct {
	uint8 u8State; //!< 状態
	uint8 u8TickDelta; //!< TICKTIMER での経過ms

	void *pvData; //!< センサーデータへのポインタ
	bool_t (*pvProcessSnsObj)(void *, teEvent); //!< イベント処理構造体
} tsSnsObj;

void vSnsObj_Process(tsSnsObj *pObj, teEvent eEv);
void vSnsObj_Init(tsSnsObj *pObj);

#define vSnsObj_NewState(o, s) ((o)->u8State = (s)) //!< 新しい状態へ遷移する @ingroup SNSDRV
#define bSnsObj_isComplete(o) ((o)->u8State == E_SNSOBJ_STATE_COMPLETE) //!< 完了状態か判定する @ingroup SNSDRV

#define SENSOR_DRIVER_H_


#endif /* SENSOR_DRIVER_H_ */
