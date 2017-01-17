/** Espressif flash interface HAL.
 * @author Daniel Casner <daniel@anki.com>
 */

#include "anki/cozmo/robot/flash_map.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"

extern "C" {
  #include "user_interface.h"
  #include "osapi.h"
}

#define OLD_NV_STORAGE_A_SECTOR (0x1c0)
#define OLD_NV_STORAGE_B_SECTOR (0x1de)
#define NVSTORAGE_END_SECTOR (ESP_INIT_DATA_SECTOR)
#define FACTORY_NVSTORAGE_END_SECTOR (FIXTURE_STORAGE_SECTOR)
#define OLD_NV_AREA_MAGIC (0xDEADBEEF)

static bool correctOldNVHeader(const uint16_t sector)
{
  uint32_t nvareaheader[3];
  SpiFlashOpResult rslt = spi_flash_read(sector * SECTOR_SIZE, nvareaheader, sizeof(nvareaheader));
  if (rslt != SPI_FLASH_RESULT_OK)
  {
    os_printf("FlashInit couldn't read OLD_NV_STORAGE at sector %x to check header, %d\r\n", sector, rslt);
    return false;
  }
  else if (nvareaheader[0] != OLD_NV_AREA_MAGIC)
  {
    if (nvareaheader[0] != 0xFFFFffff) // Sector isn't already blank
    {
      rslt = spi_flash_erase_sector(sector);
      if (rslt != SPI_FLASH_RESULT_OK)
      {
        os_printf("FlashInit ERROR: OLD_NV_STORAGE header at %x is wrong and couldn't be erased! %d\r\n", sector, rslt);
        return false;
      }
    }
    nvareaheader[0] = OLD_NV_AREA_MAGIC;
    nvareaheader[1] = 0xFFFFffff; // Storage version -1
    nvareaheader[2] = 0; // Journal number 0, minimum valid
    rslt = spi_flash_write(sector * SECTOR_SIZE, nvareaheader, sizeof(nvareaheader));
    if (rslt != SPI_FLASH_RESULT_OK)
    {
      os_printf("FlashInit couldn't write repaired header to sector %x, %d\r\n", sector, rslt);
      return false;
    }
    else return true;
  }
  else return true;
}

void Anki::Cozmo::HAL::FlashInit()
{
  // Make sure there is an NV entry header to make sure we don't trigger the WipeAll factory firmware bug.
  if (!correctOldNVHeader(OLD_NV_STORAGE_A_SECTOR)) correctOldNVHeader(OLD_NV_STORAGE_B_SECTOR);
}

// Prevent writing to places that could cause damage or security breach
bool Anki::Cozmo::HAL::FlashWriteOkay(const u32 address, const u32 length)
{
  if ((address + length) <= address) return false; // Check for integer overflow or 0 length
  // NVStorage region is default okay region
  else if ((address >= (NV_STORAGE_SECTOR * SECTOR_SIZE)) && ((address + length) <= (OLD_NV_STORAGE_A_SECTOR * SECTOR_SIZE))) return true; // First allowable segment of NVStorage
  else if ((address >= ((OLD_NV_STORAGE_A_SECTOR + 1) * SECTOR_SIZE)) && ((address + length) <= (OLD_NV_STORAGE_B_SECTOR * SECTOR_SIZE))) return true; // Second allowable segment of NVStorage
  else if ((address >= ((OLD_NV_STORAGE_B_SECTOR + 1) * SECTOR_SIZE)) && ((address + length) <= (NVSTORAGE_END_SECTOR * SECTOR_SIZE))) return true; // Third allowable segment of NVStorage
  else if (ALLOW_FACTORY_NV_WRITE &&
           (address >= ((FACTORY_NV_STORAGE_SECTOR) * SECTOR_SIZE)) && ((address + length) <= (FACTORY_NVSTORAGE_END_SECTOR * SECTOR_SIZE))) return true; // Factory NVStorage only in Factory Builds
  else return false;
}

// Prevent reading back firmware etc.
static bool readOkay(const u32 address, const u32 length)
{
  if ((address + length) <= address) return false; // Check for integer overflow or 0 length;
  else if ((address >= (FACTORY_NV_STORAGE_SECTOR * SECTOR_SIZE)) && ((address + length) <= (DHCP_MARKER_SECTOR * SECTOR_SIZE))) return true;
  else if ((address >= (NV_STORAGE_SECTOR * SECTOR_SIZE)) && ((address + length) <= (ESP_INIT_DATA_SECTOR * SECTOR_SIZE))) return true;
  else return false;
}

// Converts SpiFlashOpResult codes to NVResult codes
#define RSLT_CONV(rslt) (((rslt) == SPI_FLASH_RESULT_OK) ? NV_OKAY : (((rslt) == SPI_FLASH_RESULT_ERR) ? NV_ERROR : NV_TIMEOUT))

Anki::Cozmo::NVStorage::NVResult Anki::Cozmo::HAL::FlashWrite(u32 address, u32* data, u32 length)
{
  using namespace Anki::Cozmo::NVStorage;
  
  if ((int)data & 0x3) return NV_BAD_ARGS; // Check for alignment
  else if (FlashWriteOkay(address, length))
  {
    return RSLT_CONV(spi_flash_write(address, data, length));
  }
  else return NV_BAD_ARGS;
}

Anki::Cozmo::NVStorage::NVResult Anki::Cozmo::HAL::FlashRead (u32 address, u32* data, u32 length)
{
  using namespace Anki::Cozmo::NVStorage;
  
  if ((int)data & 0x3) return NV_BAD_ARGS; // Check for alignment
  else if (readOkay(address, length))
  {
    return RSLT_CONV(spi_flash_read(address, data, length));
  }
  else return NV_BAD_ARGS;
}

Anki::Cozmo::NVStorage::NVResult Anki::Cozmo::HAL::FlashErase(u32 address)
{
  using namespace Anki::Cozmo::NVStorage;
  
  if ((int)address & SECTOR_MASK) return NV_BAD_ARGS;
  else if (FlashWriteOkay(address, SECTOR_SIZE))
  {
    u16 sector = address / SECTOR_SIZE;
    return RSLT_CONV(spi_flash_erase_sector(sector));
  }
  else return NV_BAD_ARGS;
}
