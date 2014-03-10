#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "anki/common/shared/radians.h"
#include "anki/common/types.h"


namespace Anki {
  namespace Cozmo {
    namespace Localization {

      ReturnCode Init();
      
      //Embedded::Pose2d GetCurrMatPose();
      
      void GetCurrentMatPose(f32& x, f32& y, Radians& angle);
      void SetCurrentMatPose(f32  x, f32  y, Radians  angle);

      // Get orientation of robot on current mat
      Radians GetCurrentMatOrientation();
      
      void Update();

    } // Localization
  } // Cozmo
} // Anki
#endif

