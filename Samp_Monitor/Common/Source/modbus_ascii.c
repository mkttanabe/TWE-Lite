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
#include <jendefs.h>
#include "modbus_ascii.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

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

/** @ingroup MBUSA
 * Modbus ASCII 入力系列の解釈を行う。\n
 * - :[0-9A-Z]+(CRLF) 系列の入力を行い、LRCチェックを行う
 * - + + + を入力したときに Verbose モードの判定を行う
 *
 * @param pCmd 管理構造体
 * @param u8byte 入力文字
 * @return 状態コード (teModbusCmdState 参照)
 */
uint8 ModBusAscii_u8Parse(tsModbusCmd *pCmd, uint8 u8byte) {
	// check for timeout
	if (pCmd->u8state != E_MODBUS_CMD_EMPTY && !pCmd->bverbose) {
		if ((!pCmd->bverbose) && u32TickCount_ms - pCmd->u32timestamp > MODBUS_CMD_TIMEOUT_ms) {
			pCmd->u8state = E_MODBUS_CMD_EMPTY; // start from new
		}
	}

	// check for complete or error status
	if (pCmd->u8state >= 0x80) {
		pCmd->u8state = E_MODBUS_CMD_EMPTY;
	}

	// run state machine
	switch (pCmd->u8state) {
	case E_MODBUS_CMD_EMPTY:
		if (u8byte == ':') {
			pCmd->u8state = E_MODBUS_CMD_READPAYLOAD;

			pCmd->u32timestamp = u32TickCount_ms; // store received time for timeout
			pCmd->u16pos = 0;
			pCmd->u8lrc = 0;
			// memset(pCmd->au8data, 0, pCmd->u16maxlen);
		} else
		if (u8byte == '+') {
			// press "+ (pause) + (pause) +" to toggle verbose mode.
			pCmd->u8state = E_MODBUS_CMD_PLUS1;
			pCmd->u32timestamp = u32TickCount_ms; // store received time for timeout
		}
		break;
	case E_MODBUS_CMD_READPAYLOAD:
		if ((u8byte >= '0' && u8byte <= '9') || (u8byte >= 'A' && u8byte <= 'F')) {
			/* オーバーフローのチェック */
			if (pCmd->u16pos / 2 == pCmd->u16maxlen) {
				pCmd->u8state = E_MODBUS_CMD_ERROR;
				break;
			}

			/* 文字の16進変換 */
			const uint8 u8tbl[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15 };
			uint8 u8val = u8tbl[u8byte - '0'];

			/* バイナリ値として格納する */
			uint8 *p = &pCmd->au8data[pCmd->u16pos / 2];

			if (pCmd->u16pos & 1) {
				*p = (*p & 0xF0) | u8val;
				pCmd->u8lrc += *p;
			} else {
				*p = u8val << 4;
			}

			pCmd->u16pos++;
		} else if (u8byte == 0x0d) { // CR入力
			pCmd->u8state = E_MODBUS_CMD_READLF;
		} else {
			pCmd->u8state = E_MODBUS_CMD_EMPTY;
		}
		break;
	case E_MODBUS_CMD_READLF:
		if (u8byte == 0x0a) {
			// エラーチェック
			if (pCmd->u8lrc) {
				// 格納値
				uint8 u8lrc = pCmd->au8data[pCmd->u16pos / 2 - 1]; // u16posは最後のデータの次の位置
				// 計算値(二の補数の計算、ビット反転+1), デバッグ用に入力系列に対応する正しいLRCを格納しておく
				pCmd->u8lrc = ~(pCmd->u8lrc - u8lrc) + 1;
				pCmd->u8state = E_MODBUS_CMD_LRCERROR;
			} else {
				// LRCが正しければ、全部足したら 0 になる。
				pCmd->u8state = E_MODBUS_CMD_COMPLETE; // 完了！
				pCmd->u16len = pCmd->u16pos / 2 - 1;
			}
		} else {
			pCmd->u8state = E_MODBUS_CMD_ERROR;
		}
		break;

	case E_MODBUS_CMD_PLUS1: // second press of '+'
		if ((u8byte == '+') && (u32TickCount_ms - pCmd->u32timestamp > 200) && (u32TickCount_ms - pCmd->u32timestamp < 1000)) {
			pCmd->u8state = E_MODBUS_CMD_PLUS2;
			pCmd->u32timestamp = u32TickCount_ms;
		} else {
			pCmd->u8state = E_MODBUS_CMD_ERROR;
		}
		break;

	case E_MODBUS_CMD_PLUS2: // third press of '+'
		if ((u8byte == '+') && (u32TickCount_ms - pCmd->u32timestamp > 200) && (u32TickCount_ms - pCmd->u32timestamp < 1000)) {
			pCmd->bverbose = pCmd->bverbose ? FALSE : TRUE;
			if (pCmd->bverbose) {
				pCmd->u32timevbs = u32TickCount_ms;
				pCmd->u8state = E_MODBUS_CMD_VERBOSE_ON;
			} else {
				pCmd->u8state = E_MODBUS_CMD_VERBOSE_OFF;
			}
		} else {
			pCmd->u8state = E_MODBUS_CMD_ERROR;
		}
		break;

	default:
		break;
	}

	return pCmd->u8state;
}

/** @ingroup MBUSA
 * １バイトを１６進数２文字で出力する (0xA5 -> "A5")
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
#ifdef LRC_CHARBASE
		*pu8lrc += u8OutH;
		*pu8lrc += u8OutL;
#else
		*pu8lrc += u8byte;
#endif
	}
}

/** @ingroup MBUSA
 * Modbus ASCII 形式の出力を行う。
 *
 * @param psSerStream
 * @param u8addr アドレス（１バイト目）
 * @param u8cmd コマンド（２バイト目）
 * @param pDat ペイロード
 * @param u16len ペイロードの長さ
 */
void vSerOutput_ModbusAscii(tsFILE *psSerStream, uint8 u8addr, uint8 u8cmd, uint8 *pDat, uint16 u16len) {
	int i;
	uint8 u8lrc = 0;

	// NULL buffer
	if (pDat == NULL || u16len == 0) return;

	// Header byte
	vPutChar(psSerStream, ':');

	// Address and Cmd
	vPutByte(psSerStream, u8addr, &u8lrc);
	vPutByte(psSerStream, u8cmd, &u8lrc);

	// Payload
	for (i = 0; i < u16len; i++) {
		vPutByte(psSerStream, *pDat++, &u8lrc);
	}

	// ２の補数の計算 (LRC)
	u8lrc = ~u8lrc + 1;
	vPutByte(psSerStream, u8lrc, NULL);

	// Trailer byte
	vPutChar(psSerStream, 0x0D);
	vPutChar(psSerStream, 0x0A);
}
