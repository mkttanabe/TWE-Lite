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
 * アプリケーション基本データ構造体
 */
typedef struct {
	// MAC
	uint8 u8channel;

	// LED Counter
	uint32 u32LedCt;

	// Network context
	tsToCoNet_Nwk_Context *pContextNwk;
	tsToCoNet_NwkLyTr_Config sNwkLayerTreeConfig;

	// その他
	tsFlash sFlash; //!< フラッシュの情報
	bool_t bFlashLoaded;
	uint8 u8DebugLevel;
} tsAppData_Ro;
extern tsAppData_Ro sAppData_Ro;

#endif /* APPDATA_H_ */
