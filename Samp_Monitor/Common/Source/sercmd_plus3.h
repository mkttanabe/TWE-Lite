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
 * @defgroup SERCMD_PLUS3 プラス入力３回の判定を行う
 *
 */

#ifndef PLUS3_H_
#define PLUS3_H_

#include <jendefs.h>
// Serial options
#include <serial.h>
#include <fprintf.h>

/** @ingroup SERCMD_PLUS3
 * 内部状態
 */
typedef enum {
	E_SERCMD_PLUS3_EMPTY = 0,      //!< 入力されていない
	E_SERCMD_PLUS3_PLUS1,          //!< E_PLUS3_CMD_PLUS1
	E_SERCMD_PLUS3_PLUS2,          //!< E_PLUS3_CMD_PLUS2
	E_SERCMD_PLUS3_ERROR = 0x81,   //!< エラー状態
	E_SERCMD_PLUS3_VERBOSE_OFF = 0x90,     //!< verbose モード ON になった
	E_SERCMD_PLUS3_VERBOSE_ON     //!< verbose モード OFF になった
} tePlus3CmdState;

/** @ingroup SERCMD_PLUS3
 * 管理構造体
 */
typedef struct {
	uint32 u32timestamp; //!< タイムアウトを管理するためのカウンタ
	uint8 u8state; //!< 状態

	bool_t bverbose; //!< TRUE: verbose モード
	uint32 u32timevbs; //!< verbose モードになったタイムスタンプ
} tsSerCmdPlus3_Context;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

uint8 SerCmdPlus3_u8Parse(tsSerCmdPlus3_Context *pCmd, uint8 u8byte);
#define SerCmdPlus3_bVerbose(pCmd) (pCmd->bverbose) //!< VERBOSEモード判定マクロ @ingroup MBUSA

#endif /* MODBUS_ASCII_H_ */
