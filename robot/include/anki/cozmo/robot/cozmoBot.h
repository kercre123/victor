#ifndef ANKI_COZMOBOT_H
#define ANKI_COZMOBOT_H

#include "anki/common/types.h"

namespace Anki {

  namespace Cozmo {

    namespace Robot {

      Result Init();
      void Destroy();

      Result step_MainExecution();

      //
      // State Machine Operation Modes
      //
      enum OperationMode {
        INIT_MOTOR_CALIBRATION,
        WAITING,
        PICK_UP_BLOCK,
        PUT_DOWN_BLOCK,
        FOLLOW_PATH
      };

      OperationMode GetOperationMode();
      void SetOperationMode(OperationMode newMode);


    } // namespace Robot

  } // namespace Cozmo

} // namespace Anki

#endif // ANKI_COZMOBOT_H
