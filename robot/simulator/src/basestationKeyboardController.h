#ifndef BASESTATION_KEYBOARD_CONTROLLER_H
#define BASESTATION_KEYBOARD_CONTROLLER_H

namespace Anki {
  namespace Cozmo {
    class RobotManager;
    class BlockWorld;
    class BehaviorManager;
    
    namespace Sim {
      namespace BSKeyboardController {

        void Init(RobotManager *robotMgr, BlockWorld* blockWorld, BehaviorManager* behaviorMgr);
        void Enable(void);
        void Disable(void);
        bool IsEnabled(void);
        void ProcessKeystroke(void);
        
        
      } // namespace KeyboardController
    } // namespace Sim 
  } // namespace Cozmo
} // namespace Anki


#endif //BASESTATION_KEYBOARD_CONTROLLER_H