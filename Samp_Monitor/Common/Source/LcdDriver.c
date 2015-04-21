/****************************************************************************
 *
 * MODULE:             Driver for UC1601 and UC1606 LCD Driver-Controller
 *
 * COMPONENT:          $RCSfile: LcdDriver.c,v $
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
#define TocosEvaKit

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PUBLIC void vLcdSendCommand(uint8 u8Command);
PUBLIC void vLcdSendData(uint8 u8Data);
PUBLIC void vLcdSendData32(uint32 u32Data);
PUBLIC void vLcdGrabSpiBus(void);
PUBLIC void vLcdFreeSpiBus(void);
PUBLIC void vLcdSetCDline(uint8 u8OnOff);

PRIVATE void vWriteText(char *pcString, uint8 u8Row, uint8 u8Column, uint8 u8Mask);
PRIVATE void vWait(uint32 iWait);
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
 * NAME: vLcdResetDefault
 *
 * DESCRIPTION:
 * Called when bringing LCD out of reset.
 *
 * NOTES:
 * Default values are used for most things.
 * It is assumed that a reasonable time has passed since power was applied
 * as 20~30ms is required by LCD.
 *
 ****************************************************************************/
PUBLIC void vLcdResetDefault(void)
{
    vLcdReset(3, 0);
}

/****************************************************************************
 *
 * NAME: vLcdReset
 *
 * DESCRIPTION:
 * Called when bringing LCD out of reset.
 *
 * PARAMETERS:      Name      RW  Usage
 *                  u8Bias    R   Bias value to use (see data sheet) (0-3)
 *                  u8Gain    R   Gain value to use (see data sheet) (0-7)
 *
 * NOTES:
 * Default values are used for most things.
 * It is assumed that a reasonable time has passed since power was applied
 * as 20~30ms is required by LCD.
 *
 ****************************************************************************/
PUBLIC void vLcdReset(uint8 u8Bias, uint8 u8Gain)
{

#ifdef TocosEvaKit

	vAHI_DioSetDirection(0x0, 0x102); // DIO1 and DIO8
    vAHI_DioSetOutput(0x0,0x100); //DIO8  OFF
    vWait(200000);//100ms
    vAHI_DioSetOutput(0x100,0x0); //DIO8  ON

    /* Initialise font */
    vLcdFontReset();

    /* Clear shadow area to blank */
    vLcdClear();

    vLcdSendCommand(0xa0);
    vLcdSendCommand(0xae);
    vLcdSendCommand(0xc0);
    vLcdSendCommand(0xa2);
    vLcdSendCommand(0x2f);
    vLcdSendCommand(0x26);
    vLcdSendCommand(0x81);
    vLcdSendCommand(0x11);
    vLcdSendCommand(0xaf);

    /* Refresh display */
    vLcdRefreshAll();
#endif



#ifdef JennicEvaKit
	/* Put LCD in to reset */
    vAHI_DioSetOutput(0, LCD_RST_BIT_MASK);

    /* Set up DIO */
    vAHI_DioSetDirection(0, LCD_DIO_BIT_MASK);

    /* Initialise font */
    vLcdFontReset();

    /* Clear shadow area to blank */
    vLcdClear();

    /* Bring device out of reset */
    vAHI_DioSetOutput(LCD_RST_BIT_MASK, 0);

    /* Set contrast */
    vLcdSendCommand(LCD_CMD_SET_BIAS | u8Bias);

    /* Set automatic wrapping */
    vLcdSendCommand(LCD_CMD_SET_RAM_ADDRESS_CONTROL | 1);

    /* Set fast refesh rate */
    vLcdSendCommand(LCD_CMD_SET_FRAME_RATE | 1);

    /* Bring LCD display out of reset */
    vLcdSendCommand(LCD_CMD_SET_DISPLAY | 1);

    /* Refresh display */
    vLcdRefreshAll();
#endif

}

/****************************************************************************
 *
 * NAME: vLcdClear
 *
 * DESCRIPTION:
 * Clears the LCD shadow. Does not refresh the display itself
 *
 ****************************************************************************/
PUBLIC void vLcdClear(void)
{
    int i;

    for (i = 0; i < 1024; i++)
    {
        au8Shadow[i] = 0;
    }

}

/****************************************************************************
 *
 * NAME: vLcdRefreshAll
 *
 * DESCRIPTION:
 * Copies the contents of the shadow memory to the LCD
 *
 * NOTES:
 * Takes a certain amount of time!
 *
 ****************************************************************************/
PUBLIC void vLcdRefreshAll(void)
{

#ifdef TocosEvaKit

	uint8 u8Row;
	    uint8 u8Col;
	    uint8 *pu8Shadow = au8Shadow;
	    unsigned char page = 0xB0+7;			//Page Address + 0xB0

	    vAHI_SpiConfigure(LCD_SS,FALSE,FALSE, FALSE, 3, FALSE, FALSE); //3 (2MHz)
	  	        /* Set slave select */
	  	vAHI_SpiSelect(LCD_SS_MASK);

	    /* Set row to 0 and column to 0 */
	  	vLcdSendCommand(0x40);
	   // vAHI_DioSetOutput(0x2,0x0); //a0 = 1;
	    for (u8Row = 0; u8Row < 8; u8Row++)
	    {
	        /* Send one word of dummy data (controller has 132 column memory) */
	    	 //vAHI_SpiStartTransfer(7,0);
	    	 //vAHI_SpiWaitBusy();
	 		vLcdSendCommand(page);				//send page address
	 		vLcdSendCommand(0x10);				//column address upper 4 bits + 0x10
	 		vLcdSendCommand(0x00);				//column address lower 4 bits + 0x00
	        /* Send line of data */

	 		vAHI_DioSetOutput(0x2,0x0); //a0 = 1;
	 		vAHI_SpiConfigure(LCD_SS,TRUE,FALSE, FALSE, 3, FALSE, FALSE); //3 (2MHz)

	        for (u8Col = 0; u8Col < 128; u8Col += 1)
	        {
	            /* Send data in 32 bit chunks. Fortunately the byte ordering
	               is correct so no swapping about required */
	            //vLcdSendData32(*(uint32 *)pu8Shadow);
	        	vAHI_SpiStartTransfer(7,*(uint8 *)pu8Shadow);
	            vAHI_SpiWaitBusy();
	            pu8Shadow += 1;
	        }   page--;
	        vAHI_DioSetOutput(0x0,0x2); //a0 = 0;
	    }

#endif

#ifdef JennicEvaKit
	uint8 u8Row;
    uint8 u8Col;
    uint8 *pu8Shadow = au8Shadow;

    vLcdGrabSpiBus();

    /* Set row to 0 and column to 0 */
    vLcdSendCommand(LCD_CMD_SET_PAGE_ADDRESS);
    vLcdSendCommand(LCD_CMD_SET_COLUMN_LSB);
    vLcdSendCommand(LCD_CMD_SET_COLUMN_MSB);

    vLcdSetCDline(1);

    for (u8Row = 0; u8Row < 8; u8Row++)
    {
        /* Send one word of dummy data (controller has 132 column memory) */
        vLcdSendData32(0);

        /* Send line of data */
        for (u8Col = 0; u8Col < 128; u8Col += 4)
        {
            /* Send data in 32 bit chunks. Fortunately the byte ordering
               is correct so no swapping about required */
            vLcdSendData32(*(uint32 *)pu8Shadow);
            pu8Shadow += 4;
        }
    }
    vLcdSetCDline(0);

    vLcdFreeSpiBus();
#endif
}

/****************************************************************************
 *
 * NAME: vLcdWriteText
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
PUBLIC void vLcdWriteText(char *pcString, uint8 u8Row, uint8 u8Column)
{
    vWriteText(pcString, u8Row, u8Column, 0);
}

/****************************************************************************
 *
 * NAME: vLcdWriteInvertedText
 *
 * DESCRIPTION:
 * Puts text into shadow buffer. Text is aligned to a character row but can
 * be at any column. Characters are variable width but 8 pixels high. Text is
 * inverted (black <-> white).
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
PUBLIC void vLcdWriteInvertedText(char *pcString, uint8 u8Row, uint8 u8Column)
{
    vWriteText(pcString, u8Row, u8Column, 0xff);
}

/****************************************************************************
 *
 * NAME: vLcdWriteBitmap
 *
 * DESCRIPTION:
 * Copies a bitmap to shadow memory
 *
 * PARAMETERS:      Name            RW  Usage
 *                  ps8Bitmap       R   Bitmap image: see .h file for details
 *                  u8LeftColumn    R   Left column to place bitmap (0-127)
 *                  u8TopRow        R   Top row of bitmap (0-7)
 *
 ****************************************************************************/
PUBLIC void vLcdWriteBitmap(tsBitmap *psBitmap, uint8 u8LeftColumn,
                            uint8 u8TopRow)
{
    uint8 u8Row;
    uint8 u8Col;
    uint8 u8EndRow;
    uint8 u8EndCol;
    uint8 *pu8Shadow;
    uint8 *pu8Bitmap;

    /* Set ends so bitmap does not wrap around */
    u8EndRow = psBitmap->u8Height;
    if ((u8TopRow + u8EndRow) > 8)
    {
        u8EndRow = 8 - u8TopRow;
    }

    u8EndCol = psBitmap->u8Width;
    if ((u8LeftColumn + u8EndCol) > 128)
    {
        u8EndCol = 128 - u8LeftColumn;
    }

    /* Copy bitmap */
    for (u8Row = 0; u8Row < u8EndRow; u8Row++)
    {
        pu8Shadow = &au8Shadow[(u8TopRow + u8Row) * 128 + u8LeftColumn];
        pu8Bitmap = &psBitmap->pu8Bitmap[u8Row * psBitmap->u8Width];

        for (u8Col = 0; u8Col < u8EndCol; u8Col++)
        {
            pu8Shadow[u8Col] = pu8Bitmap[u8Col];
        }
    }
}

/****************************************************************************
 *
 * NAME: vLcdSendCommand
 *
 * DESCRIPTION:
 * Sends a command to the LCD controller. Commands are the same as data but
 * are differentiated by the CD line being low.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u8Command       R   8-bit command (includes parameters)
 *
 ****************************************************************************/
PUBLIC void vLcdSendCommand(uint8 u8Command)
{

#ifdef TocosEvaKit

    vAHI_SpiConfigure(LCD_SS,FALSE,FALSE, FALSE, 2, FALSE, FALSE); //2 (4MHz)
        /* Set slave select */
    vAHI_SpiSelect(LCD_SS_MASK);


    vAHI_DioSetOutput(0x0,0x2); //a0 = 0;
    //vAHI_DioSetOutput(0x0,0x1);  //cs = 0;
    vAHI_SpiStartTransfer(7,u8Command);
    vAHI_SpiWaitBusy();
    //vAHI_DioSetOutput(0x1,0x0);  //cs = 1;


#endif


#ifdef JennicEvaKit
	vLcdGrabSpiBus();
    vLcdSetCDline(0);
    vLcdSendData(u8Command);
    vLcdFreeSpiBus();
#endif

}

/****************************************************************************
 *
 * NAME: vLcdSendData
 *
 * DESCRIPTION:
 * Sends 8 bits of data to LCD controller and waits for it to finish.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u8Data          R   8 bits of data
 *
 ****************************************************************************/
PUBLIC void vLcdSendData(uint8 u8Data)
{
#ifdef TocosEvaKit

    vAHI_DioSetOutput(0x2,0x0); //a0 = 1;
   // vAHI_DioSetOutput(0x0,0x1);  //cs = 0;
    vAHI_SpiStartTransfer(7,u8Data);
    vAHI_SpiWaitBusy();
    //vAHI_DioSetOutput(0x1,0x0);  //cs = 1;

#endif


#ifdef JennicEvaKit
    vAHI_SpiStartTransfer8(u8Data);
    vAHI_SpiWaitBusy();
#endif
}

/****************************************************************************
 *
 * NAME: vLcdSendData32
 *
 * DESCRIPTION:
 * Sends 32 bits of data to LCD controller and waits for it to finish.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u32Data         R   32 bits of data
 *
 ****************************************************************************/
PUBLIC void vLcdSendData32(uint32 u32Data)
{
    vAHI_SpiStartTransfer32(u32Data);
    vAHI_SpiWaitBusy();
}

/****************************************************************************
 *
 * NAME: vLcdGrabSpiBus
 *
 * DESCRIPTION:
 * Sets up the SPI bus for use by this driver. It sets it to automatic
 * slave select and then selects the correct slave, and also sets the clock
 * divider to give 4MHz
 *
 ****************************************************************************/
PUBLIC void vLcdGrabSpiBus(void)
{
    /* Set SPI master to use auto slave select at 4MHz */
    vAHI_SpiConfigure(LCD_SS, FALSE, FALSE, FALSE, 2, FALSE, TRUE);

    /* Set slave select */
    vAHI_SpiSelect(LCD_SS_MASK);
}

/****************************************************************************
 *
 * NAME: vLcdFreeSpiBus
 *
 * DESCRIPTION:
 * Performs any clear-up after LCD accesses. Currently a null function.
 *
 ****************************************************************************/
PUBLIC void vLcdFreeSpiBus(void)
{
}

/****************************************************************************
 *
 * NAME: vLcdSetCDline
 *
 * DESCRIPTION:
 * Sets the CD line to either high or low, as desired. This is used by the
 * LCD controller to distinguish between commands and pixel data.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u8OnOff         R   Whether line should be high or low
 *
 ****************************************************************************/
PUBLIC void vLcdSetCDline(uint8 u8OnOff)
{
    if (u8OnOff)
    {
        vAHI_DioSetOutput(LCD_CD_BIT_MASK, 0);
    }
    else
    {
        vAHI_DioSetOutput(0, LCD_CD_BIT_MASK);
    }
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vLcdWriteText
 *
 * DESCRIPTION:
 * Puts text into shadow buffer. Text is aligned to a character row but can
 * be at any column. Characters are variable width but 8 pixels high.
 *
 * PARAMETERS:      Name       RW  Usage
 *                  pcString   R   Pointer to zero-terminated string
 *                  u8Row      R   Character row (0-7)
 *                  u8Column   R   Start column (0-127)
 *                  u8Mask     R   Mask to XOR with (0:normal, 0xff:invert)
 *
 * NOTES:
 * To see text, perform a refresh
 *
 ****************************************************************************/
PRIVATE void vWriteText(char *pcString, uint8 u8Row, uint8 u8Column, uint8 u8Mask)
{
    uint8 u8Columns;
    uint8 *pu8CharMap;
    uint8 *pu8Shadow = &au8Shadow[u8Row * 128 + u8Column];

    /* Column before first character */
    *pu8Shadow = u8Mask;
    pu8Shadow++;

    while (*pcString != 0)
    {
        pu8CharMap = pu8LcdFontGetChar(*pcString);
        u8Columns = *pu8CharMap;

        /* Copy character bitmap to shadow memory */
        do
        {
            pu8CharMap++;
            *pu8Shadow = (*pu8CharMap) ^ u8Mask;
            pu8Shadow++;
            u8Columns--;
        } while (u8Columns > 0);

        /* Add a column spacing */
        *pu8Shadow = u8Mask;
        pu8Shadow++;

        pcString++;
    }
}

/****************************************************************************
 *
 * NAME: vWait
 *
 * DESCRIPTION:
 * Waits by looping around for the specified number of iterations.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  iWait           R   Number of iterations to wait for
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vWait(uint32 iWait)
{
    volatile int i;
    for (i = 0; i < iWait; i++);
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
