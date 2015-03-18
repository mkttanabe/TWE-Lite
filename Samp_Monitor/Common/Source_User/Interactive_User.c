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

// 本ファイルは Interactive.c から include される

/**
 * フラッシュ設定構造体をデフォルトに巻き戻します。
 * - ここでシステムのデフォルト値を決めています。
 *
 * @param p 構造体へのアドレス
 */
static void Config_vSetDefaults(tsFlashApp *p) {
	p->u32appid = APP_ID;
	p->u32chmask = CHMASK;
	p->u8ch = CHANNEL;
	p->u8pow = 3;
	p->u8id = 0;

#ifdef PARENT
	p->u32baud_safe = UART_BAUD;
	p->u8parity = 0;
#endif

#ifdef ENDDEVICE_INPUT
	p->u16RcClock = 10000;
	p->u32Slp = DEFAULT_SLEEP_DUR_ms;
	p->u8wait = 30;
	p->u8mode = 0x10;
	p->i16param = 0;
#endif

	p->u32Opt = E_APPCONF_OPT_TO_ROUTER; // デフォルトの設定ビット

#ifdef ROUTER
	p->u8layer = 0;
#endif

	p->u32EncKey = DEFAULT_ENC_KEY;
}
