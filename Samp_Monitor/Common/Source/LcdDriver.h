/*****************************************************************************
 *
 * MODULE:              Driver for LCD Driver-Controller
 *
 * COMPONENT:           $RCSfile: LcdDriver.h,v $
 *
 * VERSION:             $Name: RD_RELEASE_6thMay09 $
 *
 * REVISION:            $Revision: 1.4 $
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
 * Copyright Jennic Ltd 2005, 2006. All rights reserved
 *
 ***************************************************************************/

#ifndef  LCD_DRIVER_INCLUDED
#define  LCD_DRIVER_INCLUDED

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
#define LCD_SS           (1)
#define LCD_SS_MASK      (1 << LCD_SS)
#define LCD_CD_BIT_MASK  (1 << 1)
#define LCD_RST_BIT_MASK (1 << 2)

#define LCD_DIO_BIT_MASK (LCD_CD_BIT_MASK | LCD_RST_BIT_MASK)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct
{
    /* Bitmap can be any number of columns but a multiple of eight bits high,
       will be aligned to a character row when displayed and is as follows:
       column   0  1  2    y-1
       row  0  aa ab ac .. ax
       row  1  ba bb bc .. bx
       row  2  ca cb cc .. cx
               .. .. .. .. ..
       row  7  ha hb hc .. hx
       row  8  ia ib ic .. ix
       row  9  ja jb jc .. jx
               .. .. .. .. ..
       row 15  pa pb pc .. px

       pu8Bitmap[0]   = (MSB) ha ga fa ea da ca ba aa (LSB)
       pu8Bitmap[1]   = (MSB) hb gb fb eb db cb bb ab (LSB)
       pu8Bitmap[y-1] = (MSB) hx gx fx ex dx cx bx ax (LSB)
       pu8Bitmap[y]   = (MSB) pa oa na ma la ka ja ia (LSB)
    */
    uint8 *pu8Bitmap;
    uint8 u8Width;  /* Width in pixels (y in example above) */
    uint8 u8Height; /* Height in character rows (2 in example above) */
} tsBitmap;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void vLcdResetDefault(void);
PUBLIC void vLcdReset(uint8 u8Bias, uint8 u8Gain);
PUBLIC void vLcdStop(void);
PUBLIC void vLcdClear(void);
PUBLIC void vLcdRefreshAll(void);
PUBLIC void vLcdRefreshArea(uint8 u8LeftColumn, uint8 u8TopRow, uint8 u8Width,
                            uint8 u8Height);
PUBLIC void vLcdWriteText(char *pcString, uint8 u8Row, uint8 u8Column);
PUBLIC void vLcdWriteTextRightJustified(char *pcString, uint8 u8Row,
                                        uint8 u8EndColumn);
PUBLIC void vLcdWriteInvertedText(char *pcString, uint8 u8Row, uint8 u8Column);
PUBLIC void vLcdWriteBitmap(tsBitmap *psBitmap, uint8 u8LeftColumn,
                            uint8 u8TopRow);
PUBLIC void vLcdScrollUp(uint8 u8Row);
PUBLIC void vLcdClearLine(uint8 u8Row);
PUBLIC void vLcdWriteTextToClearLine(char *pcString, uint8 u8Row, uint8 u8Column);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PUBLIC uint8 au8Shadow[1024] __attribute__ ((aligned (4)));

#if defined __cplusplus
}
#endif

#endif  /* LCD_DRIVER_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

