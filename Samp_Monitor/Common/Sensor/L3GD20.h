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

#ifndef  L3GD20_INCLUDED
#define  L3GD20_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define L3GD20_IDX_X 0
#define L3GD20_IDX_Y 1
#define L3GD20_IDX_Z 2

#define L3GD20_IDX_BEGIN 0
#define L3GD20_IDX_END (L3GD20_IDX_Z+1) // should be (last idx + 1)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct {
	// protected
	bool_t bBusy;			// should block going into sleep

	// data
	int16 ai16Result[3];

	// working
	uint8 u8TickCount, u8TickWait;
} tsObjData_L3GD20;

/****************************************************************************/
/***        Exported Functions (state machine)                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions (primitive funcs)                          ***/
/****************************************************************************/
void vL3GD20_Init(tsObjData_L3GD20 *pData, tsSnsObj *pSnsObj, int16 i16param );
void vL3GD20_Final(tsObjData_L3GD20 *pData, tsSnsObj *pSnsObj);

PUBLIC bool_t bL3GD20reset();
PUBLIC bool_t bL3GD20startRead();
PUBLIC int16 i16L3GD20readResult( uint8 u8axis );

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* L3GD20_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

