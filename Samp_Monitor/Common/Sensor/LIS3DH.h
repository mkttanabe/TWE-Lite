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

#ifndef  LIS3DH_INCLUDED
#define  LIS3DH_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define LIS3DH_IDX_X 0
#define LIS3DH_IDX_Y 1
#define LIS3DH_IDX_Z 2

#define LIS3DH_IDX_BEGIN 0
#define LIS3DH_IDX_END (LIS3DH_IDX_Z+1) // should be (last idx + 1)

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
} tsObjData_LIS3DH;

/****************************************************************************/
/***        Exported Functions (state machine)                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions (primitive funcs)                          ***/
/****************************************************************************/
void vLIS3DH_Init(tsObjData_LIS3DH *pData, tsSnsObj *pSnsObj );
void vLIS3DH_Final(tsObjData_LIS3DH *pData, tsSnsObj *pSnsObj);

PUBLIC bool_t bLIS3DHreset();
PUBLIC bool_t bLIS3DHstartRead();
PUBLIC int16 i16LIS3DHreadResult( uint8 u8axis );

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* LIS3DH_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

