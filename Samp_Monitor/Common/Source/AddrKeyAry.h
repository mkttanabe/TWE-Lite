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
 * @defgroup ADDRKEYA アドレスとキーを格納する配列
 *
 * パケットの重複チェックを行うためのデータベース。\n
 * 格納する情報はアドレス 32bit 値(0以外)、紐付けされ 32bit キー値、格納された時のタイムスタンプを同時に保存する。
 * 紐付け値に例えばタイムスタンプや続き番号などを格納し、重複しているかの判定を行うことを目的とする。
 * 一定時間過ぎたものはタイムアウトとしデータベースから抹消する。
 * 本アルゴリズムでは、探索数が多くないため単純な線形検索を行っている。
 *
 * - 利用法:
 *   - 事前に tsAdrKeyA_Context 構造体を用意し DUPCHK_vInit() で初期化しておく
 *   - 定期的に ADRKEYA_bFind(..,0,NULL) を実行しタイムアウト処理を行う
 *   - 対象となるパケットを受信したとき、ADRKEYA_bFind()により対象のアドレスから紐付け情報を得る。
 *     - 紐付けキー情報が無ければ、新しいパケットなので採用する
 *     - 紐付けキー情報が有れば、これが新規のものか既に処理したものかアプリケーション側で判定する
 *     - 最新のパケットとして採用した場合、ADRKEYA_vAdd() を呼び出しアドレスと紐付けキー情報を保存する。
 */


#ifndef ADDRKEYARY_H_
#define ADDRKEYARY_H_

#include <jendefs.h>
extern uint32 u32TickCount_ms; //!< TOCONETのタイムスタンプ @ingroup DUPCHK

/** @ingroup DUPCHK
 * ノードリストをクリアする数
 */
#define ADDRKEYA_TIMEOUT 60000

/** @ingroup DUPCHK
 * ノードリストの要素数
 */
#define ADDRKEYA_MAX_HISTORY 12

/** @ingroup DUPCHK
 * データベース
 */
typedef struct {
	uint32 au32ScanListAddr[ADDRKEYA_MAX_HISTORY]; //!< ノードリスト
	uint32 au32ScanListTick[ADDRKEYA_MAX_HISTORY]; //!< 最後に追加したタイムスタンプ
	uint32 au32ScanListKey[ADDRKEYA_MAX_HISTORY]; //!< キー情報
} tsAdrKeyA_Context;

void ADDRKEYA_vInit(tsAdrKeyA_Context *p);
bool_t ADDRKEYA_bFind(tsAdrKeyA_Context *p, uint32 u32Addr, uint32 *pu32Key);
void ADDRKEYA_vAdd(tsAdrKeyA_Context *p, uint32 u32Addr, uint32 u32Key);

#endif /* ADDRKEYARY_H_ */
