#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "anki/common/shared/radians.h"
#include "anki/common/types.h"


namespace Anki {
  namespace Cozmo {
    namespace Localization {

      Result Init();
      
      //Embedded::Pose2d GetCurrMatPose();
      
      void GetCurrentMatPose(f32& x, f32& y, Radians& angle);
      void SetCurrentMatPose(f32  x, f32  y, Radians  angle);

      // Get orientation of robot on current mat
      Radians GetCurrentMatOrientation();

      // Set/Get the current pose frame ID
      void SetPoseFrameId(PoseFrameID_t id);
      PoseFrameID_t GetPoseFrameId();
      
      
      void Update();

    } // Localization
  } // Cozmo
} // Anki
#endif

