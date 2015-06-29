/**
 * File: imuFilter.cpp
 *
 * Author: Kevin Yoon
 * Created: 4/1/2014
 *
 * Description:
 *
 *   Filter for gyro and accelerometer
 *   Orientation of gyro axes is assumed to be identical to that of robot when the head is at 0 degrees.
 *   i.e. x-axis points forward, y-axis points to robot's left, z-axis points up.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "anki/common/robot/trig_fast.h"
#include "imuFilter.h"
#include "headController.h"
#include "liftController.h"
#include "wheelController.h"
#include "anki/cozmo/robot/hal.h"

// For event callbacks
#include "testModeController.h"


#define DEBUG_IMU_FILTER 0


namespace Anki {
  namespace Cozmo {
    namespace IMUFilter {
      
      namespace {
        // Last read IMU data
        HAL::IMU_DataStructure imu_data_;
        
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
        f32 prev_accel_robot_frame_filt[3] = {0,0,0};
        const f32 ACCEL_FILT_COEFF = 0.1f;
        
        // ==== Pickup detection ===
        bool pickedUp_ = false;
        
        // Filter coefficient on z-axis accelerometer
        const f32 ACCEL_PICKUP_FILT_COEFF = 0.1f;
        
        // Minimum delta in filtered data required to be considering trending up/down
        const f32 PD_ACCEL_MIN_DELTA = 5.f;
        
        // Minimum length of trend (in tics) required for pickup detection.
        const u32 PD_MIN_TREND_LENGTH = 20;
        
        // A trend that spans this difference in accelerometer readings (mm/s2) can be considered
        // a pickup even if the head was moving.
        const f32 PD_SUFFICIENT_TREND_DIFF = 4200;

        f32 pdFiltAccX_aligned_ = 0;
        f32 pdFiltAccY_aligned_ = 0;
        f32 pdFiltAccZ_aligned_ = 0;
        f32 pdFiltAccX_ = 0.f;
        f32 pdFiltAccY_ = 0.f;
        f32 pdFiltAccZ_ = 9800.f;
        f32 pdTrendStartVal_ = 0;
        u8 pdRiseCnt_ = 0;
        u8 pdFallCnt_ = 0;
        u8 pdLastHeadMoveCnt_ = 0;
        u8 pdUnexpectedMotionCnt_ = 0;
        // === End of Pickup detection ===

        
        u32 lastMotionDetectedTime_us = 0;
        const u32 MOTION_DETECT_TIMEOUT_US = 500000;
        const f32 ACCEL_MOTION_THRESHOLD = 60.f;
        const f32 GYRO_MOTION_THRESHOLD = 0.24f;
        
        
        // Recorded buffer
        bool isRecording_ = false;
        u8 recordDataIdx_ = 0;
        Messages::IMUDataChunk imuChunkMsg_;

        
        // ====== Event detection vars ======
        enum IMUEvent {
          FALLING = 0,
          UPSIDE_DOWN,
          LEFTSIDE_DOWN,
          RIGHTSIDE_DOWN,
          FRONTSIDE_DOWN,
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
        const LEDId INDICATOR_LED_ID = LED_BACKPACK_LEFT;
        const LEDId HEADLIGHT_LED_ID = LED_BACKPACK_RIGHT;
        
      } // "private" namespace
      
      
      // ==== Event callback functions ===
      void ToggleHeadLights() {
        static bool lightsOn = false;
        if (lightsOn) {
          HAL::SetLED(HEADLIGHT_LED_ID, LED_OFF);
          lightsOn = false;
        } else {
          HAL::SetLED(HEADLIGHT_LED_ID, LED_RED);
          lightsOn = true;
        }
      }
      
      void TurnOnIndicatorLight()
      {
        TestModeController::Start(TM_NONE);
        HAL::SetLED(INDICATOR_LED_ID, LED_RED);
      }
      void TurnOffIndicatorLight()
      {
        HAL::SetLED(INDICATOR_LED_ID, LED_OFF);
      }
      
      void StartPickAndPlaceTest()
      {
        TestModeController::Start(TM_PICK_AND_PLACE);
        TurnOffIndicatorLight();
      }
      
      void StartPathFollowTest()
      {
        TestModeController::Start(TM_PATH_FOLLOW);
        TurnOffIndicatorLight();
      }
      
      void StartLiftTest()
      {
        TestModeController::Start(TM_LIFT);
        TurnOffIndicatorLight();
      }
      
      //===== End of event callbacks ====
      void SetPickupDetect(bool pickupDetected)
      {
        // TEST WITH LIGHT
        if (pickupDetected) {
          HAL::SetLED(INDICATOR_LED_ID, LED_BLUE);
        } else {
          HAL::SetLED(INDICATOR_LED_ID, LED_OFF);
        }
        
        pickedUp_ = pickupDetected;
        pdFallCnt_ = 0;
        pdRiseCnt_ = 0;
        pdLastHeadMoveCnt_ = 0;
      }
      
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
        eventActivationCallbacks[FRONTSIDE_DOWN] = TurnOnIndicatorLight;
        eventDeactivationCallbacks[FRONTSIDE_DOWN] = StartLiftTest;
        eventActivationCallbacks[BACKSIDE_DOWN] = TurnOnIndicatorLight;
        eventDeactivationCallbacks[BACKSIDE_DOWN] = TurnOffIndicatorLight;
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
        eventStateRaw_[LEFTSIDE_DOWN] = accel_robot_frame_filt[1] < -NSIDE_DOWN_THRESH_MMPS2;
        eventStateRaw_[RIGHTSIDE_DOWN] = accel_robot_frame_filt[1] > NSIDE_DOWN_THRESH_MMPS2;
        eventStateRaw_[FRONTSIDE_DOWN] = accel_robot_frame_filt[0] < -NSIDE_DOWN_THRESH_MMPS2;
        eventStateRaw_[BACKSIDE_DOWN] = accel_robot_frame_filt[0] > NSIDE_DOWN_THRESH_MMPS2;
      }
      
      
      bool MotionDetected() {
        return (lastMotionDetectedTime_us + MOTION_DETECT_TIMEOUT_US) > HAL::GetMicroCounter();
      }
      
      // Robot pickup detector
      //
      // Pickup detection occurs when the z-axis accelerometer reading is detected to be trending
      // up or down. When the robot moves under it's own power the accelerometer readings tend to be
      // much more noisy than when it is held by a person. The trend must satisfy one of two cases to
      // be considered a pickup detection:
      //
      // 1) Be trending for at least PD_MIN_TREND_LENGTH tics without any head motion.
      //    (Head motion is sometimes smooth enough to look like a pickup.)
      //
      // 2) Be trending for at least PD_MIN_TREND_LENGTH and have spanned a delta of
      //    PD_SUFFICIENT_TREND_DIFF mm/s^2. In this case head motion is allowed.
      //    This is so we can at least have a less sensitive detector if the robot
      //    is engaged is some never-ending head motions.
      void DetectPickup()
      {
        
        // Compute the acceleration componenet aligned with the z-axis of the robot
        const f32 xzAccelMagnitude = sqrtf(pdFiltAccX_*pdFiltAccX_ + pdFiltAccZ_*pdFiltAccZ_);
        const f32 accel_angle_imu_frame = atan2_fast(pdFiltAccZ_, pdFiltAccX_);
        const f32 accel_angle_robot_frame = accel_angle_imu_frame + HeadController::GetAngleRad();
        
        f32 pdFiltPrevVal = pdFiltAccZ_aligned_;
        
        pdFiltAccX_aligned_ = xzAccelMagnitude * cosf(accel_angle_robot_frame);
        pdFiltAccY_aligned_ = pdFiltAccY_;
        pdFiltAccZ_aligned_ = xzAccelMagnitude * sinf(accel_angle_robot_frame);

        
        
        if (IsPickedUp()) {
          
          // Picked up flag is reset only when the robot has stopped moving
          // and it has been set upright.
          if (!MotionDetected() && accel_robot_frame_filt[2] > NSIDE_DOWN_THRESH_MMPS2) {
            SetPickupDetect(false);
          }
            
        } else {

          // Do simple check first.
          // If wheels aren't moving, any motion is because a person was messing with it!
          if (!WheelController::AreWheelsPowered() && !HeadController::IsMoving() && !LiftController::IsMoving()) {
            if (ABS(pdFiltAccX_aligned_) > 5000 ||
                ABS(pdFiltAccY_aligned_) > 3000 ||
                ABS(pdFiltAccZ_aligned_) > 11000 ||
                ABS(gyro_robot_frame_filt[0]) > 5.f ||
                ABS(gyro_robot_frame_filt[1]) > 5.f ||
                ABS(gyro_robot_frame_filt[2]) > 5.f) {
              if (++pdUnexpectedMotionCnt_ > 40) {
                PRINT("PDWhileStationary: acc (%f, %f, %f), gyro (%f, %f, %f)\n",
                      pdFiltAccX_aligned_, pdFiltAccY_aligned_, pdFiltAccZ_aligned_,
                      gyro_robot_frame_filt[0], gyro_robot_frame_filt[1], gyro_robot_frame_filt[2]);
                SetPickupDetect(true);
              }
            }
          } else {
            pdUnexpectedMotionCnt_ = 0;
          }

          
          // Do conservative check for pickup.
          // Only when we're really sure it's moving!
          // TODO: Make this smarter!
          if (!IsPickedUp() &&
              (ABS(pdFiltAccX_aligned_) > 5000 ||
               ABS(pdFiltAccY_aligned_) > 3000 ||
               ABS(pdFiltAccZ_aligned_) > 11000)) {
            ++pdRiseCnt_;
            if (pdRiseCnt_ > 40) {
              SetPickupDetect(true);
              PRINT("PickupDetect: accX = %f, accY = %f, accZ = %f\n",
                    pdFiltAccX_aligned_, pdFiltAccY_aligned_, pdFiltAccZ_aligned_);
            }
          } else {
            pdRiseCnt_ = 0;
          }
          
          /*
          // If rise detected this tic...
          if (pdFiltAccZ_aligned_ > pdFiltPrevVal + PD_ACCEL_MIN_DELTA) {
            if (pdFallCnt_ > 0) {
              // If we've been trending down, check if the trend
              // meets pickup detection conditions. Otherwise,
              // reset vars for upwards trend.
              if ((pdFallCnt_ - pdLastHeadMoveCnt_ > PD_MIN_TREND_LENGTH)
                  || (pdFallCnt_ > PD_MIN_TREND_LENGTH && (pdTrendStartVal_ - pdFiltAccZ_aligned_) > PD_SUFFICIENT_TREND_DIFF)) {
                PRINT("PDFall: %d, lastHead: %d, val %f, startVal %f\n", pdFallCnt_, pdLastHeadMoveCnt_, pdFiltAccZ_, pdTrendStartVal_);
                SetPickupDetect(true);
              } else {
                pdFallCnt_ = 0;
                pdLastHeadMoveCnt_ = 0;
                pdTrendStartVal_ = pdFiltAccZ_aligned_;
                ++pdRiseCnt_;
              }
            } else {
              ++pdRiseCnt_;

              if (pdRiseCnt_ == 1) {
                pdTrendStartVal_ = pdFiltAccZ_aligned_;
              }
              
              if (HeadController::IsMoving()) {
                pdLastHeadMoveCnt_ = pdRiseCnt_;
              }
              
            }
            
          // If fall detected this tic...
          } else if (pdFiltAccZ_aligned_ < pdFiltPrevVal - PD_ACCEL_MIN_DELTA) {
            if (pdRiseCnt_ > 0) {
              // If we've been trending up, check if the trend
              // meets pickup detection conditions. Otherwise,
              // reset vars for downwards trend.
              if ((pdRiseCnt_ - pdLastHeadMoveCnt_> PD_MIN_TREND_LENGTH)
                  || (pdRiseCnt_ > PD_MIN_TREND_LENGTH && (pdFiltAccZ_aligned_ - pdTrendStartVal_) > PD_SUFFICIENT_TREND_DIFF)) {
                PRINT("PDRise: %d, lastHead: %d, val %f, startVal %f\n", pdRiseCnt_, pdLastHeadMoveCnt_, pdFiltAccZ_, pdTrendStartVal_);
                SetPickupDetect(true);
              } else {
                pdRiseCnt_ = 0;
                pdLastHeadMoveCnt_ = 0;
                ++pdFallCnt_;
                pdTrendStartVal_ = pdFiltAccZ_aligned_;
              }
            } else {
              ++pdFallCnt_;
              
              if (pdFallCnt_ == 1) {
                pdTrendStartVal_ = pdFiltAccZ_aligned_;
              }
              
              if (HeadController::IsMoving()) {
                pdLastHeadMoveCnt_ = pdFallCnt_;
              }

            }
          } else {
            
            // Trend is flat. Decrement counter values.
            if (pdRiseCnt_ > 0) {
              --pdRiseCnt_;
            }
            if (pdFallCnt_ > 0) {
              --pdFallCnt_;
            }
            if (pdLastHeadMoveCnt_ > 0) {
              --pdLastHeadMoveCnt_;
            }
            
          }
           */
        }
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
      
      // Update the last time motion was detected
      void DetectMotion()
      {
        u32 currTime = HAL::GetMicroCounter();
        
        // Are wheels being powered?
        if (WheelController::AreWheelsPowered()) {
          lastMotionDetectedTime_us = currTime;
        }
        
        // Was motion detected by accel or gyro?
        for(u8 i=0; i<3; ++i) {
          // Check accelerometer
          f32 dAccel = ABS(accel_robot_frame_filt[i] - prev_accel_robot_frame_filt[i]);
          prev_accel_robot_frame_filt[i] = accel_robot_frame_filt[i];
          if (dAccel > ACCEL_MOTION_THRESHOLD) {
            lastMotionDetectedTime_us = currTime;
          }
          
          // Check gyro
          if (ABS(gyro_robot_frame_filt[i]) > GYRO_MOTION_THRESHOLD) {
            lastMotionDetectedTime_us = currTime;
          }
        }
      }
      
      Result Update()
      {
        Result retVal = RESULT_OK;
        
        // Don't do IMU updates until head is calibrated
        if (!HeadController::IsCalibrated()) {
          return retVal;
        }
        
        
        // Get IMU data
        HAL::IMUReadData(imu_data_);
        
        
        ////// Gyro Update //////
        
        // Filter rotation speeds
        // TODO: Do this in hardware?
        gyro_filt[0] = imu_data_.rate_x * RATE_FILT_COEFF + gyro_filt[0] * (1.f-RATE_FILT_COEFF);
        gyro_filt[1] = imu_data_.rate_y * RATE_FILT_COEFF + gyro_filt[1] * (1.f-RATE_FILT_COEFF);
        gyro_filt[2] = imu_data_.rate_z * RATE_FILT_COEFF + gyro_filt[2] * (1.f-RATE_FILT_COEFF);
      
        
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
        
        
        
        
        
        
        
        ///// Accelerometer update /////
        accel_filt[0] = imu_data_.acc_x * ACCEL_FILT_COEFF + accel_filt[0] * (1.f-ACCEL_FILT_COEFF);
        accel_filt[1] = imu_data_.acc_y * ACCEL_FILT_COEFF + accel_filt[1] * (1.f-ACCEL_FILT_COEFF);
        accel_filt[2] = imu_data_.acc_z * ACCEL_FILT_COEFF + accel_filt[2] * (1.f-ACCEL_FILT_COEFF);
        //printf("accel: %f %f %f\n", accel_filt[0], accel_filt[1], accel_filt[2]);
        pitch_ = atan2(accel_filt[0], accel_filt[2]) - headAngle;
        
        //PERIODIC_PRINT(50, "Pitch %f\n", RAD_TO_DEG_F32(pitch_));

        // Compute accelerations in robot frame
        const f32 xzAccelMagnitude = sqrtf(accel_filt[0]*accel_filt[0] + accel_filt[2]*accel_filt[2]);
        const f32 accel_angle_imu_frame = atan2_fast(accel_filt[2], accel_filt[0]);
        const f32 accel_angle_robot_frame = accel_angle_imu_frame + headAngle;
        
        prev_accel_robot_frame_filt[0] = accel_robot_frame_filt[0];
        prev_accel_robot_frame_filt[1] = accel_robot_frame_filt[1];
        prev_accel_robot_frame_filt[2] = accel_robot_frame_filt[2];
        
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


#if(0)
        // Measure peak readings every 2 seconds
        static f32 max_gyro[3] = {0,0,0};
        static f32 max_accel[3] = {0,0,0};
        for (int i=0; i<3; ++i) {
          if(ABS(gyro_robot_frame_filt[i]) > max_gyro[i]) {
            max_gyro[i] = ABS(gyro_robot_frame_filt[i]);
          }
          
          if (prev_accel_robot_frame_filt[i] != 0) {
            f32 dAccel = ABS(accel_robot_frame_filt[i] - prev_accel_robot_frame_filt[i]);
            if(dAccel > max_accel[i]) {
              max_accel[i] = dAccel;
            }
          }
        }
        
        static u32 measurement_cycles = 0;
        if (measurement_cycles++ == 400) {
          PRINT("Max gyro: %f %f %f\n",
                         max_gyro[0],
                         max_gyro[1],
                         max_gyro[2]);
          PRINT("Max accel_delta: %f %f %f\n",
                         max_accel[0],
                         max_accel[1],
                         max_accel[2]);
          
            measurement_cycles = 0;
          for (int i=0; i<3; ++i) {
            max_accel[i] = 0;
            max_gyro[i] = 0;
          }
        }
#endif
        
        DetectMotion();
        
        // XY-plane rotation rate is robot frame z-axis rotation rate
        rotSpeed_ = gyro_robot_frame_filt[2];
        
        // Update orientation if motion detected or expected
        if (MotionDetected()) {
          f32 dAngle = rotSpeed_ * CONTROL_DT;
          rot_ += dAngle;
        }
        
        // XXX: DEBUG!
        //UpdateEventDetection();
        
        // Pickup detection
        pdFiltAccX_ = imu_data_.acc_x * ACCEL_PICKUP_FILT_COEFF + pdFiltAccX_ * (1.f - ACCEL_PICKUP_FILT_COEFF);
        pdFiltAccY_ = imu_data_.acc_y * ACCEL_PICKUP_FILT_COEFF + pdFiltAccY_ * (1.f - ACCEL_PICKUP_FILT_COEFF);
        pdFiltAccZ_ = imu_data_.acc_z * ACCEL_PICKUP_FILT_COEFF + pdFiltAccZ_ * (1.f - ACCEL_PICKUP_FILT_COEFF);
        
        // XXX: Commenting this out because pickup detection seems to be firing
        //      when the robot drives up ramp (or the side of a platform) and
        //      clearing pose history.
        DetectPickup();
        
        //UpdateEventDetection();
        
        
        // Recording IMU data for sending to basestation
        if (isRecording_) {
          
          // Scale accel range of -20000 to +20000mm/s2 (roughly -2g to +2g) to be between -127 and +127
          const f32 accScaleFactor = 127.f/20000.f;
          /*
          imuChunkMsg_.aX[recordDataIdx_] = (accel_robot_frame_filt[0] * accScaleFactor);
          imuChunkMsg_.aY[recordDataIdx_] = (accel_robot_frame_filt[1] * accScaleFactor);
          imuChunkMsg_.aZ[recordDataIdx_] = (accel_robot_frame_filt[2] * accScaleFactor);
           */
          imuChunkMsg_.aX[recordDataIdx_] = (pdFiltAccX_aligned_ * accScaleFactor);
          imuChunkMsg_.aY[recordDataIdx_] = (pdFiltAccY_aligned_ * accScaleFactor);
          imuChunkMsg_.aZ[recordDataIdx_] = (pdFiltAccZ_aligned_ * accScaleFactor);
          
          // Scale gyro range of -2pi to +2pi rad/s to be between -127 and +127
          const f32 gyroScaleFactor = 127.f/(2.f*PI_F);
          imuChunkMsg_.gX[recordDataIdx_] = (gyro_robot_frame_filt[0] * gyroScaleFactor);
          imuChunkMsg_.gY[recordDataIdx_] = (gyro_robot_frame_filt[1] * gyroScaleFactor);
          imuChunkMsg_.gZ[recordDataIdx_] = (gyro_robot_frame_filt[2] * gyroScaleFactor);

          // Send message when it's full
          if (++recordDataIdx_ == IMU_CHUNK_SIZE) {
            HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::IMUDataChunk), &imuChunkMsg_);
            recordDataIdx_ = 0;
            ++imuChunkMsg_.chunkId;
            
            if (imuChunkMsg_.chunkId == imuChunkMsg_.totalNumChunks) {
              PRINT("IMU RECORDING COMPLETE (time %dms)\n", HAL::GetTimeStamp());
              isRecording_ = false;
            }
          }
        }
        
        
        return retVal;
        
      } // Update()
      
      HAL::IMU_DataStructure GetLatestRawData()
      {
        return imu_data_;
      }
      
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
      
      bool IsPickedUp()
      {
        return pickedUp_;
      }
      
      
      void RecordAndSend(const u32 length_ms)
      {
        PRINT("STARTING IMU RECORDING (time = %dms)\n", HAL::GetTimeStamp());
        isRecording_ = true;
        recordDataIdx_ = 0;
        imuChunkMsg_.seqId++;
        imuChunkMsg_.chunkId=0;
        imuChunkMsg_.totalNumChunks = length_ms / (TIME_STEP * IMU_CHUNK_SIZE);
      }
      
    } // namespace IMUFilter
  } // namespace Cozmo
} // namespace Anki
