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

#ifndef  CONFIG_H_INCLUDED
#define  CONFIG_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <AppHardwareApi.h>
#include "utils.h"

#include "common.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/* Serial Configuration */
#define UART_BAUD   		115200
#define UART_PARITY_ENABLE	E_AHI_UART_PARITY_DISABLE
#define UART_PARITY_TYPE 	E_AHI_UART_ODD_PARITY // if enabled
#define UART_BITLEN			E_AHI_UART_WORD_LEN_8
#define UART_STOPBITS 		E_AHI_UART_1_STOP_BIT

/* Specify which serial port to use when outputting debug information */
#define UART_PORT			E_AHI_UART_0

/* Specify the PAN ID and CHANNEL to be used by tags, readers and gateway */
#define APP_ID              0x67726305
#define NEKO_ID              0x67726308
#define APP_NAME            "Samp_Monitor"
#define NEKO_NAME            "Samp_Nekotter"
#define CHANNEL             15
#ifdef TWX0003
#define DEFAULT_SENSOR		0x31
#elif LITE2525A
#define DEFAULT_SENSOR		0x35
#else
#define DEFAULT_SENSOR		0x10
#endif
//#define CHMASK              ((1UL << CHANNEL) | (1UL << (CHANNEL+5)) | (1UL << (CHANNEL+10)))
#define CHMASK              (1UL << CHANNEL)

#define DEFAULT_ENC_KEY     (0xA5A5A5A5)
/**
 * メッセージプールの送信周期
 */
#define PARENT_MSGPOOL_TX_DUR_s 10

/**
 * 子機のデフォルトスリープ周期
 */
#define DEFAULT_SLEEP_DUR_ms (5000UL)

/**
 * 温度センサーの利用
 */
#undef USE_TEMP_INSTDOF_ADC2

/**
 * DIO_SUPERCAP_CONTROL の制御閾値
 *
 * - ADC1 に二次電池・スーパーキャパシタの電圧を 1/2 に分圧して入力
 * - 本定義の電圧以上になると、DIO_SUPERCAP_CONTROL を LO に設定する
 *   (1300なら 2.6V を超えた時点で IO が LO になる)
 * - 外部の電源制御回路の制御用
 */
#define VOLT_SUPERCAP_CONTROL 1300

/*************************************************************************/
/***        TARGET PCB                                                 ***/
/*************************************************************************/
#define DIO_BUTTON (PORT_INPUT1)         // DI1
#define DIO_VOLTAGE_CHECKER (PORT_OUT1)  // DO1: 始動後速やかに LO になる
#define DIO_SUPERCAP_CONTROL (PORT_OUT2) // DO2: SUPER CAP の電圧が上昇すると LO に設定
#define DIO_SNS_POWER (PORT_OUT3)        // DO3: センサー制御用(稼働中だけLOになる)

#ifdef LITE2525A
#define LED (5)
#else
#define LED (0)
#endif

#define PORT_INPUT_MASK ( 1UL << DIO_BUTTON)
//#define PORT_INPUT_MASK_ADXL345 ( (1UL << PORT_INPUT2) | (1UL <<  PORT_INPUT3))
#define PORT_INPUT_MASK_ADXL345 ( (1UL << DIO_BUTTON ) | (1UL <<  PORT_INPUT3) )
#define PORT_INPUT_MASK_ACL ( (1UL << PORT_INPUT2 ) | (1UL <<  DIO_BUTTON) )

#ifdef PARENT
# define sAppData sAppData_Pa
#endif
#ifdef ROUTER
# define sAppData sAppData_Ro
#endif
#ifdef ENDDEVICE_INPUT
# define sAppData sAppData_Ed
#endif
#ifdef ENDDEVICE_REMOTE
# define sAppData sAppData_Re
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* CONFIG_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
