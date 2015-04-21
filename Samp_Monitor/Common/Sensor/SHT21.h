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

#ifndef  HUM_SHT21_INCLUDED
#define  HUM_SHT21_INCLUDED

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include "sensor_driver.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define SHT21_IDX_TEMP 0
#define SHT21_IDX_HUMID 1
#define SHT21_IDX_BEGIN 0
#define SHT21_IDX_END (SHT21_IDX_HUMID+1) // should be (last idx + 1)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct {
	// data
	int16 ai16Result[2];

	// working
	uint8 u8TickCount, u8TickWait;
	uint8 u8IdxMeasuruing;
} tsObjData_SHT21;

/****************************************************************************/
/***        Exported Functions (state machine)                            ***/
/****************************************************************************/
void vSHT21_Init(tsObjData_SHT21 *pData, tsSnsObj *pSnsObj);
void vSHT21_Final(tsObjData_SHT21 *pData, tsSnsObj *pSnsObj);

#define i16SHT21_GetTemp(pSnsObj) ((tsObjData_SHT21 *)(pSnsObj->pData)->ai16Result[SHT21_IDX_TEMP])
#define i16SHT21_GetHumd(pSnsObj) ((tsObjData_SHT21 *)(pSnsObj->pData)->ai16Result[SHT21_IDX_HUMID])

/****************************************************************************/
/***        Exported Functions (primitive funcs)                          ***/
/****************************************************************************/
PUBLIC bool_t bSHT21reset();
PUBLIC bool_t bSHT21startRead(uint8);
PUBLIC int16 i16SHT21readResult(int16*, int16*);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#endif  /* HUM_SHT21_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

