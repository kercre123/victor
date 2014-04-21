#include "anki/common/robot/trig_fast.h"
#include "imuFilter.h"
#include "headController.h"
#include "anki/cozmo/robot/hal.h"

// For event callbacks
#include "testModeController.h"
#include "anki/cozmo/robot/cozmoBot.h"

#define DEBUG_IMU_FILTER 0


namespace Anki {
  namespace Cozmo {
    namespace IMUFilter {
      
      namespace {
        
        // Orientation and speed in XY-plane (i.e. horizontal plane) of robot
        f32 rot_ = 0;   // radians
        f32 rotSpeed_ = 0; // rad/s
        
        // Angle of accelerometer wrt gravity horizontal
        f32 pitch_ = 0;
        
        f32 gyro_filt[3];
        f32 gyro_robot_frame_filt[3];
        const f32 RATE_FILT_COEFF = 0.9f;

        f32 accel_filt[3];
        f32 accel_robot_frame_filt[3];
        const f32 ACCEL_FILT_COEFF = 0.4f;
        
        
        // ====== Event detection vars ======
        enum IMUEvent {
          FALLING = 0,
          UPSIDE_DOWN,
          LEFTSIDE_DOWN,
          RIGHTSIDE_DOWN,
          BACKSIDE_DOWN,
          NUM_IMU_EVENTS
        };
        
        // Stores whether or not IMU conditions for an event are met this cycle
        bool eventStateRaw_[NUM_IMU_EVENTS];
        
        // Stores whether or not IMU and timing conditions for an event are met.
        // This is the true indicator of whether or not an event has occured.
        bool eventState_[NUM_IMU_EVENTS];
        
        // Used to store time that event was first activated or deactivated
        u32 eventTime_[NUM_IMU_EVENTS] = {0};
        
        // The amount of time required for eventStateRaw to be true for eventState to be true
        const u32 eventActivationTime_us_[NUM_IMU_EVENTS] = {500000, 500000, 500000, 500000, 500000};
        
        // The amount of time required for eventStateRaw to be false for eventState to be false
        const u32 eventDeactivationTime_us_[NUM_IMU_EVENTS] = {500000, 500000, 500000, 500000, 500000};
        
        // Callback functions associated with event activation and deactivation
        typedef void (*eventCallbackFn)(void);
        eventCallbackFn eventActivationCallbacks[NUM_IMU_EVENTS] = {0};
        eventCallbackFn eventDeactivationCallbacks[NUM_IMU_EVENTS] = {0};
        
        // Falling
        const f32 FALLING_THRESH_MMPS2_SQRD = 4000000;
        
        // N-side down
        const f32 NSIDE_DOWN_THRESH_MMPS2 = 8000;

        // LED assignments
        const HAL::LEDId INDICATOR_LED_ID = HAL::LED_LEFT_EYE_TOP;
        const HAL::LEDId HEADLIGHT_LED_ID = HAL::LED_RIGHT_EYE_TOP;
        
      } // "private" namespace
      
      
      // ==== Event callback functions ===
      void ToggleHeadLights() {
        static bool lightsOn = false;
        if (lightsOn) {
          HAL::SetLED(HEADLIGHT_LED_ID, HAL::LED_OFF);
          lightsOn = false;
        } else {
          HAL::SetLED(HEADLIGHT_LED_ID, HAL::LED_RED);
          lightsOn = true;
        }
      }
      
      void TurnOnIndicatorLight()
      {
        TestModeController::Init(TestModeController::TM_NONE);
        HAL::SetLED(INDICATOR_LED_ID, HAL::LED_RED);
        Robot::StopRobot();
      }
      
      void StartPickAndPlaceTest()
      {
        TestModeController::Init(TestModeController::TM_PICK_AND_PLACE);
        HAL::SetLED(INDICATOR_LED_ID, HAL::LED_OFF);
      }
      
      void StartPathFollowTest()
      {
        TestModeController::Init(TestModeController::TM_PATH_FOLLOW);
        HAL::SetLED(INDICATOR_LED_ID, HAL::LED_OFF);
      }
      
      //===== End of event callbacks ====
      
      
      void Reset()
      {
        rot_ = 0;
        rotSpeed_ = 0;
        // Event callback functions
        // TODO: This should probably go somewhere else
        eventActivationCallbacks[UPSIDE_DOWN] = ToggleHeadLights;
        eventActivationCallbacks[RIGHTSIDE_DOWN] = TurnOnIndicatorLight;
        eventDeactivationCallbacks[RIGHTSIDE_DOWN] = StartPickAndPlaceTest;
        eventActivationCallbacks[LEFTSIDE_DOWN] = TurnOnIndicatorLight;
        eventDeactivationCallbacks[LEFTSIDE_DOWN] = StartPathFollowTest;
        eventActivationCallbacks[BACKSIDE_DOWN] = TurnOnIndicatorLight;
      }
      
      
      void DetectFalling()
      {
        const f32 accelMagnitudeSqrd = accel_filt[0]*accel_filt[0] +
                                       accel_filt[1]*accel_filt[1] +
                                       accel_filt[2]*accel_filt[2];
        
        eventStateRaw_[FALLING] = accelMagnitudeSqrd < FALLING_THRESH_MMPS2_SQRD;
      }
      
      void DetectNsideDown()
      {
        eventStateRaw_[UPSIDE_DOWN] = accel_robot_frame_filt[2] < -NSIDE_DOWN_THRESH_MMPS2;
        eventStateRaw_[RIGHTSIDE_DOWN] = accel_robot_frame_filt[1] < -NSIDE_DOWN_THRESH_MMPS2;
        eventStateRaw_[LEFTSIDE_DOWN] = accel_robot_frame_filt[1] > NSIDE_DOWN_THRESH_MMPS2;
        eventStateRaw_[BACKSIDE_DOWN] = accel_robot_frame_filt[0] > NSIDE_DOWN_THRESH_MMPS2;
      }
      
      
      
      void UpdateEventDetection()
      {
        // Call detect functions and update raw event state
        DetectFalling();
        DetectNsideDown();
      
        
        // Now update event state according to (de)activation time
        u32 currTime = HAL::GetMicroCounter();
        for (int e = FALLING; e < NUM_IMU_EVENTS; ++e) {
         
#if(DEBUG_IMU_FILTER)
          //PERIODIC_PRINT(40,"IMUFilter event %d: state %d, rawState %d, eventTime %d, currTime %d\n",
          //               e, eventState_[e], eventStateRaw_[e], eventTime_[e], currTime);
#endif
          
          if (!eventState_[e]) {
            if (eventStateRaw_[e]) {
              if (eventTime_[e] == 0) {
                // Raw event state conditions met for the first time
                eventTime_[e] = currTime;
              } else if (currTime - eventTime_[e] > eventActivationTime_us_[e]) {
                // Event activated
                eventState_[e] = true;
                
                // Call activation function
                if (eventActivationCallbacks[e]) {
#if(DEBUG_IMU_FILTER)
                  PRINT("IMUFilter: Activation callback %d\n", e);
#endif
                  eventActivationCallbacks[e]();
                }
              }
            } else {
              eventTime_[e] = 0;
            }
            
          } else {
            
            if (!eventStateRaw_[e]){
              if (eventTime_[e] == 0) {
                eventTime_[e] = currTime;
              } else if (currTime - eventTime_[e] > eventDeactivationTime_us_[e]) {
                // Event deactivated
                eventState_[e] = false;
                
                // Call deactivation function
                if (eventDeactivationCallbacks[e]) {
#if(DEBUG_IMU_FILTER)
                  PRINT("IMUFilter: Deactivation callback %d\n", e);
#endif
                  eventDeactivationCallbacks[e]();
                }
              }
            } else {
              eventTime_[e] = 0;
            }
          }
        }
        
      }
      
      Result Update()
      {
        Result retVal = RESULT_OK;
        
        // Get IMU data
        HAL::IMU_DataStructure imu_data;
        HAL::IMUReadData(imu_data);
        
        
        ////// Gyro Update //////
        
        // Filter rotation speeds
        // TODO: Do this in hardware?
        gyro_filt[0] = imu_data.rate_x * RATE_FILT_COEFF + gyro_filt[0] * (1.f-RATE_FILT_COEFF);
        gyro_filt[1] = imu_data.rate_y * RATE_FILT_COEFF + gyro_filt[1] * (1.f-RATE_FILT_COEFF);
        gyro_filt[2] = imu_data.rate_z * RATE_FILT_COEFF + gyro_filt[2] * (1.f-RATE_FILT_COEFF);
      
        
        // Compute head angle wrt to world horizontal plane
        const f32 headAngle = HeadController::GetAngleRad();  // TODO: Use encoders or accelerometer data? If encoders,
                                                        // may need to use accelerometer data anyway for when it's on ramps.
        

        // Compute rotation speeds in robot XY-plane.
        // https://www.chrobotics.com/library/understanding-euler-angles
        // http://ocw.mit.edu/courses/mechanical-engineering/2-017j-design-of-electromechanical-robotic-systems-fall-2009/course-text/MIT2_017JF09_ch09.pdf
        //
        // r: roll angle (x-axis), p: pitch angle (y-axis), y: yaw angle (z-axis)
        //
        //            |  1    sin(r)*tan(p)    cos(r)*tan(p)  |
        // D(r,p,y) = |  0       cos(r)           -sin(r)     |
        //            |  0    sin(r)/cos(p)    cos(r)/cos(p)  |
        //
        // Rotation in robot frame = D * [dr/dt, dp/dt, dy,dt] where the latter vector is given by gyro readings.
        // In our case, we only care about yaw. In other words, it's always true that r = y = 0.
        // (NOTE: This is true as long as we don't start turning on ramps!!!)
        // So the result simplifies to...
        gyro_robot_frame_filt[0] = gyro_filt[0] + gyro_filt[2] * tanf(headAngle);
        gyro_robot_frame_filt[1] = gyro_filt[1];
        gyro_robot_frame_filt[2] = gyro_filt[2] / cosf(headAngle);
        // TODO: We actually only care about gyro_robot_frame_filt[2]. Any point in computing the others?
        
        
        // XY-plane rotation rate is robot frame z-axis rotation rate
        rotSpeed_ = gyro_robot_frame_filt[2];
        
        
        // Update orientation
        f32 dAngle = rotSpeed_ * CONTROL_DT;
        rot_ += dAngle;
        
        
        
        ///// Accelerometer update /////
        accel_filt[0] = imu_data.acc_x * ACCEL_FILT_COEFF + accel_filt[0] * (1.f-ACCEL_FILT_COEFF);
        accel_filt[1] = imu_data.acc_y * ACCEL_FILT_COEFF + accel_filt[1] * (1.f-ACCEL_FILT_COEFF);
        accel_filt[2] = imu_data.acc_z * ACCEL_FILT_COEFF + accel_filt[2] * (1.f-ACCEL_FILT_COEFF);
        
        pitch_ = atan2(accel_filt[0], accel_filt[2]);
        
        //PERIODIC_PRINT(50, "Pitch %f\n", RAD_TO_DEG_F32(pitch_));

        // Compute accelerations in robot frame
        const f32 xzAccelMagnitude = sqrtf(accel_filt[0]*accel_filt[0] + accel_filt[2]*accel_filt[2]);
        const f32 accel_angle_imu_frame = atan2_fast(accel_filt[2], accel_filt[0]);
        const f32 accel_angle_robot_frame = accel_angle_imu_frame + headAngle;
        
        
        accel_robot_frame_filt[0] = xzAccelMagnitude * cosf(accel_angle_robot_frame);
        accel_robot_frame_filt[1] = accel_filt[1];
        accel_robot_frame_filt[2] = xzAccelMagnitude * sinf(accel_angle_robot_frame);
        
#if(DEBUG_IMU_FILTER)
        PERIODIC_PRINT(200, "Accel angle %f %f\n", accel_angle_imu_frame, accel_angle_robot_frame);
        PERIODIC_PRINT(200, "Accel (robot frame): %f %f %f\n",
                       accel_robot_frame_filt[0],
                       accel_robot_frame_filt[1],
                       accel_robot_frame_filt[2]);
#endif
        
        
        UpdateEventDetection();
        
        return retVal;
        
      } // Update()
      
      
      f32 GetRotation()
      {
        return rot_;
      }
      
      f32 GetRotationSpeed()
      {
        return rotSpeed_;
      }
      
      f32 GetPitch()
      {
        return pitch_;
      }
      

    } // namespace IMUFilter
  } // namespace Cozmo
} // namespace Anki
