#ifndef ANKI_COZMOBOT_H
#define ANKI_COZMOBOT_H

#include "coretech/common/shared/types.h"

namespace Anki {

  namespace Cozmo {

    namespace Robot {

      Result Init(const int * shutdownSignal);

      void Destroy();

      Result step_MainExecution();

      void CalibrateMotorsOnNextCalmModeExit(bool enable);

    } // namespace Robot

  } // namespace Cozmo

} // namespace Anki

#endif // ANKI_COZMOBOT_H
