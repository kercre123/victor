#ifndef __ESPRESSIF_DRIVER_RTC_H_
#define __ESPRESSIF_DRIVER_RTC_H_
/** @file Espressif real time clock, and associated RAM interface
 * @author Daniel Casner <daniel@anki.com>
 */

#include "anki/cozmo/robot/flash_map.h"

/** Return which image is currently running
 */
FWImageSelection GetImageSelection(void);

/** Stores wifi channel number in RTC through reboots
 */
void SetStoredWiFiChannel(uint8_t channel);

/** Retrieves wifi channel number stored in RTC across boots
 */
uint8_t GetStoredWiFiChannel(void);

#endif
