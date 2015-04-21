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
 * @defgroup SERCMD シリアルコマンドの解釈を行う
 * シリアルコマンドの解釈を行う。解釈部は書式に応じてアスキー形式、バイナリ形式、必要で
 * あるなら本定義に基づき拡張も可能である。
 *
 * - 入力の仕様（アスキー形式）
 *   - :([0-9A-F][0-9A-F])+[0-9A-F][0-9A-F][CR][LF]
 *   - 最後の 2 バイトは LRC となっていてLRCを含む 16進変換したバイトを加算すると 0 になる。\n
 *     つまり LRC は全データの和を取り、符号を反転（二の補数）させたもの。
 * - 入力の仕様（バイナリ形式）
 *   - 0xA5 0x5A [LenH] [LenL] [Payload] [XOR] [EOT]
 *      - LenH: ペイロードの長さ(バイト)の上位バイト (MSB を 0x80 にすること)
 *      - LenL: ペイロードの長さ(バイト)の下位バイト
 *      - Payload: ペイロード
 *      - XOR: ペイロード部の各バイトの XOR 値を用いたチェックサム
 *      - EOT: 省略可能。vOutput() 出力時には付与される。
 *
 * - API 利用方法
 *   - 準備
 *     - 事前に tsSerCmd_Context sSC 構造体および入力バッファデータ領域を確保しておく
 *     - SerCmdAscii_vInit() SerCmdBinary_vInit() 関数により構造体を初期化する
 *     - 入力タイムアウトを設定する場合は tsSerCmd_Context 構造体の u16timeout メンバーの値[ms]を設定する
 *
 *   - 解釈実行
 *     - UART からの入力バイト(u8Byte)を  uint8 u8ret = sSC.u8Parse(&sSC, u8Byte) に入力する
 *     - u8ret の値が 0x80 以上になると、解釈が終了したことを示す
 *        - E_SERCMD_COMPLETE: エラーなく系列が入力された。このとき、入力系列に応じた処理を行う。
 *        - E_SERCMD_ERROR: 解釈エラー
 *        - E_SERCMD_CHECKSUM_ERROR: チェックサムだけ合わなかった。このとき sSC.u16cksum に期待したチェックサム値が格納される。
 *     - 続けて u8Parse() を実行する。直前の状態が終了・エラー状態であっても、新たな系列として解釈を始めます。
 *
 */

#ifndef SERCMD_GEN_H_
#define SERCMD_GEN_H_

#include <jendefs.h>
// Serial options
#include <serial.h>
#include <fprintf.h>

/** @ingroup SERCMD
 * シリアルコマンド解釈の内部状態。
 * - 本状態は各解釈部共通の定義とする
 */
typedef enum {
	E_SERCMD_EMPTY = 0,                //!< 入力されていない
	E_SERCMD_COMPLETE = 0x80,          //!< 入力が完結した(LCRチェックを含め)
	E_SERCMD_ERROR = 0x81,             //!< 入力エラー
	E_SERCMD_CHECKSUM_ERROR = 0x82,   //!< チェックサムが間違っている
} teSerCmdGenState;

/** @ingroup SERCMD
 * シリアルコマンド解釈の管理構造体への型定義
 */
typedef struct _sSerCmd_Context tsSerCmd_Context;

/** @ingroup SERCMD
 * シリアルコマンド解釈の管理構造体
 */
struct _sSerCmd_Context {
	uint16 u16len; //!< ペイロード長
	uint8 *au8data; //!< バッファ（初期化時に固定配列へのポインタを格納しておく)
	uint16 u16maxlen; //!< バッファの最大長

	uint32 u32timestamp; //!< タイムアウトを管理するためのカウンタ
	uint16 u16timeout; //!< 入力系列のタイムアウト (0ならタイムアウト無し)

	uint16 u16pos; //!< 入力位置（内部処理用）
	uint16 u16cksum; //!< チェックサム

	uint8 u8state; //!< 状態

	void *vExtraData; //!< 処理系独特の追加データ

	uint8 (*u8Parse)(tsSerCmd_Context *pc, uint8 u8byte); //!< 入力用の関数
	void (*vOutput)(tsSerCmd_Context *pc, tsFILE *ps); //!< 出力用の関数
};

/*
 * 初期化関数群
 */
void SerCmdBinary_vInit(tsSerCmd_Context *pc, uint8 *pbuff, uint16 u16maxlen);
void SerCmdAscii_vInit(tsSerCmd_Context *pc, uint8 *pbuff, uint16 u16maxlen);
void SerCmdChat_vInit(tsSerCmd_Context *pc, uint8 *pbuff, uint16 u16maxlen);

#endif /* SERCMD_GEN_H_ */
