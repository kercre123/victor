#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/localization.h"




namespace Anki {
  namespace Cozmo {
    namespace Localization {
      
      namespace {
        // private members
        Anki::Embedded::Pose2d currMatPose;
        
        
        // Localization:
        f32 currentMatX_, currentMatY_;
        Radians currentMatHeading_;
      }

      void Init() {

      }
/*
      Anki::Embedded::Pose2d GetCurrMatPose()
      {
        return currMatPose;
      }
*/
      void Update() {
      }

      
      void SetCurrentMatPose(f32  x, f32  y, Radians  angle)
      {
        currentMatX_ = x;
        currentMatY_ = y;
        currentMatHeading_ = angle;
      }
      
      void GetCurrentMatPose(f32& x, f32& y, Radians& angle)
      {
        x = currentMatX_;
        y = currentMatY_;
        angle = currentMatHeading_;
      }
  
    }
  }
}
