/** Implementation of Espressif real time clock interface
 * @author Daniel Casner <daniel@anki.com>
 */

#include "user_interface.h"
#include "driver/rtc.h"

#define WIFI_CHANNEL_MAGIC      (0xf1e0AA00)
#define WIFI_CHANNEL_MAGIC_MASK (0xFFFFFF00)
#define WIFI_CHANNEL_DATA_MASK  (~WIFI_CHANNEL_MAGIC_MASK)

FWImageSelection ICACHE_FLASH_ATTR GetImageSelection(void)
{
  uint32 selectedImage;
  system_rtc_mem_read(RTC_IMAGE_SELECTION, &selectedImage, 4);
  switch (selectedImage)
  {
    case FW_IMAGE_FACTORY:
    case FW_IMAGE_A:
    case FW_IMAGE_B:
      return (FWImageSelection)selectedImage;
    default:
      return FW_IMAGE_INVALID;
  }
}

void ICACHE_FLASH_ATTR SetStoredWiFiChannel(uint8_t channel)
{
  uint32_t channelKey = 0xf1e0AA00 | channel;
  system_rtc_mem_write(RTC_WIFI_CHANNEL, &channelKey, sizeof(uint32_t));
}

uint8_t ICACHE_FLASH_ATTR GetStoredWiFiChannel(void)
{
  uint32_t channelKey;
  system_rtc_mem_read(RTC_WIFI_CHANNEL, &channelKey, 4);
  if ((channelKey & WIFI_CHANNEL_MAGIC_MASK) == WIFI_CHANNEL_MAGIC)
  {
    channelKey &= WIFI_CHANNEL_DATA_MASK;
    if (channelKey < 15) return channelKey;
    else return 0;
  }
  else return 0;
}
