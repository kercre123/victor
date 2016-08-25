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

#include "trig_fast.h"
#include "imuFilter.h"
#include <math.h>
#include "headController.h"
#include "liftController.h"
#include "wheelController.h"
#include "pathFollower.h"
#include "pickAndPlaceController.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"
#include "messages.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

// For event callbacks
#include "testModeController.h"

#define DEBUG_IMU_FILTER 0

// Define type of data to send when IMURequest received
#define RECORD_AND_SEND_RAW_DATA  0
#define RECORD_AND_SEND_FILT_DATA 1
#define RECORD_AND_SEND_MODE RECORD_AND_SEND_RAW_DATA


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

        f32 gyro_filt[3]                = {0};    // Filtered gyro measurements
        f32 gyro_robot_frame_filt[3]    = {0};    // Filtered gyro measurements in robot frame
        const f32 RATE_FILT_COEFF       = 1.f;    // IIR filter coefficient (1 == disable filter)
        
        f32 gyro_drift_filt[3]          = {0};    // Filtered gyro drift offset. (Accumulate only when all axes are below GYRO_MOTION_THRESH)
        const f32 RATE_DRIFT_FILT_COEFF = 0.01f;   // IIR filter coefficient (1 == disable filter)

        f32 accel_filt[3]               = {0};    // Filtered accelerometer measurements
        f32 accel_robot_frame_filt[3]   = {0};    // Filtered accelerometer measurements in robot frame
        const f32 ACCEL_FILT_COEFF      = 0.1f;   // IIR filter coefficient (1 == disable filter)

        // ==== Pickup detection ===
        bool pickupDetectEnabled_ = true;
        bool pickedUp_ = false;

        bool enablePickupParalysis_ = false;

        // Filter coefficient on z-axis accelerometer
        const f32 ACCEL_PICKUP_FILT_COEFF = 0.1f;

        f32 pdFiltAccX_aligned_ = 0;
        f32 pdFiltAccY_aligned_ = 0;
        f32 pdFiltAccZ_aligned_ = 0;
        f32 pdFiltAccX_ = 0.f;
        f32 pdFiltAccY_ = 0.f;
        f32 pdFiltAccZ_ = 9800.f;
        u8 pdRiseCnt_ = 0;
        u8 pdFallCnt_ = 0;
        u8 pdLastHeadMoveCnt_ = 0;
        u8 pdUnexpectedMotionCnt_ = 0;
        // === End of Pickup detection ===


        u32 lastMotionDetectedTime_ms = 0;
        const u32 MOTION_DETECT_TIMEOUT_MS = 250;
        const f32 GYRO_MOTION_THRESHOLD = DEG_TO_RAD_F32(3.f);  // Maximum allowable drift from gyro post-calibration


        // Recorded buffer
        bool isRecording_ = false;
        
#if(RECORD_AND_SEND_MODE == RECORD_AND_SEND_FILT_DATA)
        u8 recordDataIdx_ = 0;
        RobotInterface::IMUDataChunk imuChunkMsg_;
#else
        RobotInterface::IMURawDataChunk imuRawDataMsg_;
        u16 totalIMUDataMsgsToSend_ = 0;
        u16 sentIMUDataMsgs_ = 0;
#endif


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
        const u32 eventActivationTime_ms_[NUM_IMU_EVENTS] = {500, 2000, 500, 500, 500};

        // The amount of time required for eventStateRaw to be false for eventState to be false
        const u32 eventDeactivationTime_ms_[NUM_IMU_EVENTS] = {500, 500, 500, 500, 500};

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

      } // "private" namespace







      // Implementation of Madgwick's IMU and AHRS algorithms.
      // See: http://www.x-io.co.uk/node/8#open_source_ahrs_and_imu_algorithms

      //---------------------------------------------------------------------------------------------------
      // Definitions
      const float beta	= 0.1f;		// 2 * proportional gain

      //---------------------------------------------------------------------------------------------------
      // Variable definitions
      volatile float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;	// quaternion of sensor frame relative to auxiliary frame

      // Euler Z-angle
      float _zAngle = 0;

      //---------------------------------------------------------------------------------------------------
      // Function declarations

      float invSqrt(float x);

      //---------------------------------------------------------------------------------------------------
      // IMU algorithm update

      void MadgwickAHRSupdateIMU(float gx, float gy, float gz, float ax, float ay, float az) {
        float recipNorm;
        float s0, s1, s2, s3;
        float qDot1, qDot2, qDot3, qDot4;
        float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2 ,_8q1, _8q2, q0q0, q1q1, q2q2, q3q3;

        // Rate of change of quaternion from gyroscope
        qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
        qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
        qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
        qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

        // Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
        if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

          // Normalise accelerometer measurement
          recipNorm = invSqrt(ax * ax + ay * ay + az * az);
          ax *= recipNorm;
          ay *= recipNorm;
          az *= recipNorm;

          // Auxiliary variables to avoid repeated arithmetic
          _2q0 = 2.0f * q0;
          _2q1 = 2.0f * q1;
          _2q2 = 2.0f * q2;
          _2q3 = 2.0f * q3;
          _4q0 = 4.0f * q0;
          _4q1 = 4.0f * q1;
          _4q2 = 4.0f * q2;
          _8q1 = 8.0f * q1;
          _8q2 = 8.0f * q2;
          q0q0 = q0 * q0;
          q1q1 = q1 * q1;
          q2q2 = q2 * q2;
          q3q3 = q3 * q3;

          // Gradient decent algorithm corrective step
          s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
          s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
          s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
          s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;
          recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
          s0 *= recipNorm;
          s1 *= recipNorm;
          s2 *= recipNorm;
          s3 *= recipNorm;

          // Apply feedback step
          qDot1 -= beta * s0;
          qDot2 -= beta * s1;
          qDot3 -= beta * s2;
          qDot4 -= beta * s3;
        }

        // Integrate rate of change of quaternion to yield quaternion
        q0 += qDot1 * (1.0f * CONTROL_DT);   // q0 += qDot1 * (1.0f / sampleFreq);
        q1 += qDot2 * (1.0f * CONTROL_DT);
        q2 += qDot3 * (1.0f * CONTROL_DT);
        q3 += qDot4 * (1.0f * CONTROL_DT);

        // Normalise quaternion
        recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
        q0 *= recipNorm;
        q1 *= recipNorm;
        q2 *= recipNorm;
        q3 *= recipNorm;


        // Compute zAngle from quaternion
        _zAngle = atan2_fast(2*(q0*q3 + q1*q2), 1 - 2*(q2*q2 + q3*q3) );
      }

      //---------------------------------------------------------------------------------------------------
      // Fast inverse square-root
      // See: http://en.wikipedia.org/wiki/Fast_inverse_square_root

      float invSqrt(float x) {
        float halfx = 0.5f * x;
        float y = x;
        long i = *(long*)&y;
        i = 0x5f3759df - (i>>1);
        y = *(float*)&i;
        y = y * (1.5f - (halfx * y * y));
        return y;
      }
      /// --------------------------------------------------------------------------------------------------





      // ==== Event callback functions ===

      void TurnOnIndicatorLight()
      {
#ifndef TARGET_K02
        TestModeController::Start(TM_NONE);
#endif
        HAL::SetLED(INDICATOR_LED_ID, LED_ENC_RED);
      }
      void TurnOffIndicatorLight()
      {
        HAL::SetLED(INDICATOR_LED_ID, LED_ENC_OFF);
      }

      void StartPathFollowTest()
      {
        #ifndef TARGET_K02
        TestModeController::Start(TM_PATH_FOLLOW);
        #endif
        TurnOffIndicatorLight();
      }

      void StartLiftTest()
      {
        #ifndef TARGET_K02
        TestModeController::Start(TM_LIFT);
        #endif
        TurnOffIndicatorLight();
      }

      //===== End of event callbacks ====
      void SetPickupDetect(bool pickupDetected)
      {
        // TEST WITH LIGHT
        if (pickupDetected) {
          HAL::SetLED(INDICATOR_LED_ID, LED_ENC_RED);
        } else {
          HAL::SetLED(INDICATOR_LED_ID, LED_ENC_OFF);
        }

        pickedUp_ = pickupDetected;
        pdFallCnt_ = 0;
        pdRiseCnt_ = 0;
        pdLastHeadMoveCnt_ = 0;
        pdUnexpectedMotionCnt_ = 0;
      }

      void EnablePickupDetect(bool enable)
      {
        SetPickupDetect(false);
        pickupDetectEnabled_ = enable;
      }

      void EnablePickupParalysis(bool enable)
      {
        if (enable) { AnkiEvent( 327, "IMUFilter.PickupParalysis.Enabled", 305, "", 0); }
        else        { AnkiEvent( 328, "IMUFilter.PickupParalysis.Disabled", 305, "", 0); }
        enablePickupParalysis_ = enable;
      }

      void HandlePickupParalysis()
      {
        static bool wasParalyzed = false;
        if (enablePickupParalysis_) {
          if (IsPickedUp() && !wasParalyzed) {
            // Stop all movement (so we don't hurt people's hands)
            PickAndPlaceController::Reset();
            PathFollower::ClearPath();

            LiftController::Disable();
            HeadController::Disable();
            WheelController::Disable();
            wasParalyzed = true;
          }
        }

        if((!IsPickedUp() || !enablePickupParalysis_) && wasParalyzed) {
          // Just got put back down
          LiftController::Enable();
          HeadController::Enable();
          WheelController::Enable();
          wasParalyzed = false;
        }
      }

      void Reset()
      {
        rot_ = 0;
        rotSpeed_ = 0;
        
        pitch_ = 0;
        pickedUp_ = 0;
        imu_data_.Reset();

        //eventActivationCallbacks[LEFTSIDE_DOWN] = TurnOnIndicatorLight;
        //eventDeactivationCallbacks[LEFTSIDE_DOWN] = StartPathFollowTest;
        //eventActivationCallbacks[FRONTSIDE_DOWN] = TurnOnIndicatorLight;
        //eventDeactivationCallbacks[FRONTSIDE_DOWN] = StartLiftTest;
      }


      // Simple poke detect
      // If wheels aren't moving but a sudden rotation about z-axis was detected
      void DetectPoke()
      {
        static TimeStamp_t peakGyroStartTime = 0;
        static TimeStamp_t peakGyroMaxTime = 0;
        static TimeStamp_t peakAccelStartTime = 0;
        static TimeStamp_t peakAccelMaxTime = 0;
        static TimeStamp_t lastPokeDetectTime = 0;
        const u32 pokeDetectRefractoryPeriod_ms = 1000;

        // Do nothing during refractory period
        TimeStamp_t currTime = HAL::GetTimeStamp();
        if (currTime - lastPokeDetectTime < pokeDetectRefractoryPeriod_ms) {
          peakGyroStartTime = currTime;
          peakAccelStartTime = currTime;
          return;
        }
        // Only check for poke when wheels are not being driven
        if (!WheelController::AreWheelsMoving()) {

          // Check for a gyro rotation spike
          const f32 peakGyroThresh = 4.f;
          const u32 maxGyroPeakDuration_ms = 75;
          if (fabsf(gyro_robot_frame_filt[2]) > peakGyroThresh) {
            peakGyroMaxTime = currTime;
          } else if (fabsf(gyro_robot_frame_filt[2]) < peakGyroThresh) {
            if ((peakGyroMaxTime > peakGyroStartTime) && (peakGyroMaxTime - peakGyroStartTime < maxGyroPeakDuration_ms)) {
              AnkiEvent( 329, "IMUFilter.PokeDetected.Gyro", 305, "", 0);
              peakGyroStartTime = currTime;
              lastPokeDetectTime = currTime;

              RobotInterface::RobotPoked m;
              RobotInterface::SendMessage(m);
            } else {
              peakGyroStartTime = currTime;
            }
          }

          // Check for accel spike
          if (!HeadController::IsMoving() && !LiftController::IsMoving()) {
            const f32 peakAccelThresh = 4000.f;
            const u32 maxAccelPeakDuration_ms = 75;
            if (fabsf(accel_robot_frame_filt[0]) > peakAccelThresh) {
              peakAccelMaxTime = currTime;
            } else if (fabsf(accel_robot_frame_filt[0]) < peakAccelThresh) {
              if ((peakAccelMaxTime > peakAccelStartTime) && (peakAccelMaxTime - peakAccelStartTime < maxAccelPeakDuration_ms)) {
                AnkiEvent( 330, "IMUFilter.PokeDetected.Accel", 305, "", 0);
                peakAccelStartTime = currTime;
                lastPokeDetectTime = currTime;

                RobotInterface::RobotPoked m;
                RobotInterface::SendMessage(m);
              } else {
                peakAccelStartTime = currTime;
              }
            }
          } else {
            peakAccelStartTime = currTime;
          }

        } else {
          peakGyroStartTime = currTime;
          peakAccelStartTime = currTime;
        }
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

      // Checks for accelerations
      bool CheckUnintendedAcceleration() {
        return (ABS(pdFiltAccX_aligned_) > 5000) ||
               (ABS(pdFiltAccY_aligned_) > 5000) ||
               (ABS(pdFiltAccZ_aligned_) > 12000);
      }


      bool MotionDetected() {
        return (lastMotionDetectedTime_ms + MOTION_DETECT_TIMEOUT_MS) >= HAL::GetTimeStamp();
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
        if (!pickupDetectEnabled_)
          return;

        // Compute the acceleration componenet aligned with the z-axis of the robot
        const f32 xzAccelMagnitude = sqrtf(pdFiltAccX_*pdFiltAccX_ + pdFiltAccZ_*pdFiltAccZ_);
        const f32 accel_angle_imu_frame = atan2_fast(pdFiltAccZ_, pdFiltAccX_);
        const f32 accel_angle_robot_frame = accel_angle_imu_frame + HeadController::GetAngleRad();

        pdFiltAccX_aligned_ = xzAccelMagnitude * cosf(accel_angle_robot_frame);
        pdFiltAccY_aligned_ = pdFiltAccY_;
        pdFiltAccZ_aligned_ = xzAccelMagnitude * sinf(accel_angle_robot_frame);



        if (IsPickedUp()) {

          // Picked up flag is reset only when the robot has stopped moving
          // and it has been set upright.
          if (!MotionDetected() &&
              !CheckUnintendedAcceleration() &&
              (accel_robot_frame_filt[2] > NSIDE_DOWN_THRESH_MMPS2)) {
            SetPickupDetect(false);
          }

        } else {
          // Do simple check first.
          // If wheels aren't moving, any motion is because a person was messing with it!
          if (!WheelController::AreWheelsPowered()
              && !HeadController::IsMoving() && HeadController::IsInPosition()
              && !LiftController::IsMoving() && LiftController::IsInPosition()) {
            if (CheckUnintendedAcceleration()
                || ABS(gyro_robot_frame_filt[0]) > 5.f
                || ABS(gyro_robot_frame_filt[1]) > 5.f
                //|| ABS(gyro_robot_frame_filt[2]) > 5.f  // Not checking z-rotation since it is being used to trigger naive poke detection
                ) {
              if (++pdUnexpectedMotionCnt_ > 40) {
                AnkiInfo( 331, "IMUFilter.PDWhileStationary", 581, "acc (%f, %f, %f), gyro (%f, %f, %f)", 6,
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
          if (!IsPickedUp() && CheckUnintendedAcceleration()) {
            ++pdRiseCnt_;
            if (pdRiseCnt_ > 40) {
              SetPickupDetect(true);
              AnkiInfo( 332, "IMUFilter.PickupDetected", 582, "accX = %f, accY = %f, accZ = %f", 3,
                    pdFiltAccX_aligned_, pdFiltAccY_aligned_, pdFiltAccZ_aligned_);
            }
          } else {
            pdRiseCnt_ = 0;
          }

        }
      }

      void UpdateEventDetection()
      {
        // Call detect functions and update raw event state
        DetectPoke();
        DetectFalling();
        DetectNsideDown();


        // Now update event state according to (de)activation time
        u32 currTime = HAL::GetTimeStamp();
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
              } else if (currTime - eventTime_[e] > eventActivationTime_ms_[e]) {
                // Event activated
                eventState_[e] = true;

                // Call activation function
                if (eventActivationCallbacks[e]) {
#if(DEBUG_IMU_FILTER)
                  AnkiDebug( 333, "IMUFilter.EventActivationCallback", 583, "callback %d", 1, e);
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
              } else if (currTime - eventTime_[e] > eventDeactivationTime_ms_[e]) {
                // Event deactivated
                eventState_[e] = false;

                // Call deactivation function
                if (eventDeactivationCallbacks[e]) {
#if(DEBUG_IMU_FILTER)
                  AnkiDebug( 334, "IMUFilter.EventDeactivationCallback", 583, "callback %d", 1, e);
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
        u32 currTime = HAL::GetTimeStamp();
        // Are wheels or head being powered?
        if (WheelController::AreWheelsPowered() || !HeadController::IsInPosition()) {
          lastMotionDetectedTime_ms = currTime;
        }
        
        // Check gyro for motion
        if (ABS(imu_data_.rate_x) > GYRO_MOTION_THRESHOLD ||
            ABS(imu_data_.rate_y) > GYRO_MOTION_THRESHOLD ||
            ABS(imu_data_.rate_z) > GYRO_MOTION_THRESHOLD) {
          lastMotionDetectedTime_ms = currTime;
        }
        
        // TODO: Gyro seems to be sensitive enough that it's sufficient for detecting motion, but if
        //       that's not the case, check for changes in gravity vector.
        // ...
        
        
/*
        // Measure peak readings every 2 seconds
        static f32 max_gyro[3] = {0,0,0};
        for (int i=0; i<3; ++i) {
          if(ABS(gyro_robot_frame_filt[i]) > max_gyro[i]) {
            max_gyro[i] = ABS(gyro_robot_frame_filt[i]);
          }
        }
        
        static u32 measurement_cycles = 0;
        if (measurement_cycles++ == 400) {
          AnkiDebug( 25, "IMUFilter", 166, "Max gyro: %f %f %f", 3,
                    max_gyro[0],
                    max_gyro[1],
                    max_gyro[2]);
          
          measurement_cycles = 0;
          for (int i=0; i<3; ++i) {
            max_gyro[i] = 0;
          }
        }
*/
        
      }

      void UpdatePitch()
      {
        static f32 lastHeadAngle = 10000;
        f32 headAngle = HeadController::GetAngleRad();

        // If not moving then reset pitch angle with accelerometer.
        // Otherwise, update it with gyro.
        if (!MotionDetected()) {
          pitch_ = atan2(accel_filt[0], accel_filt[2]) - headAngle;
          
        // Was originally only updating pitch if head wasn't moving in order to avoid noisy pitch measurements,
        // that was causing some problems with pounce success detection so now updating pitch all the time.
        // NB: This pitch measurement isn't precise to begin with, but it's extra imprecise when the head is moving
        // so be careful relying on it when the head is moving.
        //} else if (HeadController::IsInPosition() && !HeadController::IsMoving()) {
        } else if (lastHeadAngle != 10000) {
          f32 dAngle = -gyro_robot_frame_filt[1] * CONTROL_DT;
          pitch_ += dAngle - (headAngle - lastHeadAngle);
        }
        
        lastHeadAngle = headAngle;

        //AnkiDebugPeriodic(50, 182, "RobotPitch", 483, "%f deg (motion %d, gyro %f)", 3, RAD_TO_DEG_F32(pitch_), MotionDetected(), gyro_robot_frame_filt[1]);
      }
      
      void UpdateCameraMotion()
      {
        static u8 cameraMotionDecimationCounter = 0;
        if (cameraMotionDecimationCounter++ > 3)
        {
          ImageImuData msg;
          HAL::IMUGetCameraTime(&msg.imageId, &msg.line2Number);
          msg.rateX = gyro_robot_frame_filt[0];
          msg.rateY = gyro_robot_frame_filt[1];
          msg.rateZ = gyro_robot_frame_filt[2];
          RobotInterface::SendMessage(msg);
          cameraMotionDecimationCounter = 0;
        }
      }

      Result Update()
      {
        Result retVal = RESULT_OK;

        // Don't do IMU updates until head is calibrated
        if (!HeadController::IsCalibrated()) {
          Reset();
          return retVal;
        }


        // Get IMU data
        // NB: Only call IMUReadData once per mainExecution tic!
        while (HAL::IMUReadData(imu_data_)) {

        ////// Gyro Update //////

        // Filter rotation speeds
        // TODO: Do this in hardware?
        gyro_filt[0] = (imu_data_.rate_x - gyro_drift_filt[0]) * RATE_FILT_COEFF + gyro_filt[0] * (1.f-RATE_FILT_COEFF);
        gyro_filt[1] = (imu_data_.rate_y - gyro_drift_filt[1]) * RATE_FILT_COEFF + gyro_filt[1] * (1.f-RATE_FILT_COEFF);
        gyro_filt[2] = (imu_data_.rate_z - gyro_drift_filt[2]) * RATE_FILT_COEFF + gyro_filt[2] * (1.f-RATE_FILT_COEFF);


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

        DetectMotion();
        UpdatePitch();

        // XY-plane rotation rate is robot frame z-axis rotation rate
        rotSpeed_ = gyro_robot_frame_filt[2];

        // Update orientation
        f32 dAngle = rotSpeed_ * CONTROL_DT;
        rot_ += dAngle;
        
        MadgwickAHRSupdateIMU(imu_data_.rate_x, imu_data_.rate_y, imu_data_.rate_z,
                              imu_data_.acc_x, imu_data_.acc_y, imu_data_.acc_z);
        //MadgwickAHRSupdateIMU(gyro_filt[0], gyro_filt[1], gyro_filt[2],
        //                      accel_filt[0], accel_filt[1], accel_filt[2]);

        
        if (!MotionDetected()) {
          // Update gyro drift offset while not moving
          gyro_drift_filt[0] = imu_data_.rate_x * RATE_DRIFT_FILT_COEFF + gyro_drift_filt[0] * (1.f - RATE_DRIFT_FILT_COEFF);
          gyro_drift_filt[1] = imu_data_.rate_y * RATE_DRIFT_FILT_COEFF + gyro_drift_filt[1] * (1.f - RATE_DRIFT_FILT_COEFF);
          gyro_drift_filt[2] = imu_data_.rate_z * RATE_DRIFT_FILT_COEFF + gyro_drift_filt[2] * (1.f - RATE_DRIFT_FILT_COEFF);
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

        UpdateEventDetection();

        HandlePickupParalysis();
        
        UpdateCameraMotion();

        // Recording IMU data for sending to basestation
        if (isRecording_) {

#if(RECORD_AND_SEND_MODE == RECORD_AND_SEND_FILT_DATA)
          imuChunkMsg_.aX[recordDataIdx_] = accel_robot_frame_filt[0];
          imuChunkMsg_.aY[recordDataIdx_] = accel_robot_frame_filt[1];
          imuChunkMsg_.aZ[recordDataIdx_] = accel_robot_frame_filt[2];

          imuChunkMsg_.gX[recordDataIdx_] = gyro_robot_frame_filt[0];
          imuChunkMsg_.gY[recordDataIdx_] = gyro_robot_frame_filt[1];
          imuChunkMsg_.gZ[recordDataIdx_] = gyro_robot_frame_filt[2];


          // Send message when it's full
          if (++recordDataIdx_ == IMU_CHUNK_SIZE) {
            RobotInterface::SendMessage(imuChunkMsg_);
            recordDataIdx_ = 0;
            ++imuChunkMsg_.chunkId;

            if (imuChunkMsg_.chunkId == imuChunkMsg_.totalNumChunks) {
              AnkiDebug( 335, "IMUFilter.IMURecording.Complete", 584, "time %dms", 1, HAL::GetTimeStamp());
              isRecording_ = false;
            }
          }
#else
      
          // Raw IMU chunks
          HAL::IMUReadRawData(imuRawDataMsg_.a, imuRawDataMsg_.g, &imuRawDataMsg_.timestamp);
          ++sentIMUDataMsgs_;
          if (sentIMUDataMsgs_ == totalIMUDataMsgsToSend_) {
            AnkiDebug( 336, "IMUFilter.IMURecording.CompleteRaw", 584, "time %dms", 1, HAL::GetTimeStamp());
            isRecording_ = false;
            imuRawDataMsg_.order = 2;  // 2 == last msg of sequence
          }
          RobotInterface::SendMessage(imuRawDataMsg_);
          imuRawDataMsg_.order = 1;    // 1 == intermediate msg of sequence
#endif

        }

        } // while (HAL::IMUReadData())
        
        return retVal;

      } // Update()

      HAL::IMU_DataStructure GetLatestRawData()
      {
        return imu_data_;
      }

      f32 GetRotation()
      {
        //return _zAngle;  // Computed from 3D orientation tracker (Madgwick filter)
        return rot_;     // Computed from simplified yaw-only tracker
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
        AnkiDebug( 337, "IMUFilter.IMURecording.Start", 585, "time = %dms", 1, HAL::GetTimeStamp());
        isRecording_ = true;
#if(RECORD_AND_SEND_MODE == RECORD_AND_SEND_FILT_DATA)
        recordDataIdx_ = 0;
        imuChunkMsg_.seqId++;
        imuChunkMsg_.chunkId=0;
        imuChunkMsg_.totalNumChunks = length_ms / (TIME_STEP * IMU_CHUNK_SIZE);
#else
        imuRawDataMsg_.order = 0; // 0 == first message of sequence
        sentIMUDataMsgs_ = 0;
        totalIMUDataMsgsToSend_ = length_ms / TIME_STEP;
#endif
      }

    } // namespace IMUFilter
  } // namespace Cozmo
} // namespace Anki
