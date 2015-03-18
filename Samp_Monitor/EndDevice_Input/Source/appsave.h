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

#ifndef APPSAVE_H_
#define APPSAVE_H_

#include <jendefs.h>

/** @ingroup FLASH
 * フラッシュ格納データ構造体
 */
typedef struct _tsFlashApp {
	uint32 u32appkey;		//!<
	uint32 u32ver;			//!<

	uint32 u32appid;		//!< アプリケーションID
	uint32 u32chmask;		//!< 使用チャネルマスク（３つまで）

	uint8 u8id;				//!< 論理ＩＤ (子機 1～100まで指定)
	uint8 u8ch;				//!< チャネル（未使用、チャネルマスクに指定したチャネルから選ばれる）
	uint8 u8pow;			//!< 出力パワー (0-3)

	uint8 u8wait;			//!< センサーの時間待ち

	uint32 u32Slp;			//!< スリープ周期

	uint32 u32EncKey;		//!< 暗号化キー(128bitだけど、32bitの値から鍵を生成)
	uint32 u32Opt;			//!< 色々オプション

	uint16 u16RcClock;		//!< RCクロックキャリブレーション

	uint8 u8mode;			//!< センサの種類
	int16 i16param;		//!< 選択したセンサ特有のパラメータ
} tsFlashApp;


#endif /* APPSAVE_H_ */
