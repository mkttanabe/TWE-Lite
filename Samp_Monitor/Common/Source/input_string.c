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

#include <string.h>
#include <jendefs.h>

#include "input_string.h"

/** @ingroup INPSTR
 * 構造体の初期化を行う。
 * 同時にシリアル出力構造体を格納する。
 * @param p
 * @param pSerStream
 */
void INPSTR_vInit(tsInpStr_Context *p, tsFILE *pSerStream) {
	memset(p, 0, sizeof(tsInpStr_Context));
	p->pSerStream = pSerStream;
}

/** @ingroup INPSTR
 * 文字列入力を開始する。
 *
 * @param p 管理構造体
 * @param u8DataType データ種
 * 別 (E_INPUTSTRING_DATATYPE_STRING/E_INPUTSTRING_DATATYPE_DEC/E_INPUTSTRING_DATATYPE_HEX)
 * @param u8Maxlen 最大入力長
 * @param u32Opt 何か値を格納しておきたい時に使う
 */
void INPSTR_vStart(tsInpStr_Context*p, uint8 u8DataType, uint8 u8Maxlen, uint32 u32Opt) {
	p->u8State = E_INPUTSTRING_STATE_ACTIVE;
	p->u8Idx = 0;
	p->u8MaxLen = u8Maxlen;
	p->u8DataType = u8DataType;
	p->u32Opt = u32Opt;
}

/** @ingroup INPSTR
 * 文字列入力時に１バイトの入力を処理する。
 * - 事前に管理構造体の値を参照して E_INPUTSTRING_STATE_ACTIVE 状態を確認した上で本関数を適用すること
 *
 * @param p 管理構造体
 * @param u8Byte 入力されたバイト
 * @return 完了状態 (E_INPUTSTRING_STATE_CANCELED/E_INPUTSTRING_STATE_COMPLETE)
 */
uint8 INPSTR_u8InputByte(tsInpStr_Context*p, uint8 u8Byte) {
	bool_t bComplete = FALSE;
	bool_t bCancel = FALSE;

	uint8 u8StateRet = p->u8State;

	if (u8Byte == 0x0d || u8Byte == 0x0a) {
		// enter、系列の終了
		bComplete = TRUE;
	} else if (u8Byte == 0x08 || u8Byte == 0x7F) {
		// backspace
		if (p->u8Idx) {
			p->u8Idx--;
			vPutChar(p->pSerStream, 0x08);
		}
	} else if (u8Byte < 0x20 || u8Byte > 0x7F) {
		// コントロールコードは却下
		bCancel = TRUE;
	} else {
		// 入力データをチェックする
		bool_t bAccept = FALSE;

		// STRING 入力モードの場合、何でも受け付ける
		if (p->u8DataType == E_INPUTSTRING_DATATYPE_STRING) {
			bAccept = TRUE;
		}

		// [0-9]
		if (!bAccept
			&& (p->u8DataType == E_INPUTSTRING_DATATYPE_DEC
					|| p->u8DataType == E_INPUTSTRING_DATATYPE_HEX)
			&& u8Byte >= '0' && u8Byte <= '9') {
			bAccept = TRUE;
		}

		// [a-fA-F]
		if (!bAccept
			&& p->u8DataType  == E_INPUTSTRING_DATATYPE_HEX
			&& ((u8Byte >= 'a' && u8Byte <= 'f') || (u8Byte >= 'A' && u8Byte <= 'F')) ) {
			bAccept = TRUE;
		}

		// 長さオーバーのチェック
		if (bAccept && p->u8MaxLen != 0) {
			if (p->u8Idx >=  p->u8MaxLen) {
				bAccept = FALSE;
			}
		}

		// 系列が受けつけられたら処理する
		if (bAccept) {
			vPutChar(p->pSerStream, u8Byte);
			p->au8Data[p->u8Idx] = (uint8)u8Byte;
			p->u8Idx++;
			p->au8Data[p->u8Idx] = 0;

			if (p->u8Idx >= sizeof(p->au8Data) - 1) {
				bCancel = TRUE;
			}
		}
	}

	// 完了時の処理
	if (bComplete) {
		p->u8State = E_INPUTSTRING_STATE_IDLE;
		u8StateRet = E_INPUTSTRING_STATE_COMPLETE;
	}
	// キャンセル時の処理
	if (bCancel) {
		p->u8State = E_INPUTSTRING_STATE_IDLE;
		u8StateRet = E_INPUTSTRING_STATE_CANCELED;
	}

	return u8StateRet;
}
