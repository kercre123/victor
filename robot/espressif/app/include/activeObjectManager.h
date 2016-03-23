/** @file Header file for active object managager on the Espressif
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __ACTIVE_OBJECT_MANAGER_h
#define __ACTIVE_OBJECT_MANAGER_h

#include "anki/types.h"


namespace Anki {
  namespace Cozmo {
    namespace ActiveObjectManager {
      /// Initalize the ActiveObjectManager module
      bool Init();
      
      /// Main poll loop update function
      void Update();
      
      /// Update some or all of the slot settings
      void SetSlots(const u8 start, const u8 count, const u32* const factory_ids);
      
      /// Disconnect one slot
      void Disconnect(const u8 slot);
      
      /// Disconnect all slots
      void DisconnectAll(void);
    }
  }
}

#endif
