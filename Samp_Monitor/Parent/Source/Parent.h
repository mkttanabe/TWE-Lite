/****************************************************************************
 * (C) Tokyo Cosmos Electric, Inc. (TOCOS) - 2013 all rights reserved.
 *
 * Condition to use:
 *   - The full or part of source code is limited to use for TWE (TOCOS
 *     Wireless Engine) as compiled and flash programmed.
 *   - The full or part of source code is prohibited to distribute without
 *     permission from TOCOS.
 *
 ****************************************************************************/

#ifndef  SLAVE_H_INCLUDED
#define  SLAVE_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include "config.h"
#include "appdata.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
//	受信したパケットから取得した基本的な情報
typedef struct _tsRxPktInfo{
	uint8 u8lqi_1st;		//	LQI
	uint32 u32addr_1st;		//	アドレス
	uint32 u32addr_rcvr;	//	受信機アドレス
	uint8 u8id;				//	ID
	uint16 u16fct;			//	FCT
	uint8 u8pkt;			//	子機のセンサモード
} tsRxPktInfo;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* SLAVE_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
