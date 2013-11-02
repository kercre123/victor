#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/localization.h"




namespace Anki {
  namespace Cozmo {
    namespace Localization {
      
      namespace {
        // private members
        Anki::Embedded::Pose2d currMatPose;
        
        
        // Localization:
        f32 currentMatX_=0.f, currentMatY_=0.f;
        Radians currentMatHeading_(0.f);
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
        
#if(USE_OVERLAY_DISPLAY)
        {
          using namespace Sim::OverlayDisplay;
          SetText(CURR_EST_POSE, "Est. Pose: (x,y)=(%.4f, %.4f) at angle=%.1f",
                  currentMatX_, currentMatY_,
                  currentMatHeading_.getDegrees());
          f32 xTrue, yTrue, angleTrue;
          HAL::GetGroundTruthPose(xTrue, yTrue, angleTrue);
          Radians angleRad(angleTrue);
          
          SetText(CURR_TRUE_POSE, "True Pose: (x,y)=(%.4f, %.4f) at angle=%.1f",
                  xTrue, yTrue, angleRad.getDegrees());
        }
#endif

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
