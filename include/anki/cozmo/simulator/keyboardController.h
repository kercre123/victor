#ifndef KEYBOARD_CONTROLLER_H
#define KEYBOARD_CONTROLLER_H

namespace Anki {
  namespace Cozmo {
    
    // Being in the Simulator namespace gives us access to
    // Simulator::CozmoBot, a pointer to the webots supervisor
    // object.
    namespace Sim {
      
      namespace KeyboardController {
        

        void Enable(void);
        void Disable(void);
        bool IsEnabled(void);
        void ProcessKeystroke(void);
        
        
      } // namespace KeyboardController
      
    } // namespace Sim(ulator)
    
  } // namespace Cozmo
} // namespace Anki



#endif //KEYBOARD_CONTROLLER_H