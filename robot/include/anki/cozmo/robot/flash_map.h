#ifndef __FLASH_MAP_H__
#define __FLASH_MAP_H__
/*******************************************************************************
 * @file Espressif flash memory map
 * @author Daniel Casner <daniel@anki.com>
 * See :robot/espressif_bootloader/README.md for details
 */

#ifdef __cplusplus
extern "C" {
#endif

/// Flash sector size always 4kb
#define SECTOR_SIZE (0x1000)
/// Flash block size (for faster block erasure), depends on chip, might be 0x10000
#define BLOCK_SIZE (0x8000)
/// A mask to check that an address is the start of a sector
#define SECTOR_MASK (SECTOR_SIZE-1)

/// Map of the sectors of flash where various things are stored
typedef enum {
  BOOTLOADER_SECTOR           = 0x000, ///< Where the boot loader (this code) lives.
  FACTORY_SECTOR              = 0x001, ///< Where factory build information will be stored
  CRASH_DUMP_SECTOR           = 0x002, ///< Where we will store crash dumps for reporting
  APPLICATION_A_SECTOR        = 0x003, ///< Start of application image A region
  APPLICATION_B_SECTOR        = 0x103, ///< Start of application image B region
  FACTORY_WIFI_FW_SECTOR      = 0x080, ///< Where the factory firmware starts
  FACTORY_RTIP_BODY_FW_SECTOR = 0x0c5, ///< Where the image for the RTIP and Body firmware is stored
  FACTORY_NV_STORAGE_SECTOR   = 0x0de, ///< A region used for storing large factory test data
  FIXTURE_STORAGE_SECTOR      = 0x0fc, ///< 16KB for fixture data
  NV_STORAGE_SECTOR           = 0x180, ///< Start of NV Storage region
  ESP_INIT_DATA_SECTOR        = 0x1fc, ///< Where the Espressif OS keeps it's init data, two sectors long
  ESP_WIFI_CFG_SECTOR         = 0x1fe, ///< Where the Espressif OS keeps it's wifi configuration data, two sectors long
} FlashSector;

/// Map of data stored in the RTC
typedef enum {
  RTC_SYSTEM_RESERVED = 0x00,
  RTC_IMAGE_SELECTION = 0xbf,
} RTCMemAddress;

/// Enum for boot images
// Use complex bit pattern to make error rejection easier
// Use amusing bit pattern because we are moving the code around
typedef enum {
  FW_IMAGE_FACTORY = 0x0000C0DE,
  FW_IMAGE_A       = 0x00C0DE00,
  FW_IMAGE_B       = 0xC0DE0000,
} FWImageSelection;

#define FLASH_CACHE_POINTER ((uint32_t*)(0x40200000))

#ifdef __cplusplus
} // End extern "C"
#endif

#endif
