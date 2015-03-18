/****************************************************************************
 * (C) Tokyo Cosmos Electric, Inc. (TOCOS) - all rights reserved.
 *
 * Condition to use: (refer to detailed conditions in Japanese)
 *   - The full or part of source code is limited to use for TWE (TOCOS
 *     Wireless Engine) as compiled and flash programmed.
 *   - The full or part of source code is prohibited to distribute without
 *     permission from TOCOS.
 *
 * 利用条件:
 *   - 本ソースコードは、別途ソースコードライセンス記述が無い限り東京コスモス電機が著作権を
 *     保有しています。
 *   - 本ソースコードは、無保証・無サポートです。本ソースコードや生成物を用いたいかなる損害
 *     についても東京コスモス電機は保証致しません。不具合等の報告は歓迎いたします。
 *   - 本ソースコードは、東京コスモス電機が販売する TWE シリーズ上で実行する前提で公開
 *     しています。他のマイコン等への移植・流用は一部であっても出来ません。
 *
 ****************************************************************************/


#ifndef COMMON_H_
#define COMMON_H_

#include <serial.h>
#include <fprintf.h>

#include "ToCoNet.h"

void vDispInfo(tsFILE *psSerStream, tsToCoNet_NwkLyTr_Context *pc);

extern const uint32 u32DioPortWakeUp;
void vSleep(uint32 u32SleepDur_ms, bool_t bPeriodic, bool_t bDeep);

bool_t bTransmitToParent(tsToCoNet_Nwk_Context *pNwk, uint8 *pu8Data, uint8 u8Len);

bool_t bRegAesKey(uint32 u32seed);

extern const uint8 au8EncKey[];

/*
 * パケット識別子
 */
#define PKT_ID_STANDARD 0x10
#define PKT_ID_LM61 0x11
#define PKT_ID_SHT21 0x31
#define PKT_ID_ADT7410 0x32
#define PKT_ID_MPL115A2 0x33
#define PKT_ID_LIS3DH 0x34
#define PKT_ID_IO_TIMER 0x51
#define PKT_ID_UART 0x81
#define PKT_ID_BOTTON 0xFE

/*
 * 標準ポート定義 (TWE-Lite DIP)
 */
#if defined(JN516x)
#if defined (USE_TOCOSTICK)
// ToCoStick 用
#warning "IO CONF IS FOR TOCOSTICK!"
#define PORT_OUT1 16 // DIO16/18 をスワップ
#define PORT_OUT2 19
#define PORT_OUT3 4
#define PORT_OUT4 9

#define PORT_INPUT1 12
#define PORT_INPUT2 13
#define PORT_INPUT3 11
#define PORT_INPUT4 18 // DIO16/18 をスワップ

#define PORT_CONF1 10
#define PORT_CONF2 2
#define PORT_CONF3 3

#define PORT_BAUD 17
#define PORT_UART0_RX 7

#else	//	USE_TOCOSTICK
#define PORT_OUT1 18
#define PORT_OUT2 19
#define PORT_OUT3 4
#define PORT_OUT4 9
#define PORT_INPUT1 12
#define PORT_INPUT2 13
#define PORT_INPUT3 11
#define PORT_INPUT4 16
#define PORT_CONF1 10
#define PORT_CONF2 2
#define PORT_CONF3 3
#define PORT_BAUD 17
#define PORT_UART0_RX 7
#endif	//	USE_TOCOSTICK
#elif defined(JN514x)
#define PORT_OUT1 PORT_KIT_LED1
#define PORT_OUT2 PORT_KIT_LED2
#define PORT_OUT3 PORT_KIT_LED3
#define PORT_OUT4 PORT_KIT_LED4
#define PORT_INPUT1 PORT_KIT_SW2
#define PORT_INPUT2 5
#define PORT_INPUT3 PORT_KIT_SW3
#define PORT_INPUT4 PORT_KIT_SW4
#define PORT_CONF1 1
#define PORT_CONF2 PORT_KIT_SW1
#define PORT_CONF3 8
#define PORT_BAUD 12
#define PORT_UART0_RX 7
#endif

#endif /* COMMON_H_ */
