#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "anki/cozmo/robot/cozmoTypes.h"

namespace Anki {
  namespace Cozmo {
    namespace Localization {

      void Init();
      
      //Embedded::Pose2d GetCurrMatPose();
      
      void GetCurrentMatPose(f32& x, f32& y, Radians& angle);
      void SetCurrentMatPose(f32  x, f32  y, Radians  angle);
      
      void Update();

    } // Localization
  } // Cozmo
} // Anki
#endif

