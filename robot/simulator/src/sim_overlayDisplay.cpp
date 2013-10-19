#include "sim_overlayDisplay.h"

// Webots Includes
#include <webots/Display.hpp>
#include <webots/Supervisor.hpp>

namespace Anki {
  namespace Cozmo {
    
    namespace Sim {
      extern webots::Supervisor* CozmoBot;
      
      namespace OverlayDisplay {
        
        namespace { // "Private members"
          
          // For Webots Display:
          const f32 OVERLAY_TEXT_SIZE = 0.08;
          const u32 OVERLAY_TEXT_COLOR = 0xff0000;
          const u16 MAX_TEXT_DISPLAY_LENGTH = 1024;
          
          char displayText_[MAX_TEXT_DISPLAY_LENGTH];
        }
        
        void SetText(TextID ot_id, const char* formatStr, ...)
        {
          va_list argptr;
          va_start(argptr, formatStr);
          vsnprintf(displayText_, MAX_TEXT_DISPLAY_LENGTH, formatStr, argptr);
          va_end(argptr);
          
          CozmoBot->setLabel(ot_id, displayText_, 0.6f,
                             0.05f + static_cast<f32>(ot_id) * (OVERLAY_TEXT_SIZE/3.f),
                             OVERLAY_TEXT_SIZE, OVERLAY_TEXT_COLOR, 0);
        } // SetText()
        
      } // namespace OverlayDisplay
    } // namespace Sim
  } // namesapce Cozmo
} // namespace Anki


