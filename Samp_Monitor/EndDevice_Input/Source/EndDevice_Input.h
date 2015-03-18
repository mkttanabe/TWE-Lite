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


#ifndef  MASTER_H_INCLUDED
#define  MASTER_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <jendefs.h>


/// TOCONET 関連の定義
#define TOCONET_DEBUG_LEVEL 0

// Select Modules (define befor include "ToCoNet.h")
#define ToCoNet_USE_MOD_NWK_LAYERTREE_MININODES

// includes
#include "ToCoNet.h"
#include "ToCoNet_mod_prototype.h"

#include "app_event.h"

#include "appdata.h"

#include "sercmd_gen.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/**
 * スイッチ閉⇒開検出後に、周期的にチェックする間隔
 */
static const uint16 u16_IO_Timer_mini_sleep_dur = 1000;

/**
 * ブザーを鳴らす時間
 */
static const uint16 u16_IO_Timer_buzz_dur = 100;

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/**
 * 無線状態管理用の状態定義
 */
typedef enum {
	E_IO_TIMER_NWK_IDLE = 0,                  //!< E_IO_TIMER_NWK_IDLE
	E_IO_TIMER_NWK_FIRE_REQUEST,              //!< E_IO_TIMER_NWK_FIRE_REQUEST
	E_IO_TIMER_NWK_NWK_START,                 //!< E_IO_TIMER_NWK_NWK_START
	E_IO_TIMER_NWK_COMPLETE_MASK = 0x10,       //!< E_IO_TIMER_NWK_COMPLETE_MASK
	E_IO_TIMER_NWK_COMPLETE_TX_SUCCESS  = 0x11,//!< E_IO_TIMER_NWK_COMPLETE_TX_SUCCESS
	E_IO_TIMER_NWK_COMPLETE_TX_FAIL  = 0x12,   //!< E_IO_TIMER_NWK_COMPLETE_TX_FAIL
} teIOTimerNwk;

/**
 * アプリケーションごとの振る舞いを決める関数テーブル
 */
typedef struct _sCbHandler {
	uint8 (*pf_cbToCoNet_u8HwInt)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	void (*pf_cbToCoNet_vHwEvent)(uint32 u32DeviceId, uint32 u32ItemBitmap);
	void (*pf_cbToCoNet_vMain)();
	void (*pf_cbToCoNet_vNwkEvent)(teEvent eEvent, uint32 u32arg);
	void (*pf_cbToCoNet_vRxEvent)(tsRxDataApp *pRx);
	void (*pf_cbToCoNet_vTxEvent)(uint8 u8CbId, uint8 bStatus);
} tsCbHandler;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
extern void *pvProcessEv1, *pvProcessEv2;
extern void (*pf_cbProcessSerialCmd)(tsSerCmd_Context *);

void vInitAppStandard();
void vInitAppBotton();
void vInitAppSHT21();
void vInitAppADT7410();
void vInitAppMPL115A2();
void vInitAppLIS3DH();
void vInitAppDoorTimer();
void vInitAppUart();
void vInitAppConfig();

void vPortSetSns(bool_t bActive);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern tsFILE sSerStream;
extern tsCbHandler *psCbHandler;

#if defined __cplusplus
}
#endif

#endif  /* MASTER_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
