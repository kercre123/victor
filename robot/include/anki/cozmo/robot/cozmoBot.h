#ifndef ANKI_COZMOBOT_H
#define ANKI_COZMOBOT_H

#include "coretech/common/shared/types.h"

namespace Anki {

  namespace Vector {

    namespace Robot {

      Result Init(const int * shutdownSignal);

      void Destroy();

      Result step_MainExecution();

    } // namespace Robot

  } // namespace Vector

} // namespace Anki

#endif // ANKI_COZMOBOT_H
