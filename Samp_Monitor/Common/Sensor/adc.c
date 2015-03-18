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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <string.h>

#include <jendefs.h>
#include <AppHardwareApi.h>

// #include "event.h"
#include "ToCoNet_event.h"
#include "adc.h"

#undef SERIAL_DEBUG
#ifdef SERIAL_DEBUG
#include <serial.h>
#include <fprintf.h>
extern tsFILE sSerStream;
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#undef USE_TEMP_RAW // 温度センサーの値を ADC 生値 (但し 12bit 拡張) で記録
#ifdef USE_TEMP_RAW
# warning "EXPERIMENTAL: USE_TEMP_RAW"
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vProcessSnsObj_ADC(void *pObj, teEvent eEvent);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/** @ingroup ADC
 * ADC のデバイスIDのテーブル（AHI のヘッダファイルと同じ並びなので、冗長であるが・・・）
 */
PRIVATE const uint8 au8AdcSrcTable[TEH_ADC_IDX_END] = {
	E_AHI_ADC_SRC_ADC_1, // need to check HardwareAPI.h
	E_AHI_ADC_SRC_ADC_2,
	E_AHI_ADC_SRC_ADC_3,
	E_AHI_ADC_SRC_ADC_4,
	E_AHI_ADC_SRC_TEMP,
	E_AHI_ADC_SRC_VOLT
};

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/** @ingroup ADC
 * ADCの初期化処理
 * - 構造体の初期化
 * - アナログ部の電源投入 ※ 直後の動きが怪しいので完了後 1ms 程度の待ちを入れること
 *
 * @param pData 設定および結果を格納する構造体
 * @param pSnsObj センサー入力イベント処理管理構造体
 * @param bInitAPR TRUEならアナログ部の電源投入初期化処理を行う
 */
void vADC_Init(tsObjData_ADC *pData, tsSnsObj *pSnsObj, bool_t bInitAPR) {
	vSnsObj_Init(pSnsObj);

	pSnsObj->pvData = (void*)pData;
	pSnsObj->pvProcessSnsObj = (void*)vProcessSnsObj_ADC;

	memset((void*)pData, 0, sizeof(tsObjData_ADC));

	// clear sensor data as "not yet"
	int i;
	for (i = 0; i < TEH_ADC_IDX_END; i++) {
		pData->ai16Result[i] = SENSOR_TAG_DATA_NOTYET;
	}

	// Initialize ADC
	if (bInitAPR) {
		if (!bAHI_APRegulatorEnabled()) {
			vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
							 E_AHI_AP_INT_ENABLE,
							 E_AHI_AP_SAMPLE_2,
							 E_AHI_AP_CLOCKDIV_500KHZ,
							 E_AHI_AP_INTREF);

		}
	}
}

/** @ingroup ADC
 * ADCのアナログ回路の初期化待ち
 */
void vADC_WaitInit() {
	while(!bAHI_APRegulatorEnabled());
}

/** @ingroup ADC
 * ADCの後始末処理
 * - スリープ前などに行う必要は無く、スリープ復帰後は vADC_Init() からやり直せばよい。
 *
 * @param pData 設定および結果を格納する構造体
 * @param pSnsObj センサー入力イベント処理管理構造体
 * @param bDeinitAPR アナログ部の無効化処理
 */
void vADC_Final(tsObjData_ADC *pData, tsSnsObj *pSnsObj, bool_t bDeinitAPR) {
	pSnsObj->u8State = E_SNSOBJ_STATE_INACTIVE;

	// De-initialize ADC
	if (bDeinitAPR) {
		vAHI_ApConfigure(E_AHI_AP_REGULATOR_DISABLE,
						 E_AHI_AP_INT_DISABLE,
						 E_AHI_AP_SAMPLE_2,
						 E_AHI_AP_CLOCKDIV_500KHZ,
						 E_AHI_AP_INTREF);
	}
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/** @ingroup ADC
 * ADC処理のイベント処理部
 *
 * - 本イベント処理では ADC の状態まで判定せず、ADCが終了した時点で E_ORDER_KICK イベントを
 *   投入する事で状態を遷移させる。
 * - IDLE 状態に手 E_ORDER_KICK イベントで ADC を開始する。
 * - 以後、E_ORDER_KICK イベント毎に入力チャネルを変更して ADC を行う。
 * - COMPLETE 状態になってから E_ORDER_KICK を行えば、IDLE 状態に戻る。
 * - ※ アナログ部の初期化 vAHI_ApConfigure() を済ませていないと、ハングアップする。特にスリープ復帰後
 *     に手続きを忘れることが有るので注意。
 * - ※ アナログ割り込みにて ADC 完了を検知し、E_ORDER_KICK イベントを発行することを期待して設計したが、
 *     このやり方では ADC 部が原因不明のスタックする事がある。このため、KICK 後、十分長い時間を待ってから
 *     KICKするという処理を行ように。
 */
static void vProcessSnsObj_ADC(void *pvObj, teEvent eEvent) {
	tsSnsObj *pSnsObj = (tsSnsObj *)pvObj;
	tsObjData_ADC *pObj = (tsObjData_ADC *)pSnsObj->pvData;

	switch(pSnsObj->u8State)
	{
	case E_SNSOBJ_STATE_INACTIVE:
		break;

	case E_SNSOBJ_STATE_IDLE:
		if (eEvent == E_ORDER_KICK) {
#ifdef SERIAL_DEBUG
vfPrintf(&sSerStream, "\n\rE_ADC KICKED %x", pObj->u8SourceMask);
#endif

			vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_MEASURING);
		}
		break; //E_SNSOBJ_STATE_IDLE//

	case E_SNSOBJ_STATE_MEASURING: {
		uint8 u8Src;
		u8Src = 1 << pObj->u8IdxMeasuruing;
#ifdef SERIAL_DEBUG
vfPrintf(&sSerStream, "\n\rIDX %x, SRC %d", pObj->u8IdxMeasuruing, u8Src);
#endif
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			if (u8Src & pObj->u8SourceMask) {
				uint8 u8rng;
				u8rng = pObj->u8InputRangeMask & u8Src ?
						E_AHI_AP_INPUT_RANGE_1 : E_AHI_AP_INPUT_RANGE_2;

				if (u8Src == TEH_ADC_SRC_TEMP) {
					u8rng = E_AHI_AP_INPUT_RANGE_1;
				}

				if (u8Src == TEH_ADC_SRC_VOLT) {
					u8rng = E_AHI_AP_INPUT_RANGE_2;
				}
#ifdef SERIAL_DEBUG
vfPrintf(&sSerStream, "\n\rE_ADC STARTED %x", au8AdcSrcTable[pObj->u8IdxMeasuruing]);
#endif
				vAHI_AdcEnable(
						E_AHI_ADC_SINGLE_SHOT,
						u8rng,
						au8AdcSrcTable[pObj->u8IdxMeasuruing]);

				vAHI_AdcStartSample();

				pObj->bBusy = TRUE;
				break;
			} else {
				// skip!
				vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_MEASURE_NEXT);
			}
			break;

		case E_ORDER_KICK:
			// ADC が完了したので、結果データの格納と次のチャネルの ADC 開始処理 (E_SNSOBJ_STATE_MEASURE_NEXT 遷移) を行う。

			// save ADC value
			pObj->ai16Result[pObj->u8IdxMeasuruing] = (int16)u16AHI_AdcRead();

#ifdef JN516x
			// 10bit を12bit にシフトしておく (12bit ADC の計算式と混在するとややこしいので)
			pObj->ai16Result[pObj->u8IdxMeasuruing] <<= 2; // convert to 12bit
#endif
#ifdef SERIAL_DEBUG
			vfPrintf(&sSerStream, "\n\rADC COMPLETE: SRC(%d)->%d", pObj->u8IdxMeasuruing, pObj->ai16Result[pObj->u8IdxMeasuruing]);
#endif

			// 電源電圧の変換  (ADCVAL -> mV)
			// 基本的に 2~3.6V を ADC のフルスケールとして測定できるような分圧回路が半導体内部に構成される。
			// つまりADC値が0なら 2000mV, 4095 なら 3600mV となる。
			if (   pObj->u8IdxMeasuruing == TEH_ADC_IDX_VOLT
				&& !IS_SENSOR_TAG_DATA_ERR(pObj->ai16Result[TEH_ADC_IDX_VOLT])) {
				int16 i16AdcVal = pObj->ai16Result[TEH_ADC_IDX_VOLT];

#ifdef JN514x
				pObj->ai16Result[TEH_ADC_IDX_VOLT] =
					(int32)(i16AdcVal) * 100L / 114L; // in mV
#elif defined(JN516x)
				// TWE-Lite 用
				// データシートの情報に基づくなら、2/3 に分圧されていますから、
				// 10bit最大を 2470mV として mV 値に変換した上、1.5 倍する計算
				// を行います。
				pObj->ai16Result[TEH_ADC_IDX_VOLT] = ((int32)(i16AdcVal) * 3705) >> 12;
#endif
			} else
			// 内蔵温度センサーの変換 (ADCVAL -> 100x degC 23.55℃なら 2355 に変換する)
			if (   pObj->u8IdxMeasuruing == TEH_ADC_IDX_TEMP
				&& !IS_SENSOR_TAG_DATA_ERR(pObj->ai16Result[TEH_ADC_IDX_TEMP])) {
#ifndef USE_TEMP_RAW // USE_TEMP_RAW が定義された場合は、そのままの AD 値を採用
				int16 i16AdcVal = pObj->ai16Result[TEH_ADC_IDX_TEMP];
#ifdef JN514x
				pObj->ai16Result[TEH_ADC_IDX_TEMP] =
					(25000L - (((int32)(i16AdcVal - 2560L)) * 183L) + 5L) / 10L; // in 100x degC
#elif defined(JN516x)
				// TWE-Lite ではデータシートからの計算を行っているが、実測値と大きな隔たりがある。下記の価は利用できない。
				// 25 - (AdcVal<<2 - 2421[730mV]) * 0.1765
				// 25000 - (AdcVal<<2 - 2421) * 177
				pObj->ai16Result[TEH_ADC_IDX_TEMP] =
					(25000L - (((int32)(i16AdcVal - 2421L)) * 177L) + 5L) / 10L; // in 100x degC
#endif
#endif // USE_TEMP_RAW
			} else {
				// ADC1-4の値。mVで計算する。
				//   514x(Regular/Strong) と 516x(Lite) ではバンドギャップ電圧が違うので計算式も違う。
				//   相対スケールのADCも欲しい・・・
				if (pObj->u8InputRangeMask & u8Src) {
					// 0-1200mV
					pObj->ai16Result[pObj->u8IdxMeasuruing] =
#ifdef JN514x
						pObj->ai16Result[pObj->u8IdxMeasuruing] * 1200 / 4096;
#elif defined(JN516x)
						pObj->ai16Result[pObj->u8IdxMeasuruing] * 1235 / 4096;
#endif
				} else {
					// 0-2400mV
					pObj->ai16Result[pObj->u8IdxMeasuruing] =
#ifdef JN514x
						pObj->ai16Result[pObj->u8IdxMeasuruing] * 2400 / 4096;
#elif defined(JN516x)
						pObj->ai16Result[pObj->u8IdxMeasuruing] * 2470 / 4096;
#endif
				}
			}

			vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_MEASURE_NEXT);
			break;

		default:
			break;
		}
	}	break; //E_SNSOBJ_STATE_MEASURING//

	case E_SNSOBJ_STATE_MEASURE_NEXT:
		// ADCの終了判定と、次のチャネルのADC開始
#ifdef SERIAL_DEBUG
vfPrintf(&sSerStream, "\n\rADC NEXT(%d)", pObj->u8IdxMeasuruing);
#endif
		switch (eEvent) {
			case E_EVENT_NEW_STATE:
				pObj->u8IdxMeasuruing++;

				if (pObj->u8IdxMeasuruing < TEH_ADC_IDX_END) {
					// complete all data
					vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_MEASURING);
				} else {
					// complete all data
					vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_COMPLETE);
				}
				break;

			default:
				break;
		}
		break; //E_SNSOBJ_STATE_MEASURE_NEXT//

	case E_SNSOBJ_STATE_COMPLETE:
		// 全処理完了状態。
		//   KICK イベントで、IDLE 状態に巻き戻す。
		switch (eEvent) {
		case E_EVENT_NEW_STATE:
			pObj->bBusy = FALSE;
			pObj->u8IdxMeasuruing = 0;

			#ifdef SERIAL_DEBUG
			vfPrintf(&sSerStream, "\n\rADC ALL COMPLETE");
			#endif
			break;

		case E_ORDER_KICK:
			vSnsObj_NewState(pSnsObj, E_SNSOBJ_STATE_IDLE);
			break;
		default:
			break;
		}

		break; //E_SNSOBJ_STATE_COMPLETE//

	default:
		break;
	}
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
