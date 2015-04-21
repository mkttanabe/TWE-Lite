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
#include "sercmd_gen.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define IS_TRANSMIT_CHAR(c) (c=='\t' || ((c)>=0x20 && (c)!=0x7F)) // 送信対象文字列か判定

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/** @ingroup SERCMD
 * シリアルコマンド（アスキー形式）の状態定義
 */
typedef enum {
	E_SERCMD_CHAT_CMD_EMPTY = 0,      //!< 入力されていない
	E_SERCMD_CHAT_CMD_READPAYLOAD,    //!< E_SERCMD_CHAT_CMD_READPAYLOAD
	E_SERCMD_CHAT_CMD_READCR,         //!< E_SERCMD_CHAT_CMD_READCR
	E_SERCMD_CHAT_CMD_READLF,         //!< E_SERCMD_CHAT_CMD_READLF
	E_SERCMD_CHAT_CMD_COMPLETE = 0x80,//!< 入力が完結した(LCRチェックを含め)
	E_SERCMD_CHAT_CMD_ERROR = 0x81,          //!< 入力エラー
	E_SERCMD_CHAT_CMD_CHECKSUM_ERROR = 0x82,       //!< LRCが間違っている
} teSercmdAsciiState;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************///
extern uint32 u32TickCount_ms; //!< ToCoNet での TickTimer

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/** @ingroup SERCMD
 * シリアルコマンドアスキー形式の入力系列の解釈を行う。\n
 * - :[0-9A-Z]+(CRLF) 系列の入力を行い、LRCチェックを行う
 *
 * @param pCmd 管理構造体
 * @param u8byte 入力文字
 * @return 状態コード (teModbusCmdState 参照)
 */
uint8 SerCmdChat_u8Parse(tsSerCmd_Context *pCmd, uint8 u8byte) {
	// check for complete or error status
	if (pCmd->u8state >= 0x80) {
		pCmd->u8state = E_SERCMD_CHAT_CMD_EMPTY;
	}

	// run state machine
	switch (pCmd->u8state) {
	case E_SERCMD_CHAT_CMD_EMPTY:
		if (IS_TRANSMIT_CHAR(u8byte)) {
			pCmd->u8state = E_SERCMD_CHAT_CMD_READPAYLOAD;
			pCmd->au8data[0] = u8byte;

			pCmd->u32timestamp = u32TickCount_ms; // store received time for timeout
			pCmd->u16pos = 1;
			pCmd->u16len = 1;
			pCmd->u16cksum = 0;
			// memset(pCmd->au8data, 0, pCmd->u16maxlen);
		}
		break;

	case E_SERCMD_CHAT_CMD_READPAYLOAD:
		if (IS_TRANSMIT_CHAR(u8byte)) {
			pCmd->au8data[pCmd->u16pos] = u8byte;

			pCmd->u16pos++;
			pCmd->u16len++;

			// 最大長のエラー
			if (pCmd->u16pos == pCmd->u16maxlen - 1) {
				pCmd->u8state = E_SERCMD_CHAT_CMD_ERROR;
			}
		} else if (u8byte == 0x0d || u8byte == 0x0a) { // CR入力
			if (pCmd->u16pos) { // 入力が存在する場合
				pCmd->u8state = E_SERCMD_CHAT_CMD_COMPLETE;
			} else { // 何も入力されなかった場合は、とりあえずエラー
				pCmd->u8state = E_SERCMD_CHAT_CMD_ERROR;
			}
		} else if (u8byte == 0x08 || u8byte == 0x7F) {
			// BS/DEL の内部処理 (ターミナル上での処理は、アプリケーションが行う事)
			if (pCmd->u16pos) {
				pCmd->u16pos--;
				pCmd->u16len--;
			}
			if (pCmd->u16pos == 0) {
				pCmd->u8state = E_SERCMD_CHAT_CMD_EMPTY;
			}
		} else { // その他コントロールコードを受けた場合は、EMPTY
			pCmd->u8state = E_SERCMD_CHAT_CMD_EMPTY;
		}
		break;

	default:
		break;
	}

	return pCmd->u8state;
}

/** @ingroup SERCMD
 * 出力 (vOutputメソッド用)
 * - そのまま系列を出力する。
 *
 * @param pc
 * @param ps
 */
static void SerCmdChat_Output(tsSerCmd_Context *pc, tsFILE *ps) {
	int i;

	// NULL buffer
	if (pc == NULL || pc->au8data == NULL || pc->u16len == 0) return;

	for (i = 0; i < pc->u16len; i++) {
		vPutChar(ps, pc->au8data[i]);
	}
}

/** @ingroup SERCMD
 * 解析構造体を初期化する。
 *
 * @param pc
 * @param pbuff
 * @param u16maxlen
 */
void SerCmdChat_vInit(tsSerCmd_Context *pc, uint8 *pbuff, uint16 u16maxlen) {
	memset(pc, 0, sizeof(tsSerCmd_Context));

	pc->au8data = pbuff;
	pc->u16maxlen = u16maxlen;

	pc->u8Parse = SerCmdChat_u8Parse;
	pc->vOutput = SerCmdChat_Output;
}
