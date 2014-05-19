#include "anki/cozmo/robot/hal.h"
#include "localization.h"
#include "anki/common/robot/geometry.h"
#include "imuFilter.h"

#define DEBUG_LOCALIZATION 0

#ifdef SIMULATOR
// Whether or not to use simulator "ground truth" pose
#define USE_SIM_GROUND_TRUTH_POSE 0
#define USE_OVERLAY_DISPLAY 1
#else // else not simulator
#define USE_SIM_GROUND_TRUTH_POSE 0
#define USE_OVERLAY_DISPLAY 0
#endif // #ifdef SIMULATOR


// Whether or not to use the orientation given by the gyro
#define USE_GYRO_ORIENTATION 1


#if(USE_OVERLAY_DISPLAY)
#include "anki/cozmo/robot/hal.h"
#include "sim_overlayDisplay.h"
#endif

namespace Anki {
  namespace Cozmo {
    namespace Localization {
      
      namespace {
        
        const f32 BIG_RADIUS = 5000;
        
        // private members
        ::Anki::Embedded::Pose2d currMatPose;
        
        
        // Localization:
        f32 currentMatX_=0.f, currentMatY_=0.f;  // mm
        Radians currentMatHeading_(0.f);
       
#if(USE_OVERLAY_DISPLAY)
        f32 xTrue_, yTrue_, angleTrue_;
        f32 prev_xTrue_, prev_yTrue_, prev_angleTrue_;
#endif
        
        f32 prevLeftWheelPos_ = 0;
        f32 prevRightWheelPos_ = 0;
        
        f32 gyroRotOffset_ = 0;
        
        PoseFrameID_t frameId_ = 0;
        
      }

      Result Init() {
        SetCurrentMatPose(0,0,0);
        
        prevLeftWheelPos_ = HAL::MotorGetPosition(HAL::MOTOR_LEFT_WHEEL);
        prevRightWheelPos_ = HAL::MotorGetPosition(HAL::MOTOR_RIGHT_WHEEL);

        gyroRotOffset_ =  -IMUFilter::GetRotation();
        
        return RESULT_OK;
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
        
        // Convert to mm
        currentMatX_ *= 1000;
        currentMatY_ *= 1000;
        
        currentMatHeading_ = angle;
#else
     
        bool movement = false;
        
        // Update current pose estimate based on wheel motion
        
        f32 currLeftWheelPos = HAL::MotorGetPosition(HAL::MOTOR_LEFT_WHEEL);
        f32 currRightWheelPos = HAL::MotorGetPosition(HAL::MOTOR_RIGHT_WHEEL);
        
        // Compute distance traveled by each wheel
        f32 lDist = currLeftWheelPos - prevLeftWheelPos_;
        f32 rDist = currRightWheelPos - prevRightWheelPos_;

        
        // Compute new pose based on encoders and gyros, but only if there was any motion.
        movement = (!FLT_NEAR(rDist, 0) || !FLT_NEAR(lDist,0));
        if (movement ) {
#if(DEBUG_LOCALIZATION)
          PRINT("\ncurrWheelPos (%f, %f)   prevWheelPos (%f, %f)\n",
                currLeftWheelPos, currRightWheelPos, prevLeftWheelPos_, prevRightWheelPos_);
#endif
          
          f32 lRadius, rRadius; // Radii of each wheel arc path (+ve radius means origin of arc is to the left)
          f32 cRadius; // Radius of arc traversed by center of robot
          f32 cDist;   // Distance traversed by center of robot
          f32 cTheta;  // Theta traversed by center of robot
          
          
      
          // lDist / lRadius = rDist / rRadius = theta
          // rRadius - lRadius = wheel_dist  => rRadius = wheel_dist + lRadius
          
          // lDist / lRadius = rDist / (wheel_dist + lRadius)
          // (wheel_dist + lRadius) / lRadius = rDist / lDist
          // wheel_dist / lRadius = rDist / lDist - 1
          // lRadius = wheel_dist / (rDist / lDist - 1)

          if ((rDist != 0) && (lDist / rDist) < 1.01f && (lDist / rDist) > 0.99f) {
//          if (FLT_NEAR(lDist, rDist)) {
            lRadius = BIG_RADIUS;
            rRadius = BIG_RADIUS;
            cRadius = BIG_RADIUS;
            cTheta = 0;
            cDist = lDist;
          } else {
            if (FLT_NEAR(lDist,0)) {
              lRadius = 0;
            } else {
              lRadius = WHEEL_DIST_MM / (rDist / lDist - 1);
            }
            rRadius = WHEEL_DIST_MM + lRadius;
            if (ABS(rRadius) > ABS(lRadius)) {
              cTheta = rDist / rRadius;
            } else {
              cTheta = lDist / lRadius;
            }
            cDist = 0.5f*(lDist + rDist);
            cRadius = lRadius + WHEEL_DIST_HALF_MM;
          }

#if(DEBUG_LOCALIZATION)
          PRINT("lRadius %f, rRadius %f, lDist %f, rDist %f, cTheta %f, cDist %f, cRadius %f\n",
                lRadius, rRadius, lDist, rDist, cTheta, cDist, cRadius);
          
          PRINT("oldPose: %f %f %f\n", currentMatX_, currentMatY_, currentMatHeading_.ToFloat());
#endif
          
          if (ABS(cRadius) >= BIG_RADIUS) {
            currentMatX_ += cDist * cosf(currentMatHeading_.ToFloat());
            currentMatY_ += cDist * sinf(currentMatHeading_.ToFloat());
            
          } else {
            
#if(1)
            // Compute distance traveled relative to previous position.
            // Drawing a straight line from the previous position to the new position forms a chord
            // in the circle defined by the turning radius as determined by the incremental wheel motion this tick.
            // The angle of this circle that this chord spans is cTheta.
            // The angle of the chord relative to the robot's previous trajectory is cTheta / 2.
            f32 alpha = cTheta * 0.5f;
            
            // The chord length is 2 * cRadius * sin(cTheta / 2).
            f32 chord_length = ABS(2 * cRadius * sinf(alpha));
            
            // The new pose is then
            currentMatX_ += (cDist > 0 ? 1 : -1) * chord_length * cosf(currentMatHeading_.ToFloat() + alpha);
            currentMatY_ += (cDist > 0 ? 1 : -1) * chord_length * sinf(currentMatHeading_.ToFloat() + alpha);
            currentMatHeading_ += cTheta;
#else
            // Naive approximation, but seems to work nearly as well as non-naive with one less sin() call.
            currentMatX_ += cDist * cosf(currentMatHeading_.ToFloat());
            currentMatY_ += cDist * sinf(currentMatHeading_.ToFloat());
            currentMatHeading_ += cTheta;            
#endif
          }
          
#if(DEBUG_LOCALIZATION)
          PRINT("newPose: %f %f %f\n", currentMatX_, currentMatY_, currentMatHeading_.ToFloat());
#endif
       
        }

        
#if(USE_GYRO_ORIENTATION)
        // Set orientation according to gyro
        currentMatHeading_ = IMUFilter::GetRotation() + gyroRotOffset_;
#endif
        
        prevLeftWheelPos_ = HAL::MotorGetPosition(HAL::MOTOR_LEFT_WHEEL);
        prevRightWheelPos_ = HAL::MotorGetPosition(HAL::MOTOR_RIGHT_WHEEL);

        
#if(USE_OVERLAY_DISPLAY)
        if(movement)
        {
          using namespace Sim::OverlayDisplay;
          SetText(CURR_EST_POSE, "Est. Pose: (x,y)=(%.4f, %.4f) at deg=%.1f",
                  currentMatX_, currentMatY_,
                  currentMatHeading_.getDegrees());
          //PRINT("Est. Pose: (x,y)=(%.4f, %.4f) at deg=%.1f\n",
          //      currentMatX_, currentMatY_,
          //      currentMatHeading_.getDegrees());
          
          HAL::GetGroundTruthPose(xTrue_, yTrue_, angleTrue_);
          Radians angleRad(angleTrue_);
          
          
          SetText(CURR_TRUE_POSE, "True Pose: (x,y)=(%.4f, %.4f) at deg=%.1f",
                  xTrue_ * 1000, yTrue_ * 1000, angleRad.getDegrees());
          //f32 trueDist = sqrtf((yTrue_ - prev_yTrue_)*(yTrue_ - prev_yTrue_) + (xTrue_ - prev_xTrue_)*(xTrue_ - prev_xTrue_));
          //PRINT("True Pose: (x,y)=(%.4f, %.4f) at deg=%.1f (trueDist = %f)\n", xTrue_, yTrue_, angleRad.getDegrees(), trueDist);
          
          prev_xTrue_ = xTrue_;
          prev_yTrue_ = yTrue_;
          prev_angleTrue_ = angleTrue_;
          
          UpdateEstimatedPose(currentMatX_, currentMatY_, currentMatHeading_.ToFloat());
        }
#endif

        
        
#endif  //else (!USE_SIM_GROUND_TRUTH_POSE)
        

        
#if(DEBUG_LOCALIZATION)
        PRINT("LOC: %f, %f, %f\n", currentMatX_, currentMatY_, currentMatHeading_.getDegrees());
#endif
      }

      void SetCurrentMatPose(f32  x, f32  y, Radians  angle)
      {
        currentMatX_ = x;
        currentMatY_ = y;
        currentMatHeading_ = angle;
        gyroRotOffset_ = angle.ToFloat() - IMUFilter::GetRotation();
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

      
      void SetPoseFrameId(PoseFrameID_t id)
      {
        frameId_ = id;
      }

      PoseFrameID_t GetPoseFrameId()
      {
        return frameId_;
      }
      
    }
  }
}
