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

#ifndef APPDATA_H_
#define APPDATA_H_

#include <jendefs.h>

#include "config.h"
#include "ToCoNet.h"
#include "flash.h"
#include "sensor_driver.h"

#include "adc.h"

typedef struct {
	// sensor data
	uint8 u8Batt;
	uint16 u16Adc1, u16Adc2;
	uint16 u16PC1, u16PC2;
	uint16 u16Temp, u16Humid;
} tsSensorData;

typedef struct {
	// ToCoNet version
	uint32 u32ToCoNetVersion;
	uint8 u8DebugLevel; //!< ToCoNet のデバッグ

	// wakeup status
	bool_t bWakeupByButton; //!< DIO で RAM HOLD Wakeup した場合 TRUE

	// Network context
	tsToCoNet_Nwk_Context *pContextNwk;
	tsToCoNet_NwkLyTr_Config sNwkLayerTreeConfig;
	uint8 u8NwkStat;

	// frame count
	uint16 u16frame_count;

	// ADC
	tsObjData_ADC sObjADC; //!< ADC管理構造体（データ部）
	tsSnsObj sADC; //!< ADC管理構造体（制御部）
	uint8 u8AdcState; //!< ADCの状態 (0xFF:初期化前, 0x0:ADC開始要求, 0x1:AD中, 0x2:AD完了)

	// config mode
	bool_t bConfigMode; // 設定モード

	// センサ情報
	tsSensorData sSns;
	uint32 u32DI1_Dur_Opened_ms; //!< DI1 が Hi になってからの経過時間
	uint16 u16DI1_Ct_PktFired; //! DI1 の無線パケット発射回数
	bool_t bDI1_Now_Opened; //!< DI1 が Hi なら TRUE

	// その他
	tsFlash sFlash; //!< フラッシュの情報
	bool_t bFlashLoaded; //!< フラッシュにデータが合った場合は TRUE
} tsAppData_Ed;

extern tsAppData_Ed sAppData_Ed;

#endif /* APPDATA_H_ */
