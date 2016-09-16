#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "anki/cozmo/robot/flash_map.h"
#include "driver/factoryData.h"

#define RANDOM_DATA_OFFSET FACTORY_DATA_SIZE
#define MAX_RANDOM_DATA (8*sizeof(uint32_t))

#define FACTORY_DATA_SIZE (8)

int COZMO_VERSION_ID = 0;
unsigned int COZMO_BUILD_DATE = 0;

static uint32_t factoryData[FACTORY_DATA_SIZE/sizeof(uint32_t)];
static uint32_t factoryFirmwareVersion;

void ICACHE_FLASH_ATTR factoryDataInit(void)
{
  if (spi_flash_read(FACTORY_SECTOR * SECTOR_SIZE, factoryData, FACTORY_DATA_SIZE) != SPI_FLASH_RESULT_OK)
  {
    os_memset(factoryData, 0xff, FACTORY_DATA_SIZE);
  }
  if ((factoryData[0] & 0xF0000000) == 0xF0000000) // Invalid serial number
  {
    spi_flash_read(FACTORY_SECTOR * SECTOR_SIZE + RANDOM_DATA_OFFSET + 16, factoryData, 4);
  }
  spi_flash_read((FACTORY_WIFI_FW_SECTOR * SECTOR_SIZE) + 0x4A0, &factoryFirmwareVersion, sizeof(uint32_t));
}

uint32_t ICACHE_FLASH_ATTR getSerialNumber(void)
{
  return factoryData[0];
}

uint32_t ICACHE_FLASH_ATTR getSSIDNumber(void)
{
  const uint32_t sn = getSerialNumber();
  const uint32_t fix = sn >> 24;
  if (factoryFirmwareVersion < 0x857b164) return (((sn & 0x00FF0000) ^ (fix << 16)) & 0x77) | ((sn & 0x0000FF00) ^ (fix << 8)) | ((sn & 0x000000FF) ^ fix);
  else return (((sn & 0x00FF0000) ^ (fix << 16)) & 0x770000) | ((sn & 0x0000FF00) ^ (fix << 8)) | ((sn & 0x000000FF) ^ fix);
}

uint32_t ICACHE_FLASH_ATTR getFactoryFirmwareVersion(void)
{
  return factoryFirmwareVersion;
}

uint16_t ICACHE_FLASH_ATTR getModelNumber(void)
{
  return factoryData[1] & 0xffff;
}

bool ICACHE_FLASH_ATTR getFactoryRandomSeed(uint32_t* dest, const int len)
{
  if (len > MAX_RANDOM_DATA) return false;
  else return (spi_flash_read(FACTORY_SECTOR * SECTOR_SIZE + RANDOM_DATA_OFFSET, dest, len) == SPI_FLASH_RESULT_OK);
}
