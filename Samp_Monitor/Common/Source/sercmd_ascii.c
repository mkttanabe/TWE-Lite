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

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/** @ingroup SERCMD
 * シリアルコマンド（アスキー形式）の状態定義
 */
typedef enum {
	E_SERCMD_ASCII_CMD_EMPTY = 0,      //!< 入力されていない
	E_SERCMD_ASCII_CMD_READCOLON,      //!< E_SERCMD_ASCII_CMD_READCOLON
	E_SERCMD_ASCII_CMD_READPAYLOAD,    //!< E_SERCMD_ASCII_CMD_READPAYLOAD
	E_SERCMD_ASCII_CMD_READCR,         //!< E_SERCMD_ASCII_CMD_READCR
	E_SERCMD_ASCII_CMD_READLF,         //!< E_SERCMD_ASCII_CMD_READLF
	E_SERCMD_ASCII_CMD_COMPLETE = 0x80,//!< 入力が完結した(LCRチェックを含め)
	E_SERCMD_ASCII_CMD_ERROR = 0x81,          //!< 入力エラー
	E_SERCMD_ASCII_CMD_CHECKSUM_ERROR = 0x82,       //!< LRCが間違っている
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
uint8 SerCmdAscii_u8Parse(tsSerCmd_Context *pCmd, uint8 u8byte) {
	// check for timeout
	if (pCmd->u16timeout && pCmd->u8state != E_SERCMD_ASCII_CMD_EMPTY) {
		if (u32TickCount_ms - pCmd->u32timestamp > pCmd->u16timeout) {
			pCmd->u8state = E_SERCMD_ASCII_CMD_EMPTY; // start from new
		}
	}

	// check for complete or error status
	if (pCmd->u8state >= 0x80) {
		pCmd->u8state = E_SERCMD_ASCII_CMD_EMPTY;
	}

	// run state machine
	switch (pCmd->u8state) {
	case E_SERCMD_ASCII_CMD_EMPTY:
		if (u8byte == ':') {
			pCmd->u8state = E_SERCMD_ASCII_CMD_READPAYLOAD;

			pCmd->u32timestamp = u32TickCount_ms; // store received time for timeout
			pCmd->u16pos = 0;
			pCmd->u16cksum = 0;
			pCmd->u16len = 0;
			// memset(pCmd->au8data, 0, pCmd->u16maxlen);
		}
		break;

	case E_SERCMD_ASCII_CMD_READPAYLOAD:
		if ((u8byte >= '0' && u8byte <= '9') || (u8byte >= 'A' && u8byte <= 'F')) {
			/* オーバーフローのチェック */
			if (pCmd->u16pos / 2 == pCmd->u16maxlen) {
				pCmd->u8state = E_SERCMD_ASCII_CMD_ERROR;
				break;
			}

			/* 文字の16進変換 */
			const uint8 u8tbl[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15 };
			uint8 u8val = u8tbl[u8byte - '0'];

			/* バイナリ値として格納する */
			uint8 *p = &pCmd->au8data[pCmd->u16pos / 2];

			if (pCmd->u16pos & 1) {
				*p = (*p & 0xF0) | u8val;
				pCmd->u16cksum += *p;
			} else {
				*p = u8val << 4;
			}

			pCmd->u16pos++;
		} else if (u8byte == 0x0d || u8byte == 0x0a) { // CR入力
			if (pCmd->u16pos) {
				// チェックサムの確認
				pCmd->u16cksum &= 0xFF; // チェックサムを 8bit に切り捨てる
				if (pCmd->u16cksum) { // 正しければ 0 になっているはず
					// 格納値
					uint8 u8lrc = pCmd->au8data[pCmd->u16pos / 2 - 1]; // u16posは最後のデータの次の位置
					// 計算値(二の補数の計算、ビット反転+1), デバッグ用に入力系列に対応する正しいLRCを格納しておく
					pCmd->u16cksum = (~(pCmd->u16cksum - u8lrc) + 1) & 0xFF;
					pCmd->u8state = E_SERCMD_ASCII_CMD_CHECKSUM_ERROR;
				} else {
					// LRCが正しければ、全部足したら 0 になる。
					pCmd->u8state = E_SERCMD_ASCII_CMD_COMPLETE; // 完了！
					pCmd->u16len = pCmd->u16pos / 2 - 1;
				}
			} else {
				pCmd->u8state = E_SERCMD_ASCII_CMD_ERROR;
			}
		} else if (u8byte == 'X') {
			// X で終端したらチェックサムの計算を省く
			if (pCmd->u16pos) {
				pCmd->u8state = E_SERCMD_ASCII_CMD_COMPLETE; // 完了！
				pCmd->u16len = pCmd->u16pos / 2;
			}
		} else {
			pCmd->u8state = E_SERCMD_ASCII_CMD_EMPTY;
		}
		break;

#if 0
	case E_SERCMD_ASCII_CMD_READLF:
		if (u8byte == 0x0a) {
			pCmd->u16cksum &= 0xFF; // チェックサムを 8bit に切り捨てる

			// エラーチェック
			if (pCmd->u16cksum) {
				// 格納値
				uint8 u8lrc = pCmd->au8data[pCmd->u16pos / 2 - 1]; // u16posは最後のデータの次の位置
				// 計算値(二の補数の計算、ビット反転+1), デバッグ用に入力系列に対応する正しいLRCを格納しておく
				pCmd->u16cksum = (~(pCmd->u16cksum - u8lrc) + 1) & 0xFF;
				pCmd->u8state = E_SERCMD_ASCII_CMD_CHECKSUM_ERROR;
			} else {
				// LRCが正しければ、全部足したら 0 になる。
				pCmd->u8state = E_SERCMD_ASCII_CMD_COMPLETE; // 完了！
				pCmd->u16len = pCmd->u16pos / 2 - 1;
			}
		} else {
			pCmd->u8state = E_SERCMD_ASCII_CMD_ERROR;
		}
		break;
#endif

	default:
		break;
	}

	return pCmd->u8state;
}

/** @ingroup SERCMD
 * シリアルコマンドアスキー形式の出力補助関数。１バイトを１６進数２文字で出力する (0xA5 -> "A5")
 *
 * @param psSerStream 出力先
 * @param u8byte 出力したいバイト列
 * @param pu8lrc LRC計算用
 */
static inline void vPutByte(tsFILE *psSerStream, uint8 u8byte, uint8 *pu8lrc) {
	static const uint8 au8tbl[] = "0123456789ABCDEF";
	uint8 u8OutH, u8OutL;

	u8OutH = au8tbl[u8byte >> 4];
	u8OutL = au8tbl[u8byte & 0x0F];

	vPutChar(psSerStream, u8OutH);
	vPutChar(psSerStream, u8OutL);

	if (pu8lrc) {
		*pu8lrc += u8byte;
	}
}

/** @ingroup SERCMD
 * アスキー形式の出力 (vOutputメソッド用)
 * @param pc
 * @param ps
 */
static void SerCmdAscii_Output(tsSerCmd_Context *pc, tsFILE *ps) {
	int i;
	uint8 u8lrc = 0;

	// 先頭の :
	vPutChar(ps, ':');

	for (i = 0; i < pc->u16len; i++) {
		vPutByte(ps, pc->au8data[i], &u8lrc);
	}

	// ２の補数の計算 (LRC)
	u8lrc = ~u8lrc + 1;
	vPutByte(ps, u8lrc, NULL);

	// Trailer byte
	vPutChar(ps, 0x0D);
	vPutChar(ps, 0x0A);
}

/** @ingroup SERCMD
 * 解析構造体を初期化する。
 *
 * @param pc
 * @param pbuff
 * @param u16maxlen
 */
void SerCmdAscii_vInit(tsSerCmd_Context *pc, uint8 *pbuff, uint16 u16maxlen) {
	memset(pc, 0, sizeof(tsSerCmd_Context));

	pc->au8data = pbuff;
	pc->u16maxlen = u16maxlen;

	pc->u8Parse = SerCmdAscii_u8Parse;
	pc->vOutput = SerCmdAscii_Output;
}
