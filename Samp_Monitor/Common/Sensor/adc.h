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

#ifndef  ADC_H_INCLUDED
#define  ADC_H_INCLUDED

/** @file
 *
 * @defgroup ADC ADCの変換・読み出し関数群
 * ADC の変換、読み出し処理を行う。
 *
 * - 利用法
 *   - tsSnsObj, tsObjData_ADC 構造体を用意し vADC_Init() を実行します。本関数では AP レギュレータの稼働開始も可能です。
 *   - tsObjData_ADC で ADC を行うチャネルや測定レンジを指定します。
 *   - vSnsObj_Process() により E_ORDER_KICK イベントを送ります。
 *     - ADC が開始します。 ADC完了まで時間待ちを行ってください。ADC完了割り込みも発生しますが、割り込み発生後からさらに1ms程度待つようにしてください。
 *     - tsSnsObj 構造体に格納される状態が E_SNSOBJ_STATE_COMPLETE でないなら、再び E_ORDER_KICK イベントを送り時間待ちをし、状態チェックを繰り返します。
 *     - E_SNSOBJ_STATE_COMPLETE 状態になれば tsObjData_ADC 構造体から値を得られます。
 *
 * - 測定値について
 *   - AD1～AD4,電源電圧は mV で報告されます。
 *   - 温度は 100 倍した摂氏で報告されます (JN5148のみ)
 *
 * - 注意
 *   - ADC の変換を連続的に行うと、ADCがスタックする事が有ります。
 *     - タイマーにより稼働させ定周期とし時間を空けて、駆動開始、読み出し＆次のチャネルの駆動といった処理を行ってください。
 *       現時点では回避方法が時間待ち以外に判っていません。
 *   - bAHI_APRegulatorEnabled() による開始待ちを行っていますが、問題が有る可能性が有ります。(ハングアップや最初の変換が正しくない）
 *     - 念のため 1ms 程度時間を置いてから最初の AD を開始してください。
 *   - JN516x (TWE-Lite) での温度センサーの変換係数が判明していません。温度センサーの結果は利用できません。
 *
 *   - 単一のチャネルを連続して読みたい場合は本コードの使用を行うより、AHI の関連 API をそのまま使用する事を推奨します。
 *
 */

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include "sensor_driver.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define ADC_DEFAULT_25C_OFFSET      2560U //!< 基準温度 @ingroup ADC

#define TEH_ADC_SRC_ADC_1    (1<<0) //!< ADC1の指定マスク @ingroup ADC
#define TEH_ADC_SRC_ADC_2    (1<<1) //!< ADC2の指定マスク @ingroup ADC
#define TEH_ADC_SRC_ADC_3    (1<<2) //!< ADC3の指定マスク @ingroup ADC
#define TEH_ADC_SRC_ADC_4    (1<<3) //!< ADC4の指定マスク @ingroup ADC
#define TEH_ADC_SRC_TEMP     (1<<4) //!< 温度センサーの指定マスク @ingroup ADC
#define TEH_ADC_SRC_VOLT     (1<<5) //!< 電源電圧の指定マスク @ingroup ADC

#define TEH_ADC_IDX_ADC_1    0 //!< ai16Result[] の ADC1 格納位置 @ingroup ADC
#define TEH_ADC_IDX_ADC_2    1 //!< ai16Result[] の ADC2 格納位置 @ingroup ADC
#define TEH_ADC_IDX_ADC_3    2 //!< ai16Result[] の ADC3 格納位置 @ingroup ADC
#define TEH_ADC_IDX_ADC_4    3 //!< ai16Result[] の ADC4 格納位置 @ingroup ADC
#define TEH_ADC_IDX_TEMP     4 //!< ai16Result[] の温度センサー格納位置 @ingroup ADC
#define TEH_ADC_IDX_VOLT     5 //!< ai16Result[] の電源電圧格納位置 @ingroup ADC

#define TEH_ADC_IDX_BEGIN    0 //!< ai16Result[] の先頭インデックス @ingroup ADC
#define TEH_ADC_IDX_END      6 //!< ai16Result[] の末尾＋１インデックス @ingroup ADC

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/** @ingroup ADC
 * ADC設定・データ格納構造体
 */
typedef struct
{
	bool_t    bBusy; //!< ビジーフラグ（未使用）

	uint8     u8SourceMask;     //!< AD対象のチャネルマスク THH_ADC_SRC_* のマスクで指定する
	uint8     u8InputRangeMask; //!< 測定レンジ 0: 2Vref(0-2.4V), 1: Vref (0-1.2V)

	uint8     u8IdxMeasuruing; //!< 内部管理
	int16     ai16Result[TEH_ADC_IDX_END]; //!< ADC 結果格納 (-32760 以下の値は未格納またはエラー)

} tsObjData_ADC;


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
void vADC_Init(tsObjData_ADC *pData, tsSnsObj *pSnsObj, bool_t bInitAPR);
void vADC_Final(tsObjData_ADC *pData, tsSnsObj *pSnsObj, bool_t bDeinitAPR);
void vADC_WaitInit();

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* ADC_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
