/****************************************************************************
 * (C) Tokyo Cosmos Electric, Inc. (TOCOS) - 2013 all rights reserved.
 *
 * Condition to use:
 *   - The full or part of source code is limited to use for TWE (TOCOS
 *     Wireless Engine) as compiled and flash programmed.
 *   - The full or part of source code is prohibited to distribute without
 *     permission from TOCOS.
 *
 ****************************************************************************/
/*
 * LcdPrint.h
 *
 *  Created on: 2012/12/28
 *      Author: seigo13
 */

#ifndef LCDPRINT_H_
#define LCDPRINT_H_

#include <jendefs.h>

PUBLIC bool_t LCD_bTxChar(uint8 u8SerialPort, uint8 u8Data);
PUBLIC bool_t LCD_bTxBottom(uint8 u8SerialPort, uint8 u8Data);
PUBLIC void vDrawLcdDisplay(uint32 u32Xoffset, uint8 bClearShadow);
PUBLIC void vDrawLcdInit();
extern const uint8 au8TocosHan[];
extern const uint8 au8TocosZen[];
extern const uint8 au8Tocos[];

#define CHR_BLK 0x7f
#define CHR_BLK_50 0x80

#endif /* LCDPRINT_H_ */
