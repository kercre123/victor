#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/localization.h"



#ifdef SIMULATOR
// Whether or not to use simulator "ground truth" pose
#define USE_SIM_GROUND_TRUTH_POSE 1
#define USE_OVERLAY_DISPLAY 1
#else // else not simulator
#define USE_SIM_GROUND_TRUTH_POSE 0
#define USE_OVERLAY_DISPLAY 0
#endif // #ifdef SIMULATOR


#if(USE_OVERLAY_DISPLAY)
#include "anki/cozmo/robot/hal.h"
#include "sim_overlayDisplay.h"
#endif

namespace Anki {
  namespace Cozmo {
    namespace Localization {
      
      namespace {
        // private members
        ::Anki::Embedded::Pose2d currMatPose;
        
        
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
      void Update()
      {

#if(USE_SIM_GROUND_TRUTH_POSE)
        // For initial testing only
        float angle;
        HAL::GetGroundTruthPose(currentMatX_,currentMatY_,angle);
        currentMatHeading_ = angle;
#else
        // TODO: Update current pose estimate
        // ...
       
#endif

#if(DEBUG_LOCALIZATION)
        PRINT("LOC: %f, %f, %f\n", currentMatX_, currentMatY_, currentMatHeading_.getDegrees());
#endif
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
          
          UpdateEstimatedPose(currentMatX_, currentMatY_, currentMatHeading_.ToFloat());
        }
#endif

      } // SetCurrentMatPose()
      
      void GetCurrentMatPose(f32& x, f32& y, Radians& angle)
      {
        x = currentMatX_;
        y = currentMatY_;
        angle = currentMatHeading_;
      } // GetCurrentMatPose()
      
  
      Radians GetCurrentMatOrientation()
      {
        return currentMatHeading_;
      }
      
    }
  }
}
