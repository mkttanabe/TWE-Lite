/****************************************************************************
 * (C) Tokyo Cosmos Electric, Inc. (TOCOS) - 2013 all rights reserved.
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

#include <jendefs.h>
#include <string.h>

#include <AppHardwareApi.h>
#include <AppApi.h>

#ifdef JN516x
# define USE_EEPROM //!< JN516x でフラッシュを使用する
#endif

#include "flash.h"
#include "ccitt8.h"

#include "config.h"

#ifdef USE_EEPROM
# include "eeprom_6x.h"
#endif

#define FLASH_MAGIC_NUMBER (0xA501EF5A ^ APP_ID) //!< フラッシュ書き込み時のマジック番号  @ingroup FLASH

/** @ingroup FLASH
 * フラッシュの読み込み
 * @param psFlash 読み込み格納データ
 * @param sector 読み出しセクタ
 * @param offset セクタ先頭からのオフセット
 * @return TRUE:読み出し成功 FALSE:失敗
 */
bool_t bFlash_Read(tsFlash *psFlash, uint8 sector, uint32 offset) {
    bool_t bRet = FALSE;
    offset += (uint32)sector * FLASH_SECTOR_SIZE; // calculate the absolute address


#ifdef USE_EEPROM
    if (EEP_6x_bRead(0, sizeof(tsFlash), (uint8 *)psFlash)) {
    	bRet = TRUE;
    }
#else
    if (bAHI_FlashInit(FLASH_TYPE, NULL) == TRUE) {
        if (bAHI_FullFlashRead(offset, sizeof(tsFlash), (uint8 *)psFlash)) {
            bRet = TRUE;
        }
    }
#endif

    // validate content
    if (bRet && psFlash->u32Magic != FLASH_MAGIC_NUMBER) {
    	bRet = FALSE;
    }
    if (bRet && psFlash->u8CRC != u8CCITT8((uint8*)&(psFlash->sData), sizeof(tsFlashApp))) {
    	bRet = FALSE;
    }

    return bRet;
}

/**  @ingroup FLASH
 * フラッシュ書き込み
 * @param psFlash 書き込みたいデータ
 * @param sector 書き込みセクタ
 * @param offset 書き込みセクタ先頭からのオフセット
 * @return TRUE:書き込み成功 FALSE:失敗
 */
bool_t bFlash_Write(tsFlash *psFlash, uint8 sector, uint32 offset)
{
    bool_t bRet = FALSE;
    offset += (uint32)sector * FLASH_SECTOR_SIZE; // calculate the absolute address

#ifdef USE_EEPROM
	psFlash->u32Magic = FLASH_MAGIC_NUMBER;
	psFlash->u8CRC = u8CCITT8((uint8*)&(psFlash->sData), sizeof(tsFlashApp));
	if (EEP_6x_bWrite(0, sizeof(tsFlash), (uint8 *)psFlash)) {
		bRet = TRUE;
	}
#else
    if (bAHI_FlashInit(FLASH_TYPE, NULL) == TRUE) {
        if (bAHI_FlashEraseSector(sector) == TRUE) { // erase a corresponding sector.
        	psFlash->u32Magic = FLASH_MAGIC_NUMBER;
        	psFlash->u8CRC = u8CCITT8((uint8*)&(psFlash->sData), sizeof(tsFlashApp));
            if (bAHI_FullFlashProgram(offset, sizeof(tsFlash), (uint8 *)psFlash)) {
                bRet = TRUE;
            }
        }
    }
#endif

    return bRet;
}

/**  @ingroup FLASH
 * フラッシュセクター消去
 * @return TRUE:書き込み成功 FALSE:失敗
 */
bool_t bFlash_Erase(uint8 sector)
{
    bool_t bRet = FALSE;

#ifdef USE_EEPROM
	int i;
    // EEPROM の全領域をクリアする。セグメント単位で消去する
    uint8 au8buff[EEPROM_6X_SEGMENT_SIZE];
    memset (au8buff, 0xFF, EEPROM_6X_SEGMENT_SIZE);

    bRet = TRUE;
    for (i = 0; i < EEPROM_6X_USER_SEGMENTS; i++) {
		bRet &= EEP_6x_bWrite(i * EEPROM_6X_SEGMENT_SIZE, EEPROM_6X_SEGMENT_SIZE, au8buff);
    }
#else
    if (bAHI_FlashInit(FLASH_TYPE, NULL) == TRUE) {
        if (bAHI_FlashEraseSector(sector) == TRUE) { // erase a corresponding sector.
        	bRet = TRUE;
        }
    }
#endif

    return bRet;
}
