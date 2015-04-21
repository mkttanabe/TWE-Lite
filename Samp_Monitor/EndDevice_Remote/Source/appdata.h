/*
 * appdata.h
 *
 *  Created on: 2014/03/14
 *      Author: seigo13
 */

#ifndef APPDATA_H_
#define APPDATA_H_

#include <jendefs.h>
#include "ToCoNet.h"
#include "flash.h"
#include "appsave.h"

/**
 * LED 制御状態
 */
typedef enum {
	E_LED_OFF,  //!< E_LED_OFF
	E_LED_WAIT, //!< E_LED_WAIT
	E_LED_ERROR,//!< E_LED_ERROR
	E_LED_RESULT//!< E_LED_RESULT
} teLedState;

/**
 * アプリケーション基本データ構造体
 */
typedef struct {
	// Network context
	tsToCoNet_Nwk_Context *pContextNwk;
	tsToCoNet_NwkLyTr_Config sNwkLayerTreeConfig;

	// その他
	tsFlash sFlash; //!< フラッシュの情報
	bool_t bFlashLoaded; //!< フラッシュにデータが有れば TRUE
	uint8 u8DebugLevel; //!< ToCoNet デバッグレベル

	// wakeup status
	bool_t bWakeupByButton; //!< DIO で RAM HOLD Wakeup した場合 TRUE

	// config mode
	bool_t bConfigMode; //! CONFIG で起動した場合（インタラクティブモードを使用するため)

	// state
	teLedState eLedState; //!< LED表示状態
} tsAppData_Re;
extern tsAppData_Re sAppData_Re;

#endif /* APPDATA_H_ */
