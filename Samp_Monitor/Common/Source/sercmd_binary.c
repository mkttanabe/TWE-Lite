/****************************************************************************
 * (C) Tokyo Cosmos Electric, Inc. (TOCOS) - 2012 all rights reserved.
 *
 * Condition to use:
 *   - The full or part of source code is limited to use for TWE (TOCOS
 *     Wireless Engine) as compiled and flash programmed.
 *   - The full or part of source code is prohibited to distribute without
 *     permission from TOCOS.
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
#define SERCMD_SYNC_1 0xA5
#define SERCMD_SYNC_2 0x5A

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/** @ingroup SERCMD
 * シリアルコマンドバイナリ形式の状態定義
 */
typedef enum {
	E_SERCMD_BINARY_EMPTY = 0,      //!< E_SERCMD_BINARY_EMPTY
	E_SERCMD_BINARY_READSYNC,       //!< E_SERCMD_BINARY_READSYNC
	E_SERCMD_BINARY_READLEN,        //!< E_SERCMD_BINARY_READLEN
	E_SERCMD_BINARY_READLEN2,       //!< E_SERCMD_BINARY_READLEN2
	E_SERCMD_BINARY_READPAYLOAD,    //!< E_SERCMD_BINARY_READPAYLOAD
	E_SERCMD_BINARY_READCRC,        //!< E_SERCMD_BINARY_READCRC
	E_SERCMD_BINARY_PLUS1,          //!< E_SERCMD_BINARY_PLUS1
	E_SERCMD_BINARY_PLUS2,          //!< E_SERCMD_BINARY_PLUS2
	E_SERCMD_BINARY_COMPLETE = 0x80,//!< E_SERCMD_BINARY_COMPLETE
	E_SERCMD_BINARY_ERROR = 0x81,   //!< E_SERCMD_BINARY_ERROR
	E_SERCMD_BINARY_CRCERROR = 0x82 //!< E_SERCMD_BINARY_CRCERROR
} teSerCmd_Binary;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************///
extern uint32 u32TickCount_ms; // time in ms

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: u8ProcessSerCmd
 *
 * DESCRIPTION: Process incoming byte to form binary input sequence.
 *   format:
 *     +0 +1 +2  +3 +4...4+len-1 4+len
 *     A5 5A lenH,L payload      XOR
 *
 *   timeout:
 *     the timestamp of first byte is recorded by reffering external value
 *     u32TickCount_ms, then calucalted elapsed time from starting byte.
 *     if the elapsed time excess SERCMD_TIMEOUT_ms, the coming byte is
 *     considered as first byte.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   state information is returned. once E_SERCMD_BINARY_COMPLETE is returned,
 *   the input data shall be handled before calling this function.
 *   the next call will set tsSerCmd structure as new status.
 *
 *     - E_SERCMD_BINARY_EMPTY : new state (no input)
 *     - E_SERCMD_BINARY_ERROR : some error
 *     - E_SERCMD_BINARY_READCRC : CRC error
 *     - E_SERCMD_BINARY_COMPLETE : complete to input a command
 *
 * NOTES:
 ****************************************************************************/
extern uint32 u32TickCount_ms; // system tick counter

/** @ingroup SERCMD
 * シリアルコマンド系列の解釈を行う状態遷移マシン
 *
 * @param pCmd
 * @param u8byte
 * @return
 */
static uint8 SerCmdBinary_u8Parse(tsSerCmd_Context *pCmd, uint8 u8byte) {
	// check for timeout
	if (pCmd->u16timeout && pCmd->u8state != E_SERCMD_BINARY_EMPTY) {
		if (u32TickCount_ms - pCmd->u32timestamp > pCmd->u16timeout) {
			pCmd->u8state = E_SERCMD_BINARY_EMPTY; // start from new
		}
	}

	// check for complete or error status
	if (pCmd->u8state >= 0x80) {
		pCmd->u8state = E_SERCMD_BINARY_EMPTY;
	}

	// run state machine
	switch (pCmd->u8state) {
	case E_SERCMD_BINARY_EMPTY:
		if (u8byte == SERCMD_SYNC_1) {
			pCmd->u8state = E_SERCMD_BINARY_READSYNC;
			pCmd->u32timestamp = u32TickCount_ms; // store received time
		}
		break;

	case E_SERCMD_BINARY_READSYNC:
		if (u8byte == SERCMD_SYNC_2) {
			pCmd->u8state = E_SERCMD_BINARY_READLEN;
		} else {
			pCmd->u8state = E_SERCMD_BINARY_ERROR;
		}
		break;

	case E_SERCMD_BINARY_READLEN:
		if (u8byte & 0x80) {
			// long length mode (1...
			u8byte &= 0x7F;
			pCmd->u16len = u8byte;
			if (pCmd->u16len <= pCmd->u16maxlen) {
				pCmd->u8state = E_SERCMD_BINARY_READLEN2;
			} else {
				pCmd->u8state = E_SERCMD_BINARY_ERROR;
			}
		} else {
			// short length mode (1...127bytes)
			pCmd->u16len = u8byte;
			if (pCmd->u16len <= pCmd->u16maxlen) {
				pCmd->u8state = E_SERCMD_BINARY_READPAYLOAD;
				pCmd->u16pos = 0;
				pCmd->u16cksum = 0;
			} else {
				pCmd->u8state = E_SERCMD_BINARY_ERROR;
			}
		}
		break;

	case E_SERCMD_BINARY_READLEN2:
		pCmd->u16len = (pCmd->u16len * 256) + u8byte;
		if (pCmd->u16len <= pCmd->u16maxlen) {
			pCmd->u8state = E_SERCMD_BINARY_READPAYLOAD;
			pCmd->u16pos = 0;
			pCmd->u16cksum = 0;
		} else {
			pCmd->u8state = E_SERCMD_BINARY_ERROR;
		}
		break;

	case E_SERCMD_BINARY_READPAYLOAD:
		pCmd->au8data[pCmd->u16pos] = u8byte;
		pCmd->u16cksum ^= u8byte; // update XOR checksum
		if (pCmd->u16pos == pCmd->u16len - 1) {
			pCmd->u8state = E_SERCMD_BINARY_READCRC;
		}
		pCmd->u16pos++;
		break;

	case E_SERCMD_BINARY_READCRC:
		pCmd->u16cksum &= 0xFF;
		if (u8byte == pCmd->u16cksum) {
			pCmd->u8state = E_SERCMD_BINARY_COMPLETE;
		} else {
			pCmd->u8state = E_SERCMD_BINARY_CRCERROR;
		}
		break;

	default:
		break;
	}

	return pCmd->u8state;
}

/** @ingroup SERCMD
 * バイナリ形式の出力 (vOutputメソッド用)
 * @param pc
 * @param ps
 */
static void SerCmdBinary_Output(tsSerCmd_Context *pc, tsFILE *ps) {
	int i;
	uint8 u8xor = 0;

	// NULL buffer
	if (pc == NULL || pc->au8data == NULL || pc->u16len == 0) return;

	vPutChar(ps, SERCMD_SYNC_1);
	vPutChar(ps, SERCMD_SYNC_2);
	vPutChar(ps, (uint8)(0x80 | (pc->u16len >> 8)));
	vPutChar(ps, (pc->u16len & 0xff));

	for (i = 0; i < pc->u16len; i++) {
		u8xor ^= pc->au8data[i];
		vPutChar(ps, pc->au8data[i]);
	}

	vPutChar(ps, u8xor); // XOR check sum
	vPutChar(ps, 0x4); // EOT
}

/** @ingroup SERCMD
 * 解析構造体を初期化する。
 *
 * @param pc
 * @param pbuff
 * @param u16maxlen
 */
void SerCmdBinary_vInit(tsSerCmd_Context *pc, uint8 *pbuff, uint16 u16maxlen) {
	memset(pc, 0, sizeof(tsSerCmd_Context));

	pc->au8data = pbuff;
	pc->u16maxlen = u16maxlen;

	pc->u8Parse = SerCmdBinary_u8Parse;
	pc->vOutput = SerCmdBinary_Output;
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
