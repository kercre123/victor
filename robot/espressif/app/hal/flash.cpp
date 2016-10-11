/** Espressif flash interface HAL.
 * @author Daniel Casner <daniel@anki.com>
 */

#include "anki/cozmo/robot/flash_map.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"

extern "C" {
  #include "user_interface.h"
  #include "osapi.h"
  #include "driver/rtc.h"
}

#define OLD_NV_STORAGE_SECTOR (0x1c0)

static bool writeOkay(const u32 address, const u32 length)
{
  if ((address + length) <= address) return false; // Check for integer overflow or 0 length
  // NVStorage region is default okay region
  else if ((address >= (NV_STORAGE_SECTOR * SECTOR_SIZE)) && ((address + length) <= (OLD_NV_STORAGE_SECTOR * SECTOR_SIZE))) return true;
  else // App image regions also okay if for other image
  {
    switch (GetImageSelection())
    {
      case FW_IMAGE_A:
      { // Am image A so writing to B okay
        if ((address >= (APPLICATION_B_SECTOR * SECTOR_SIZE)) && ((address + length) <= (NV_STORAGE_SECTOR * SECTOR_SIZE))) return true;
        else if ((address >= (APPLICATION_A_SECTOR * SECTOR_SIZE + ESP_FW_MAX_SIZE - ESP_FW_NOTE_SIZE)) && ((address + length) <= (FACTORY_WIFI_FW_SECTOR * SECTOR_SIZE))) return true;
        else break;
      }
      case FW_IMAGE_B:
      { // Am image B so writing to A okay
        if ((address >= (APPLICATION_A_SECTOR * SECTOR_SIZE)) && ((address + length) <= (FACTORY_WIFI_FW_SECTOR * SECTOR_SIZE))) return true;
        else if ((address >= APPLICATION_B_SECTOR * SECTOR_SIZE + ESP_FW_MAX_SIZE - ESP_FW_NOTE_SIZE) && ((address + length) <= (NV_STORAGE_SECTOR * SECTOR_SIZE))) return true;
        else break;
      }
      default:
        break;
    }
    os_printf("FLASH %x[%x] not allowed\r\n", address, length);
    return false;
  }
}

// Converts SpiFlashOpResult codes to NVResult codes
#define RSLT_CONV(rslt) (((rslt) == SPI_FLASH_RESULT_OK) ? NV_OKAY : (((rslt) == SPI_FLASH_RESULT_ERR) ? NV_ERROR : NV_TIMEOUT))

Anki::Cozmo::NVStorage::NVResult Anki::Cozmo::HAL::FlashWrite(u32 address, u32* data, u32 length)
{
  using namespace Anki::Cozmo::NVStorage;
  
  if ((int)data & 0x3) return NV_BAD_ARGS; // Check for alinment
  else if (writeOkay(address, length))
  {
    return RSLT_CONV(spi_flash_write(address, data, length));
  }
  else return NV_BAD_ARGS;
}

Anki::Cozmo::NVStorage::NVResult Anki::Cozmo::HAL::FlashRead (u32 address, u32* data, u32 length)
{
  using namespace Anki::Cozmo::NVStorage;
  
  if ((int)data & 0x3) return NV_BAD_ARGS; // Check for alinment
  else return RSLT_CONV(spi_flash_read(address, data, length));
}

Anki::Cozmo::NVStorage::NVResult Anki::Cozmo::HAL::FlashErase(u32 address)
{
  using namespace Anki::Cozmo::NVStorage;
  
  if ((int)address & SECTOR_MASK) return NV_BAD_ARGS;
  else if (writeOkay(address, SECTOR_SIZE))
  {
    u16 sector = address / SECTOR_SIZE;
    return RSLT_CONV(spi_flash_erase_sector(sector));
  }
  else return NV_BAD_ARGS;
}
