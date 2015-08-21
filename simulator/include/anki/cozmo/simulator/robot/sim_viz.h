// General visualization interface to cozmo_physics.
// Any viz message that exists in VizMsgDefs.h is eligible to be sent from here.

#ifndef ANKI_SIM_VIZ_H
#define ANKI_SIM_VIZ_H

#include "anki/common/types.h"
#include "anki/planning/shared/path.h"

namespace Anki {
  namespace Cozmo { // TODO: Can PathFollower be made generic, put in coretech, and pulled out of Cozmo namespace?
    
    namespace Sim {

      namespace Viz {

        void Init();
        
        void ErasePath(s32 path_id);
        
        void AppendPathSegmentLine(s32 path_id,
                                   f32 x_start_mm, f32 y_start_mm,
                                   f32 x_end_mm, f32 y_end_mm);
        
        void AppendPathSegmentArc(s32 path_id,
                                  f32 x_center_mm, f32 y_center_mm,
                                  f32 radius_mm, f32 startRad, f32 sweepRad);
        
        void DrawPath(s32 path_id, const Planning::Path& p);
        
        void SetLabel(s32 label_id, const char* format, ...);
        
      } // namespace Viz
      
    } // namespace Sim
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_SIM_VIZ_H