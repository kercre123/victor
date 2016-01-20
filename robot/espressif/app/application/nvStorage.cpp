/** Implementation for non-volatile storage on the Espressif
 * @author Daniel Casner <daniel@anki.com>
 *
 * Theory of operation:
 * All stored data is stored as entries each of which has an NVEntryHeader followed by the entry data. The header is
 * used for retrieving entries and for storing the status of entries.
 *
 * The first two sectors of the NV storage area is used as a staging sector when doing moves or erases. All the
 * remaining sectors are available for data.
 */

#include "nvStorage.h"
extern "C" {
#include "rboot.h"
}
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/esp.h"

#define END_OF_FLASH (0x200000)
#define DATA_START_ADDRESS (NV_STORAGE_START_ADDRESS + (2*SECTOR_SIZE))

namespace Anki {
namespace Cozmo {
namespace NVStorage {

struct NVEntryHeader {
  u32 size;  ///< Size of the entry data plus this header
  u32 tag;   ///< Data identification for this structure
  u32 valid; ///< Left 0xFFFFffff when written, set to 0x00000000 or other code when entry is marked for deletion
};

NVResult Write(NVStorageBlob& entry)
{
  u32 addr = DATA_START_ADDRESS;
  NVEntryHeader header;
  const u32 blobSize  = ((entry.blob_length + 3)/4)*4;
  const u32 entrySize = blobSize + sizeof(NVEntryHeader);
  
  while ((addr + entrySize) < END_OF_FLASH)
  {
    SpiFlashOpResult flashResult = spi_flash_read(addr, reinterpret_cast<uint32*>(&header), sizeof(header));
    AnkiConditionalWarnAndReturnValue(flashResult == SPI_FLASH_RESULT_OK,
                                      flashResult == SPI_FLASH_RESULT_ERR ? NV_ERROR : NV_TIMEOUT, 42, "NVStorage.Write", 261, "Failed to read header %d", 1, flashResult);
    if (header.tag == NVEntry_Invalid) // Have reached the end of storage, put it here.
    {
      header.size  = entrySize;
      header.tag   = entry.tag;
      flashResult = spi_flash_write(addr, reinterpret_cast<uint32*>(&header), sizeof(NVEntryHeader) - 4); // We don't write the valid flag, thus leaving it valid.
      AnkiConditionalWarnAndReturnValue(flashResult == SPI_FLASH_RESULT_OK,
                                        flashResult == SPI_FLASH_RESULT_ERR ? NV_ERROR : NV_TIMEOUT, 42, "NVStorage.Write", 262, "Failed to write new header %d", 1, flashResult);
      addr += sizeof(NVEntryHeader);
      flashResult = spi_flash_write(addr, reinterpret_cast<uint32*>(entry.blob), blobSize);
      if (flashResult != SPI_FLASH_RESULT_OK)
      {
        u32 invalid = 0;
        AnkiWarn( 42, "NVStorage.Write", 263, "Failed to write blob! %d", 1, flashResult);
        const SpiFlashOpResult flashResult2 = spi_flash_write(addr-4, &invalid, 4); // Mark this header as invalid
        AnkiConditionalErrorAndReturnValue(flashResult2 == SPI_FLASH_RESULT_OK,
                                           flashResult2 == SPI_FLASH_RESULT_ERR ? NV_ERROR : NV_TIMEOUT, 42, "NVStorage.Write", 264, "Failed to invalidate header %d on failed blob write %d", 2,
                                           flashResult2, flashResult);
        return flashResult == SPI_FLASH_RESULT_ERR ? NV_ERROR : NV_TIMEOUT;
      }
      else
      {
        return NV_OKAY;
      }
    }
    else if (header.tag == entry.tag && header.valid) // Found a matching valid tag
    {
        u32 invalid = 0;
        flashResult = spi_flash_write(addr + sizeof(NVEntryHeader) - 4, &invalid, 4); // Invaildate this entry
        AnkiConditionalWarnAndReturnValue(flashResult == SPI_FLASH_RESULT_OK,
                                          flashResult == SPI_FLASH_RESULT_ERR ? NV_ERROR : NV_TIMEOUT, 42, "NVStorage.Write", 265, "Failed to invalidate old entry %d", 1, flashResult);
        addr += header.size; // Advance to the next
    }
    else
    {
      addr += header.size; // Advance to next entry
    }
  }
  
  return NV_NO_ROOM;
}

NVResult Erase(const u32 tag)
{
  u32 addr = DATA_START_ADDRESS;
  NVEntryHeader header;
  
  while (addr < END_OF_FLASH)
  {
    SpiFlashOpResult flashResult = spi_flash_read(addr, reinterpret_cast<uint32*>(&header), sizeof(header));
    AnkiConditionalWarnAndReturnValue(flashResult == SPI_FLASH_RESULT_OK,
                                      flashResult == SPI_FLASH_RESULT_ERR ? NV_ERROR : NV_TIMEOUT, 43, "NVStorage.Read", 261, "Failed to read header %d", 1, flashResult);
    if (header.tag == NVEntry_Invalid) // Have reached the end of storage without finding anything
    {
      break;
    }
    else if (header.tag == tag && header.valid)
    {
      u32 invalid = 0;
      flashResult = spi_flash_write(addr + sizeof(NVEntryHeader) - 4, &invalid, 4); // Invaildate this entry
      AnkiConditionalWarnAndReturnValue(flashResult == SPI_FLASH_RESULT_OK,
                                        flashResult == SPI_FLASH_RESULT_ERR ? NV_ERROR : NV_TIMEOUT, 49, "NVStorage.Erase", 274, "Failed to erase tag 0x%x: %d", 2, tag, flashResult);
      return NV_OKAY;
    }
    else
    {
      addr += header.size;
    }
  }

  return NV_NOT_FOUND;
  
}

NVResult Read(NVStorageBlob& entry)
{
  u32 addr = DATA_START_ADDRESS;
  NVEntryHeader header;
  
  while (addr < END_OF_FLASH)
  {
    SpiFlashOpResult flashResult = spi_flash_read(addr, reinterpret_cast<uint32*>(&header), sizeof(header));
    AnkiConditionalWarnAndReturnValue(flashResult == SPI_FLASH_RESULT_OK,
                                      flashResult == SPI_FLASH_RESULT_ERR ? NV_ERROR : NV_TIMEOUT, 43, "NVStorage.Read", 261, "Failed to read header %d", 1, flashResult);
    if (header.tag == NVEntry_Invalid) // Have reached the end of storage without finding anything
    {
      break;
    }
    else if (header.tag == entry.tag && header.valid)
    {
      addr += sizeof(NVEntryHeader);
      entry.blob_length = header.size - sizeof(NVEntryHeader);
      flashResult = spi_flash_read(addr, reinterpret_cast<uint32*>(entry.blob), entry.blob_length);
      AnkiConditionalWarnAndReturnValue(flashResult == SPI_FLASH_RESULT_OK,
                                        flashResult == SPI_FLASH_RESULT_ERR ? NV_ERROR : NV_TIMEOUT, 43, "NVStorage.Read", 266, "Failed to read blob %d", 1, flashResult);
      return NV_OKAY;
    }
    else
    {
      addr += header.size;
    }
  }

  return NV_NOT_FOUND;
}

extern "C" int NVCheckIntegrity(const bool recollect, const bool defragment)
{
  if (recollect)
  {
    AnkiWarn( 44, "NVStorage.CheckIntegrity", 267, "recollect not yet implemented.", 0);
  }
  if (defragment)
  {
    AnkiWarn( 44, "NVStorage.CheckIntegrity", 268, "defragment not yet implemented.", 0);
  }
  return 0;
}

extern "C" void NVWipeAll(void)
{
  // TODO: should use larger erase segment than sector
  for (u16 sector = NV_STORAGE_START_ADDRESS / SECTOR_SIZE; sector < END_OF_FLASH / SECTOR_SIZE; ++sector)
  {
    SpiFlashOpResult flashResult = spi_flash_erase_sector(sector);
    AnkiConditionalErrorAndReturn(flashResult == SPI_FLASH_RESULT_OK, 45, "NVStorage.WipeAll", 269, "Failed to erase sector 0x%x, %d", 2, sector, flashResult);
  }
}

extern "C" bool NVGetWiFiAPConfig(struct softap_config* config, struct ip_info* info)
{
  NVStorageBlob apEntry;
  apEntry.tag = NVEntry_APConfig;
  if (Read(apEntry) == NV_OKAY) // Found wifi configuration
  {
    const WiFiAPConfig* nvApConfig = reinterpret_cast<WiFiAPConfig*>(apEntry.blob);
    memcpy(config->ssid,     nvApConfig->ssid, 32);
    memcpy(config->password, nvApConfig->psk,  64);
    config->channel = nvApConfig->channel;
    config->beacon_interval = nvApConfig->beacon_interval;
    info->ip.addr = nvApConfig->ip_domain;
    info->gw.addr = nvApConfig->ip_domain;
    return true;
  }
  else
  {
    return false;
  }
}

extern "C" u8 NVGetWiFiStaConfig(struct station_config* config, struct ip_info* info)
{
  NVStorageBlob staEntry;
  staEntry.tag = NVEntry_StaConfig;
  switch (Read(staEntry))
  {
    case NV_OKAY:
    {
      const WiFiStaConfig* nvStaConfig = reinterpret_cast<WiFiStaConfig*>(staEntry.blob);
      
      memcpy(config->ssid,     nvStaConfig->ssid, 32);
      memcpy(config->password, nvStaConfig->psk,  64);
      config->bssid_set = 0;
      for (u8 bsc=0; bsc<6; ++bsc)
      {
        config->bssid[bsc] = nvStaConfig->bssid[bsc];
        if (nvStaConfig->bssid[bsc] != 0xff)
        {
          config->bssid_set = 1;
        }
      }
      
      info->ip.addr = nvStaConfig->staticIP;
      info->gw.addr = nvStaConfig->staticGateway;
      info->netmask.addr = nvStaConfig->staticNetmask;
      
      return nvStaConfig->mode;
    }
    case NV_NOT_FOUND:
    {
      return SOFTAP_MODE;
    }
    default:
    {
      return NULL_MODE;
    }
  }
}

} // NVStorage
} // Cozmo
} // Anki
