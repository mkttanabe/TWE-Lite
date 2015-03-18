/*****************************************************************************
 *
 * MODULE:              Driver for LCD Driver-Controller
 *
 * COMPONENT:           $RCSfile: LcdDraw.h,v $
 *
 * VERSION:             $Name: JN514x_SDK_V0002_RC0 $
 *
 * REVISION:            $Revision: 1.3 $
 *
 * DATED:               $Date: 2008/10/22 12:19:33 $
 *
 * STATUS:              $State: Exp $
 *
 * AUTHOR:              CJG
 *
 * DESCRIPTION:
 * Provides API for driving LCD panels using LCD Driver-Controller.
 *
 * LAST MODIFIED BY:    $Author: pjtw $
 *                      $Modtime: $
 *
 ****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on
 * each copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd 2009. All rights reserved
 *
 ***************************************************************************/

#ifndef  LCD_DRAW_INCLUDED
#define  LCD_DRAW_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include "jendefs.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void   vLcdPlotPoint(uint8 u8X, uint8 u8Y);
PUBLIC bool_t boLcdGetPixel(uint8 u8X, uint8 u8Y);
PUBLIC void   vLcdFloodFill(int x, int y);
PUBLIC void   vLcdDrawLine(uint8 u8x1, uint8 u8y1, uint8 u8x2, uint8 u8y2);
PUBLIC void   vLcdDrawCircle(int Xc,int Yc,int Radius);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* LCD_DRAW_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

