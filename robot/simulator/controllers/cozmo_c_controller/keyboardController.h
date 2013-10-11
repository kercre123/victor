#ifndef KEYBOARD_CONTROLLER_H
#define KEYBOARD_CONTROLLER_H

#include "anki/cozmo/robot/cozmoTypes.h"

namespace Anki {
  namespace Cozmo {
    namespace KeyboardController {

      void Init(webots::Robot &robot);
      void Enable(void);
      void Disabl(void);
      bool IsEnabled(void);
      void ProcessKeystroke(void);

      
    } // namespace KeyboardController
  } // namespace Cozmo
} // namespace Anki



#endif //KEYBOARD_CONTROLLER_H