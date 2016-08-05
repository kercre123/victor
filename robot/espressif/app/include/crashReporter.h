/** @file Header for crash reporter logic on Espressif
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __CRASH_REPORTER_H
#define __CRASH_REPORTER_H

#include "anki/types.h"
#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
  namespace Cozmo {
    namespace CrashReporter {
      /// Initalize the Crash reporter module
      bool Init();
      
      /// Main poll loop update function
      void Update();
      
      /// Start sending crash reports to the engine
      void StartSending();
      
      /// Start querrying the body for any crash reports it has
      void StartQuerry();
      
      /// Accept body NV storage data
      void AcceptBodyStorage(BodyStorageContents& msg);
    }
  }
}

#endif
