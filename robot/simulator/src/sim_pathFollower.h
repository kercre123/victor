#ifndef ANKI_SIM_PATH_FOLLOWER_H
#define ANKI_SIM_PATH_FOLLOWER_H

#include "anki/common/types.h"

namespace Anki {
  namespace Cozmo { // TODO: Can PathFollower be made generic, put in coretech, and pulled out of Cozmo namespace?
    
    namespace PathFollower {
      
      // Augment existing PathFollower namespace with visualsization methods
			namespace Viz {
        
        ReturnCode Init();
        
				void ErasePath(s32 path_id);
        
				void AppendPathSegmentLine(s32 path_id,
																	 f32 x_start_mm, f32 y_start_mm,
																	 f32 x_end_mm, f32 y_end_mm);
        
				void AppendPathSegmentArc(s32 path_id,
																	f32 x_center_mm, f32 y_center_mm,
																	f32 radius_mm, f32 startRad, f32 sweepRad);
        
				void ShowPath(s32 path_id, bool show);
        
				void SetPathHeightOffset(f32 m);
        
        
      } // namespace Viz
      
    } // namespace PathFollower
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_SIM_PATH_FOLLOWER_H