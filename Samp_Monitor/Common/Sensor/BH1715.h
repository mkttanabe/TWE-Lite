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

#ifndef  BH1715_INCLUDED
#define  BH1715_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include "sensor_driver.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct {
	// protected
	bool_t bBusy;          // should block going into sleep

	// public
	int16 i16Result;

	// private
	uint8 u8TickCount, u8TickWait;
} tsObjData_BH1715;

/****************************************************************************/
/***        Exported Functions (state machine)                            ***/
/****************************************************************************/
void vBH1715_Init(tsObjData_BH1715 *pData, tsSnsObj *pSnsObj);
void vBH1715_Final(tsObjData_BH1715 *pData, tsSnsObj *pSnsObj);


/****************************************************************************/
/***        Exported Functions (primitive funcs)                          ***/
/****************************************************************************/
PUBLIC bool_t bBH1715reset();
PUBLIC bool_t bBH1715startRead();
PUBLIC int16 i16BH1715readResult();

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* BH1715_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

