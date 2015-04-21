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

#ifndef  TSL2561_INCLUDED
#define  TSL2561_INCLUDED

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
#define GAIN_1X 0
#define GAIN_16X 1

#define INTG_13MS 0
#define INTG_101MS 1
#define INTG_402MS 2
#define INTG_MANUAL 3

#define LUX_SCALE 14	// scale by 2^14
#define RATIO_SCALE 9	// scale ratio by 2^9
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// Integration time scaling factors
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
#define CH_SCALE 10				// scale channel values by 2^10
#define CHSCALE_TINT0 0x7517	// 322/11 * 2^CH_SCALE
#define CHSCALE_TINT1 0x0fe7	// 322/81 * 2^CH_SCALE

/*−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
 T, FN, and CL Package coefficients
−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
 For Ch1/Ch0=0.00 to 0.50
 Lux/Ch0=0.0304−0.062*((Ch1/Ch0)^1.4)
 piecewise approximation
 For Ch1/Ch0=0.00 to 0.125:
 Lux/Ch0=0.0304−0.0272*(Ch1/Ch0)

 For Ch1/Ch0=0.125 to 0.250:
 Lux/Ch0=0.0325−0.0440*(Ch1/Ch0)

 For Ch1/Ch0=0.250 to 0.375:
 Lux/Ch0=0.0351−0.0544*(Ch1/Ch0)

 For Ch1/Ch0=0.375 to 0.50:
 Lux/Ch0=0.0381−0.0624*(Ch1/Ch0)

 For Ch1/Ch0=0.50 to 0.61:
 Lux/Ch0=0.0224−0.031*(Ch1/Ch0)

 For Ch1/Ch0=0.61 to 0.80:
 Lux/Ch0=0.0128−0.0153*(Ch1/Ch0)

 For Ch1/Ch0=0.80 to 1.30:
 Lux/Ch0=0.00146−0.00112*(Ch1/Ch0)

 For Ch1/Ch0>1.3:
 Lux/Ch0=0
−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−*/
#define K1T 0x0040 // 0.125 * 2^RATIO_SCALE
#define B1T 0x01f2 // 0.0304 * 2^LUX_SCALE
#define M1T 0x01be // 0.0272 * 2^LUX_SCALE
#define K2T 0x0080 // 0.250 * 2^RATIO_SCALE
#define B2T 0x0214 // 0.0325 * 2^LUX_SCALE
#define M2T 0x02d1 // 0.0440 * 2^LUX_SCALE
#define K3T 0x00c0 // 0.375 * 2^RATIO_SCALE
#define B3T 0x023f // 0.0351 * 2^LUX_SCALE
#define M3T 0x037b // 0.0544 * 2^LUX_SCALE
#define K4T 0x0100 // 0.50 * 2^RATIO_SCALE
#define B4T 0x0270 // 0.0381 * 2^LUX_SCALE
#define M4T 0x03fe // 0.0624 * 2^LUX_SCALE
#define K5T 0x0138 // 0.61 * 2^RATIO_SCALE
#define B5T 0x016f // 0.0224 * 2^LUX_SCALE
#define M5T 0x01fc // 0.0310 * 2^LUX_SCALE
#define K6T 0x019a // 0.80 * 2^RATIO_SCALE
#define B6T 0x00d2 // 0.0128 * 2^LUX_SCALE
#define M6T 0x00fb // 0.0153 * 2^LUX_SCALE
#define K7T 0x029a // 1.3 * 2^RATIO_SCALE
#define B7T 0x0018 // 0.00146 * 2^LUX_SCALE
#define M7T 0x0012 // 0.00112 * 2^LUX_SCALE
#define K8T 0x029a // 1.3 * 2^RATIO_SCALE
#define B8T 0x0000 // 0.000 * 2^LUX_SCALE
#define M8T 0x0000 // 0.000 * 2^LUX_SCALE

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct {
	// protected
	bool_t bBusy;          // should block going into sleep

	// public
	uint32 u32Result;

	// private
	uint8 u8TickCount, u8TickWait;
} tsObjData_TSL2561;

/****************************************************************************/
/***        Exported Functions (state machine)                            ***/
/****************************************************************************/
void vTSL2561_Init(tsObjData_TSL2561 *pData, tsSnsObj *pSnsObj);
void vTSL2561_Final(tsObjData_TSL2561 *pData, tsSnsObj *pSnsObj);

/****************************************************************************/
/***        Exported Functions (primitive funcs)                          ***/
/****************************************************************************/
PUBLIC bool_t bTSL2561reset(bool_t);
PUBLIC bool_t bTSL2561startRead();
PUBLIC uint32 u32TSL2561readResult(void);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* TSL2561_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

