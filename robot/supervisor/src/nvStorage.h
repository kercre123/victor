/** @file Robot non-voltatile data storage header
 * This module is intended for storing small amounts of persistent data on the robot
 * @author Daniel Casner <daniel@anki.com>
 */
 
#ifndef __SUPERVISOR_NV_STORAGE_H
#define __SUPERVISOR_NV_STORAGE_H

#include "anki/cozmo/robot/hal.h"
#include "clad/types/nvStorage.h"
#include "anki/cozmo/robot/flash_map.h"
#define NV_STORAGE_START_ADDRESS (NV_STORAGE_SECTOR * SECTOR_SIZE)
#define NV_STORAGE_END_ADDRESS   (ESP_INIT_DATA_SECTOR * SECTOR_SIZE)
#define NV_STORAGE_CHUNK_SIZE    (1024)

namespace Anki {
  namespace Cozmo {
    namespace NVStorage {
      
      /** Callback for NV operation completes
       */
      typedef void (*NVOperationCallback)(NVOpResult&);
      
      /** Initalizes NVStorage component
       */
      bool Init();
      
      /// Run pending tasks for NVStorage system and call any pending callbacks
      Result Update();
      
      /** Command an NVStorage operation
       * @param cmd Specifies all the parameters of the operation
       * @param callback Will be called when the operation is complete.
       * @return Should be NV_SCHEDULED or an error.
       */
      NVResult Command(const NVCommand& cdm, NVOperationCallback callback);
    }
  }
}

#endif
