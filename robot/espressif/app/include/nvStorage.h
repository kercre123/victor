/** @file Non Volatile storage on the Espressif Header
 * This module is intended for storing small amounts of persistent data on the robot. Total storage < 256kB and each
 * stored element <4kb.
 * Due to SPI flash timing restrictions, almost all operations are done with only flash reads. See maintenance functions
 * below for details.
 * @author Daniel Casner <daniel@anki.com>
 */
 
#ifndef __ESP_APPLICATION_NV_STORAGE_H
#define __ESP_APPLICATION_NV_STORAGE_H

extern "C" {
  #include "user_interface.h"
}
#include "anki/types.h"
#include "clad/types/nvStorage.h"

/// Storage starts at 256 bytes before the end of the 2MB flash
#define NV_STORAGE_START_ADDRESS (0x1c0000)

namespace Anki {
  namespace Cozmo {
    namespace NVStorage {
      
      /** Writes an NV storage entry into flash.
       * @param[in] entry The data to write. If the entry tag matches and existing entry, the existing entry will be
       *              overwritten.
       * @return Result code for the operation
       */
      NVResult Write(NVStorageBlob& entry);
      
      /** Erases (invalidates) entry for given tag if present
       * @param tag The tag to erase
       * @return Result code for the operation, NV_NOT_FOUND shouldn't be considered an error in this case since there
       *                being no entry matching the tag means there was no nothing needing to be erased.
       */
      NVResult Erase(const u32 tag);
      
      /** Reads an NV storage entry from flash.
       * @param[inout] entry A pointer to an NVStorageBlob instance. The tag should be set to the tag to be retrieved.
       * @return Result code for the read
       */
       NVResult Read(NVStorageBlob& entry);
       
       extern "C" {
         /** Checks the integrity of the non-volatile storge flash file system and completes and interrupted operations.
          * If either recollect or defragment is specified, this function will do SPI erase operations which may
          * interfere with processor scheduling. This should only be done at startup or shutdown.
          * @param recollect If true any blocks marked for erasure will be erased and then usable for new data again
          * @param defragment If true fragmented blocks will be defragmented to try and free up space.
          * @return The number of operations completed.
          */
        int NVCheckIntegrity(const bool recollect, const bool defragment);
        
        /** Erase the entire contents of NV storage destroying all data.
         * This function will interfere with CPU scheduling so it should only be called during startup or shutown.
         */
        void NVWipeAll(void);
        
         /** Retrieve non-volatile WiFi AP configuration if present
          * @param[out] config A softap_config structure to write configuration into
          * @param[out] info A pointer to write IP address configuration into
          * @return true if NV configuration was found, false if nothing available.
          */
         bool NVGetWiFiAPConfig(struct softap_config* config, struct ip_info* info);
         
          /** Retrieve non-volatile WiFi station configuration if present
           * @param[out] config A station_config structure to write data into
           * @param[out] info A pointer to write IP address configuration into, if DHCP is to be used, the IP address
           *                  will be set to 255.255.255.255.
           * @return NULL_MODE      On error
           *         STATION_MODE   If station info was found and was set to operate without AP
           *         SOFTAP_MODE    If no station info was found or it was set to inactive
           *         STATIONAP_MODE If station info was found and was set to operate with AP
           */
          u8 NVGetWiFiStaConfig(struct station_config* config, struct ip_info* info);
       }
    }
  }
}

#endif
