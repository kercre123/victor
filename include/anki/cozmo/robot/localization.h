#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "anki/cozmo/robot/cozmoTypes.h"


// Real robot currently has no localization.
// Robot pose is always (0,0,0)
#ifdef SIMULATOR
// Set this to 1 if you want to simulate robot behavior without localization.
#define NO_LOCALIZATION 0
#else
#define NO_LOCALIZATION 1
#endif


namespace Anki {
  namespace Cozmo {
    namespace Localization {

      void Init();
      
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

