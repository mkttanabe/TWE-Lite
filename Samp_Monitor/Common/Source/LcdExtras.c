/****************************************************************************
 *
 * MODULE:             Driver for UC1601 and UC1606 LCD Driver-Controller
 *
 * COMPONENT:          $RCSfile: LcdExtras.c,v $
 *
 * VERSION:            $Name: RD_RELEASE_6thMay09 $
 *
 * REVISION:           $Revision: 1.3 $
 *
 * DATED:              $Date: 2008/10/22 12:19:33 $
 *
 * STATUS:             $State: Exp $
 *
 * AUTHOR:             CJG
 *
 * DESCRIPTION:
 * Provides API for driving LCD panels using UC1601 or UC1606 LCD
 * Driver-Controller.
 *
 * LAST MODIFIED BY:   $Author: pjtw $
 *                     $Modtime: $
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
 * Copyright Jennic Ltd 2005. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "LcdDriver.h"
#include "LcdFont.h"
#include "AppHardwareApi.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define LCD_CMD_SET_BIAS                (0xe8)
#define LCD_CMD_SET_FRAME_RATE          (0xa0)
#define LCD_CMD_SYSTEM_RESET            (0xe2)
#define LCD_CMD_SET_DISPLAY             (0xae)
#define LCD_CMD_SET_RAM_ADDRESS_CONTROL (0x88)
#define LCD_CMD_SET_PAGE_ADDRESS        (0xb0)
#define LCD_CMD_SET_COLUMN_LSB          (0x00)
#define LCD_CMD_SET_COLUMN_MSB          (0x10)
#define  TocosEvaKit
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
extern PUBLIC void vLcdSendCommand(uint8 u8Command);
extern PUBLIC void vLcdSendData(uint8 u8Data);
extern PUBLIC void vLcdSendData32(uint32 u32Data);
extern PUBLIC void vLcdGrabSpiBus(void);
extern PUBLIC void vLcdFreeSpiBus(void);
extern PUBLIC void vLcdSetCDline(uint8 u8OnOff);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vLcdStop
 *
 * DESCRIPTION:
 * Turns off LCD and allows it to power down correctly.
 *
 * NOTES:
 * Should be called before powering down LCD panel.
 *
 ****************************************************************************/
PUBLIC void vLcdStop(void)
{
    vLcdSendCommand(LCD_CMD_SYSTEM_RESET);
}

/****************************************************************************
 *
 * NAME: vLcdRefreshArea
 *
 * DESCRIPTION:
 * Copies part of the shadow memory to the LCD
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u8LeftColumn    R   Left column of refresh area (0-127)
 *                  u8TopRow        R   Top character row of refresh area (0-7)
 *                  u8Width         R   Number of columns (1-128)
 *                  u8Height        R   Number of character rows (1-8)
 *
 * NOTES:
 * Takes a certain amount of time!
 *
 ****************************************************************************/
PUBLIC void vLcdRefreshArea(uint8 u8LeftColumn, uint8 u8TopRow, uint8 u8Width,
                            uint8 u8Height)
{

#ifdef TocosEvaKit

	  uint8 u8Row;
	    uint8 u8Col;
	    uint8 *pu8Shadow;



	    for (u8Row = 0; u8Row < u8Height; u8Row++)
	    {
	        pu8Shadow = &au8Shadow[(u8TopRow + u8Row) * 128 + u8LeftColumn];

	        /* Set row and column */
	        vLcdSendCommand((uint8)(LCD_CMD_SET_PAGE_ADDRESS | (7-(u8TopRow + u8Row))));

	        //vLcdSendCommand1((uint8)(LCD_CMD_SET_COLUMN_LSB | ((128-u8LeftColumn + 4) & 0xf)));
	        //vLcdSendCommand1((uint8)(LCD_CMD_SET_COLUMN_MSB | ((128-u8LeftColumn + 4) >> 4)));

	        vLcdSendCommand((uint8)(LCD_CMD_SET_COLUMN_LSB | ((u8LeftColumn + 4) >>4)));
	        vLcdSendCommand((uint8)(LCD_CMD_SET_COLUMN_MSB | ((u8LeftColumn) & 0xf)));

	        /* Send line of data */

	        vAHI_DioSetOutput(0x2,0x0); //a0 = 1;
	        vAHI_SpiConfigure(LCD_SS,TRUE,FALSE, FALSE, 3, FALSE, FALSE); //3 (2MHz)
	        for (u8Col = 0; u8Col < u8Width; u8Col++)
	        {
	        	vAHI_SpiStartTransfer(7,*(uint8 *)pu8Shadow);
	        	vAHI_SpiWaitBusy();
	        	 pu8Shadow++;
	        }
	        vAHI_DioSetOutput(0x0,0x2); //a0 = 0;
	    }


#endif


#ifdef JennicEvaKit

    uint8 u8Row;
    uint8 u8Col;
    uint8 *pu8Shadow;

    vLcdGrabSpiBus();

    for (u8Row = 0; u8Row < u8Height; u8Row++)
    {
        pu8Shadow = &au8Shadow[(u8TopRow + u8Row) * 128 + u8LeftColumn];

        /* Set row and column */
        vLcdSendCommand((uint8)(LCD_CMD_SET_PAGE_ADDRESS | (u8TopRow + u8Row)));
        vLcdSendCommand((uint8)(LCD_CMD_SET_COLUMN_LSB | ((u8LeftColumn + 4) & 0xf)));
        vLcdSendCommand((uint8)(LCD_CMD_SET_COLUMN_MSB | ((u8LeftColumn + 4) >> 4)));

        /* Send line of data */
        vLcdSetCDline(1);
        for (u8Col = 0; u8Col < u8Width; u8Col++)
        {
            vLcdSendData(*pu8Shadow);
            pu8Shadow++;
        }
        vLcdSetCDline(0);
    }

    vLcdFreeSpiBus();
#endif

}

/****************************************************************************
 *
 * NAME: vLcdClearLine
 *
 * DESCRIPTION:
 * Clears a specified line on the LCD Display
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u8Row           R   Character row (0-7)
 *
 * NOTES:
 * To see text, perform a refresh
 *
 ****************************************************************************/
PUBLIC void vLcdClearLine(uint8 u8Row)
{
    uint8 i;
    uint8 *pu8Shadow = &au8Shadow[u8Row * 128];

    for(i=0; i<128;i++)
    {
        *pu8Shadow = 0;
        pu8Shadow++;
    }
}

/****************************************************************************
 *
 * NAME: vLcdWriteTextToClearLine
 *
 * DESCRIPTION:
 * Puts text into shadow buffer. Text is aligned to a character row but can
 * be at any column. Characters are variable width but 8 pixels high.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  pcString        R   Pointer to zero-terminated string
 *                  u8Row           R   Character row (0-7)
 *                  u8Column        R   Start column (0-127)
 *
 * NOTES:
 * To see text, perform a refresh
 *
 ****************************************************************************/

PUBLIC void vLcdWriteTextToClearLine(char *pcString, uint8 u8Row, uint8 u8Column)
{
    vLcdClearLine(u8Row);

    vLcdWriteText(pcString, u8Row, u8Column);

}

/****************************************************************************
 *
 * NAME: vLcdWriteTextRightJustified
 *
 * DESCRIPTION:
 * Puts text into shadow buffer. Text is aligned to a character row but can
 * be at any column. Characters are variable width but 8 pixels high. The text
 * is right justified, with the supplied column being the rightmost one.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  pcString        R   Pointer to zero-terminated string
 *                  u8Row           R   Character row (0-7)
 *                  u8EndColumn     R   End column (0-127)
 *
 * NOTES:
 * To see text, perform a refresh
 *
 ****************************************************************************/
PUBLIC void vLcdWriteTextRightJustified(char *pcString, uint8 u8Row,
                                        uint8 u8EndColumn)
{
    uint8 u8Columns = 0;
    uint8 *pu8CharMap;
    char *pcStringCopy = pcString;

    /* Work out how many columns we need */
    while (*pcStringCopy != 0)
    {
        pu8CharMap = pu8LcdFontGetChar(*pcStringCopy);
        u8Columns += *pu8CharMap;
        u8Columns += 1;
        pcStringCopy++;
    }

    /* Call normal text write function */
    vLcdWriteText(pcString, u8Row, u8EndColumn - u8Columns);
}

/****************************************************************************
 *
 * NAME: vLcdScrollUp
 *
 * DESCRIPTION:
 * Scrolls the shadow memory up by one row. Region scrolled is between
 * specified row and bottom of screen. Bottom row is left cleared.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u8Row           R   First row of scrolled region
 *
 ****************************************************************************/
PUBLIC void vLcdScrollUp(uint8 u8Row)
{
    uint8 u8Count;
    uint32 *pu32ShadowTo = (uint32 *)&au8Shadow[128 * u8Row];
    uint32 *pu32ShadowFrom = (uint32 *)&au8Shadow[128 * (u8Row + 1)];

    if (u8Row < 8)
    {
        /* Move rows up */
        while (u8Row < 7)
        {
            for (u8Count = 0; u8Count < 32; u8Count++)
            {
                *pu32ShadowTo = *pu32ShadowFrom;
                pu32ShadowTo++;
                pu32ShadowFrom++;
            }
            u8Row++;
        }

        /* Clear row 7 */
        for (u8Count = 0; u8Count < 32; u8Count++)
        {
            *pu32ShadowTo = 0;
            pu32ShadowTo++;
        }
    }
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
