/** @file Interface for factory tests run on the robot Espressif MCU
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __FACTORY_TESTS_h
#define __FACTORY_TESTS_h

#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
  namespace Cozmo {
    namespace Factory {
      /// Initalize the RTIP module
      bool Init();
      
      /// Main loop tick, called from background task
      void Update();
      
      /// Get the current factory test mode
      RobotInterface::FactoryTestMode GetMode();
      
      int GetParam();
      
      void SetMode(const RobotInterface::FactoryTestMode mode,
                   const uint32_t timeout = 0,  //0 means keep existing timeout
                   const int param=0);
      
      /// Process test state update messages
      void Process_TestState(const RobotInterface::TestState& state);
      
      /// Process factory test mode messages
      void Process_EnterFactoryTestMode(const RobotInterface::EnterFactoryTestMode& msg);
    }
  }
}

#endif
