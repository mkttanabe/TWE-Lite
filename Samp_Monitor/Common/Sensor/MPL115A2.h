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

#ifndef  MPL115_INCLUDED
#define  MPL115_INCLUDED

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
} tsObjData_MPL115A2;

/****************************************************************************/
/***        Exported Functions (state machine)                            ***/
/****************************************************************************/
void vMPL115A2_Init(tsObjData_MPL115A2 *pData, tsSnsObj *pSnsObj);
void vMPL115A2_Final(tsObjData_MPL115A2 *pData, tsSnsObj *pSnsObj);

/****************************************************************************/
/***        Exported Functions (primitive funcs)                          ***/
/****************************************************************************/
PUBLIC bool_t bMPL115reset();
PUBLIC bool_t bMPL115startRead();
PUBLIC int16 i16MPL115readResult();

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* MPL115_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

