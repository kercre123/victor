#include "simulator/robot/sim_overlayDisplay.h"

#ifndef WEBOTS

namespace Anki {
  namespace Vector {
    
    namespace Sim {
      namespace OverlayDisplay {
        
        void Init(void)
        {
        }
        
        void SetText(TextID ot_id, const char* formatStr, ...)
        {
        } // SetText()
        
        void UpdateEstimatedPose(const f32 x, const f32 y, const f32 angle)
        {
        }

      } // namespace OverlayDisplay
    } // namespace Sim
  } // namespace Vector
} // namespace Anki

#endif // ndef WEBOTS
