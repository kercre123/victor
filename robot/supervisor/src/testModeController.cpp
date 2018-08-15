#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "dockingController.h"
#include "headController.h"
#include "liftController.h"
#include "imuFilter.h"
#include "localization.h"
#include "messages.h"
#include "pathFollower.h"
#include "pickAndPlaceController.h"
#include "proxSensors.h"
#include "speedController.h"
#include "steeringController.h"
#include "testModeController.h"
#include "trig_fast.h"
#include "wheelController.h"

#include <math.h>



namespace Anki {
  namespace Vector {
    namespace TestModeController {
      
      // "Private Member Variables"
      namespace {

        // Some common vars that can be used across multiple tests
        u32 ticCnt_, ticCnt2_;   // Multi-purpose tic counters
        
        // The number of cycles in between printouts
        // in those tests that print something out.
        u32 printCyclePeriod_;

        // On motor power tests, whether or not to cycle through increasing power or to
        // use set power level
        bool increasePower_ = true;

        //////// DriveTest /////////
        bool enableToggleDir_ = false;   // false: Only drive forward
                                         // true: Switch between driving forward and reverse

        bool enableDirectHALTest_ = false;  // false: Test WheelController
                                            // true:  Test direct drive via HAL::MotorSetPower

        bool enableCycleSpeedsTest_ = false; // false: Drive motors at the commanded speed/power
                                             // true: Cycle through different power levels increasing by wheelPowerStep each time


        const u32 WHEEL_TOGGLE_DIRECTION_PERIOD_MS = 2000;

        // Acceleration
        const f32 accel_mmps2 = 0;  // 0 for infinite acceleration.
                                    // Only works in WheelController mode (i.e. DIRECT_HAL_MOTOR_TEST == 0)

        // Measurements taken from a prototype cozmo.
        // Used to model open loop motor command in WheelController.
        //
        // *** COZMO 2 ***
        // Power   LSpeed  RSpeed (approx,In-air speeds)
        // 1.0    120     125
        // 0.9    110     115
        // 0.8    100     100
        // 0.7     84      88
        // 0.6     65      68
        // 0.5     52      55
        // 0.4     38      39
        // 0.3     25      25
        // 0.25    -       -

        // *** COZMO 3 (with clamping on, i.e. MAX_WHEEL_SPEED == 0.5) ***
        // Power   LSpeed  RSpeed (approx,In-air speeds)
        // 0.1      0      0
        // 0.15    15     15
        // 0.2     50     40
        // 0.25    70     60
        // 0.3     90     80
        // 0.35   104     96
        // 0.4    120    112
        // 0.5    137    131
        // 0.6    150    145
        // 0.7    155    153
        // 0.8    165    164
        // 0.9    170    173
        // 1.0    174    177

        // *** COZMO 3.2 (with clamping on, i.e. MAX_WHEEL_SPEED == 0.75) ***
        // Power   LSpeed  RSpeed (approx,In-air speeds)
        // 0.2      0      0
        // 0.25     0      0
        // 0.3      4      0
        // 0.35    13     13
        // 0.4     27     27
        // 0.5     49     49
        // 0.6     67     67
        // 0.7     91     91
        // 0.8    109    109
        // 0.9    128    128
        // 1.0    150    147

        // Percent power to step up when
        // DTF_ENABLE_CYCLE_SPEEDS_TEST is enabled
        const u32 DEFAULT_WHEEL_POWER_STEP = 5;

        // Default wheel speed
        const f32 WHEEL_SPEED_CMD_MMPS = 60;

        // Percent by which to approach command wheelTargetPower.
        // Make low to prevent motor slamming
        const u32 WHEEL_POWER_PERCENT_ACCEL = 1000;

        s32 wheelPower_;
        u32 wheelPowerStep_;
        s32 wheelTargetPower_;
        f32 wheelSpeed_mmps_;
        s8 wheelTargetDir_;

        ////// End of DriveTest ////////

        /////// LiftTest /////////
        // 0: Set power directly with MotorSetPower
        // 1: Command a desired lift height (i.e. use LiftController)
        #define LIFT_HEIGHT_TEST 1

        // Open-loop lift speeds (unloaded)
        //
        // *** Cozmo 3 (treaded) ***
        // Power   UpSpeed (rad/s)  DownSpeed (rad/s)
        // 0.2     0.0              0.0
        // 0.3     0.7              0.7
        // 0.4     1.0              1.1
        // 0.5     1.6              1.6
        // 0.6     2.0              2.0
        // 0.7     2.5              2.6
        // 0.8     3.0              3.1
        // 0.9     3.2              3.4
        // 1.0     3.7              3.9
        //
        //
        // *** Cozmo 3.2 (stronger lift motor, lower gear reduction) ***
        // Power   UpSpeed (rad/s)  DownSpeed (rad/s)
        // 0.2     1.0              1.0 (only moved a few degrees)
        // 0.25    1.5              1.5 (only moved a few degrees)
        // 0.3     2.0              3.0
        // 0.35    2.5              3.5
        // 0.4     3.0              5.0
        // 0.45    4.0              6.0
        // 0.5     5.0              7.0
        // 0.55    6.0              8.0
        // 0.6     7.0              9.2
        // 0.65    8.0              10.0
        // 0.7     9.0              11.0
        // 0.75    10               12
        // 0.8     11               13
        // 0.85    12               14
        // 0.9     13               14.5
        // 0.95    14               15
        // 1.0     15               16

        f32 liftPower_;
        const f32 LIFT_POWER_CMD = 0.2f;
        const f32 LIFT_DES_HIGH_HEIGHT = LIFT_HEIGHT_CARRY - 10;
        const f32 LIFT_DES_LOW_HEIGHT = LIFT_HEIGHT_LOWDOCK + 10;
        LiftTestFlags liftTestMode_ = LiftTestFlags::LiftTF_TEST_POWER;
        s32 liftNodCycleTime_ms_ = 2000;
        f32 avgLiftSpeed_ = 0;
        f32 startLiftHeightMM_ = 0;
        f32 maxLiftSpeed_radPerSec_ = 0;
        //// End of LiftTest  //////


        //////// HeadTest /////////
        // 0: Set power directly with MotorSetPower
        // 1: Command a desired head angle (i.e. use HeadController)
        #define HEAD_POSITION_TEST 1

        f32 headPower_;
        const f32 HEAD_POWER_CMD = 0.2;
        const f32 HEAD_DES_HIGH_ANGLE = MAX_HEAD_ANGLE;
        const f32 HEAD_DES_LOW_ANGLE = MIN_HEAD_ANGLE;
        HeadTestFlags headTestMode_ = HeadTestFlags::HTF_TEST_POWER;
        s32 headNodCycleTime_ms_ = 2000;
        f32 maxHeadSpeed_radPerSec_ = 0;
        
        //// End of HeadTest //////

        
        //////// DockPathTest /////////
        enum {
          DT_STOP,
          DT_STRAIGHT,
          DT_LEFT,
          DT_STRAIGHT2,
          DT_RIGHT
        };
        u8 dockPathState_ = DT_STOP;
        const f32 DOCK_PATH_TOGGLE_TIME_S = 3.f;

        ////// End of DockPathTest ////


        //////// PathFollowTest /////////
        #define PATH_FOLLOW_ALIGNED_START 1
        #define PATH_FOLLOW_BACKWARDS 0
        const f32 PF_TARGET_SPEED_MMPS = 100 * (PATH_FOLLOW_BACKWARDS ? -1 : 1);
        const f32 PF_ACCEL_MMPS2 = 200;
        const f32 PF_DECEL_MMPS2 = 500;
        bool pathStarted_;       // Flag for whether or not we started to traverse a path

        ////// End of PathFollowTest ////

        //////// PathFollowConvenienceFuncTest /////////
        enum {
          PFCF_DRIVE_STRAIGHT,
          PFCF_DRIVE_ARC,
          PFCF_DRIVE_POINT_TURN,
          PFCF_NUM_STATES
        };
        u8 pfcfState_ = PFCF_DRIVE_STRAIGHT;
        ////// End of PathFollowConvenienceFuncTest ////


        //////// StopTest /////////
        bool ST_go;
        f32 ST_speed;
        bool ST_slowingDown;
        f32 ST_prevLeftPos, ST_prevRightPos;
        u16 ST_slowDownTics;
        f32 ST_slowSpeed_mmps = 20;
        f32 ST_fastSpeed_mmps = 100;
        f32 ST_period_ms = 1000;
        ///// End of StopTest /////


        ///////// IMUTest ////////
        bool imuTestDoTurns_ = false;
        bool IT_turnLeft;
        const f32 IT_TARGET_ANGLE = DEG_TO_RAD_F32(180.f);
        const f32 IT_MAX_ROT_VEL = DEG_TO_RAD_F32(90.f);
        const f32 IT_ROT_ACCEL = DEG_TO_RAD_F32(720.f);
        const f32 IT_ROT_TOL = DEG_TO_RAD_F32(5.f);
        ///// End of IMUTest /////


        /////// LightTest ////////
        LEDId ledID_ = (LEDId)0;
        u8 ledColorIdx_ = 0;
        const u8 LED_COLOR_LIST_SIZE = 4;
        const LEDColor LEDColorList_[LED_COLOR_LIST_SIZE] = {LEDColor::LED_RED, LEDColor::LED_GREEN, LEDColor::LED_BLUE, LEDColor::LED_OFF};

        bool ledCycleTest_ = true;
        ///// End of LightTest ///

        // Current test mode
        TestMode testMode_ = TestMode::TM_NONE;

        // Pointer to update function for current test mode
        Result (*updateFunc)() = NULL;

      } // private namespace


      TestMode GetMode()
      {
        return testMode_;
      }

      // Bring robot to normal state and stops all motors
      Result Reset()
      {
        AnkiInfo( "TestModeController.Reset", "");

        // Reset state and updateFunc
        testMode_ = TestMode::TM_NONE;
        updateFunc = NULL;

        // Stop animations that might be playing
        //AnimationController::Clear();

        // Stop wheels and vision system
        WheelController::Enable();
        PickAndPlaceController::Reset();
        SteeringController::ExecuteDirectDrive(0,0);

        // Stop lift and head
        LiftController::Enable();
        LiftController::SetAngularVelocity(0);
        HeadController::Enable();
        HeadController::SetAngularVelocity(0);

        // Re-enable prox sensors
        ProxSensors::EnableCliffDetector(true);

        return RESULT_OK;
      }

      Result PlaceOnGroundTestInit(s32 x_offset_mm, s32 y_offset_mm, s32 angle_offset_degrees)
      {
        AnkiInfo( "TestModeController.PlaceOnGroundTestInit", "xOffset %d mm, yOffset %d mm, angleOffset %d degrees",
                 x_offset_mm, y_offset_mm, angle_offset_degrees);
        ticCnt_ = 0;
        PickAndPlaceController::PlaceOnGround(50,
                                              100,
                                              100,
                                              x_offset_mm,
                                              y_offset_mm,
                                              DEG_TO_RAD_F32(angle_offset_degrees));
        return RESULT_OK;
      }

      Result PlaceOnGroundTestUpdate()
      {
        if (!PickAndPlaceController::IsBusy()) {
          AnkiInfo( "TestModeController.PlaceOnGroundTestUpdate.Complete", "");
          Reset();
        }
        return RESULT_OK;
      }


      // Commands a path and executes it
      Result DockPathTestInit()
      {
        AnkiInfo( "TestModeController.DockPathTestInit", "");
        ticCnt_ = 0;
        dockPathState_ = DT_STOP;
        return RESULT_OK;
      }

      Result DockPathTestUpdate()
      {

        // Toggle dock path state
        if (ticCnt_++ >= DOCK_PATH_TOGGLE_TIME_S * ONE_OVER_CONTROL_DT ) {
          if (dockPathState_ == DT_RIGHT) {
            dockPathState_ = DT_STOP;
          } else {
            dockPathState_++;
          }
          AnkiInfo( "TestModeController.DockPathTestUpdate", "Test state: %d", dockPathState_);
          ticCnt_ = 0;
        }

        switch(dockPathState_)
        {
          case DT_STOP:
            break;
          case DT_STRAIGHT:
            DockingController::SetRelDockPose(200, 0, 0);
            break;
          case DT_LEFT:
            //DockingController::SetRelDockPose(200, 50, 0);
            DockingController::SetRelDockPose(200, 0, -0.1);
            break;
          case DT_STRAIGHT2:
            DockingController::SetRelDockPose(200, 0, 0);
            break;
          case DT_RIGHT:
            //DockingController::SetRelDockPose(200, -50, 0);
            DockingController::SetRelDockPose(200, 0, 0.1);
            break;
          default:
            break;
        }

        return RESULT_OK;
      }



      // Commands a path and executes it
      Result PathFollowTestInit()
      {
        AnkiInfo( "TestModeController.PathFollowTestInit", "");
        pathStarted_ = false;
        Localization::SetDriveCenterPose(0, 0, Radians(-M_PI_2_F));
        ProxSensors::EnableCliffDetector(false);
        return RESULT_OK;
      }

      Result PathFollowTestUpdate()
      {
        const u32 startDriveTime_ms = 500;
        if (!pathStarted_ && HAL::GetTimeStamp() > startDriveTime_ms) {

          /*
          // Small circle drive test
          PathFollower::ClearPath();
          Localization::SetDriveCenterPose(0, 0, M_PI_2_F);
          f32 speed = 20;
          f32 radius = WHEEL_DIST_HALF_MM;
          PathFollower::AppendPathSegment_Arc(-radius, 0, radius, 0,    M_PI_F, speed, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          PathFollower::AppendPathSegment_Arc(-radius, 0, radius, M_PI_F, M_PI_F, speed, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);

          PathFollower::StartPathTraversal();
          pathStarted_ = true;
          return RESULT_OK;
          */

          // Create a path and follow it
          PathFollower::ClearPath();
#if(PATH_FOLLOW_ALIGNED_START)
          Localization::SetDriveCenterPose(0, 0, M_PI_2_F * (PATH_FOLLOW_BACKWARDS ? 1 : -1));

          //PathFollower::AppendPathSegment_PointTurn(0, 0, -M_PI_2_F, -1.5f, 2.f, 2.f);

          float arc1_radius = sqrt((float)5000);  // Radius of sqrt(50^2 + 50^2)
          f32 sweepAng = atan_fast((350-arc1_radius)/250);

          PathFollower::AppendPathSegment_Arc(arc1_radius, 0, arc1_radius, -M_PI_F, sweepAng,
                                              PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);

          f32 firstConnectionPt_x = arc1_radius - arc1_radius*cos(sweepAng);
          f32 firstConnectionPt_y = - arc1_radius*sin(sweepAng);
          f32 secondConnectionPt_x = firstConnectionPt_x + (350 - arc1_radius);
          f32 secondConnectionPt_y = firstConnectionPt_y - 250;

          PathFollower::AppendPathSegment_Line(firstConnectionPt_x, firstConnectionPt_y, secondConnectionPt_x, secondConnectionPt_y,
                                               PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);

          PathFollower::AppendPathSegment_Arc(350, -250, arc1_radius, -M_PI_F + sweepAng, M_PI_F - sweepAng,
                                              PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
#else
          PathFollower::AppendPathSegment_Line(0.0, 0.0, 300, -300,
                                               PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          float arc1_radius = sqrt((float)5000);  // Radius of sqrt(50^2 + 50^2)
          PathFollower::AppendPathSegment_Arc(350, -250, arc1_radius, -0.75f*PI, 0.75f*PI,
                                              PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
#endif
          PathFollower::AppendPathSegment_Line(350 + arc1_radius, -250, 350 + arc1_radius, 200,
                                               PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          float arc2_radius = sqrt((float)20000); // Radius of sqrt(100^2 + 100^2)
          //PathFollower::AppendPathSegment_Arc(0.35 + arc1_radius - arc2_radius, 0.2, arc2_radius, 0, M_PI_2_F);
          PathFollower::AppendPathSegment_Arc(350 + arc1_radius - arc2_radius, 200, arc2_radius, 0, 3*M_PI_2_F,
                                              PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          PathFollower::AppendPathSegment_Arc(350 + arc1_radius - arc2_radius, 200 - 2*arc2_radius, arc2_radius, M_PI_2_F, -3*M_PI_2_F,
                                              PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);

          PathFollower::AppendPathSegment_Line(350 + arc1_radius - 2*arc2_radius, 200 - 2*arc2_radius, 350 + arc1_radius - 2*arc2_radius, 0,
                                               PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          float arc3_radius = 0.5f * (350 + arc1_radius - 2*arc2_radius);
          PathFollower::AppendPathSegment_Arc(arc3_radius, 0, arc3_radius, 0, M_PI_F,
                                              PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);


          PathFollower::StartPathTraversal();
          pathStarted_ = true;
        } else if (pathStarted_ && !PathFollower::IsTraversingPath()) {
          AnkiInfo( "TestModeController.PathFollowTestUpdate.Complete", "");
          Reset();
        }

        return RESULT_OK;
      }




      Result PathFollowConvenienceFuncTestInit()
      {
        AnkiInfo( "TestModeController.PathFollowConvenienceFuncTestInit", "");
        PathFollower::ClearPath();
        pfcfState_ = PFCF_DRIVE_STRAIGHT;
        return RESULT_OK;
      }

      Result PathFollowConvenienceFuncTestUpdate()
      {


        // Cycle through DriveStraight, DriveArc, and PointTurn
        if (!PathFollower::IsTraversingPath()) {

          switch(pfcfState_) {
            case PFCF_DRIVE_STRAIGHT:
              AnkiInfo( "TestModeController.PathFollowConvenienceFuncTestUpdate.DriveStraight", "Started");
              if (!PathFollower::DriveStraight(100, 0.25, 0.25, 2)) {
                 AnkiInfo( "TestModeController.PathFollowConvenienceFuncTestUpdate.DriveStraight", "FAILED");
                return RESULT_FAIL;
              }
              break;

            case PFCF_DRIVE_ARC:
              AnkiInfo( "TestModeController.PathFollowConvenienceFuncTestUpdate.DriveArc", "Started");
              if (!PathFollower::DriveArc(-M_PI_2_F, 40, 0.25, 0.25, 1)) {
                AnkiInfo( "TestModeController.PathFollowConvenienceFuncTestUpdate.DriveArc", "FAILED");
                return RESULT_FAIL;
              }


              break;
            case PFCF_DRIVE_POINT_TURN:
              AnkiInfo( "TestModeController.PathFollowConvenienceFuncTestUpdate.DrivePointTurn", "Started");
              if (!PathFollower::DrivePointTurn(-M_PI_2_F, 0.25, 0.25, DEG_TO_RAD_F32(1.f), 1)) {
              AnkiInfo( "TestModeController.PathFollowConvenienceFuncTestUpdate.DrivePointTurn", "FAILED");
                return RESULT_FAIL;
              }
              break;
            default:
              break;
          }

          if (++pfcfState_ == PFCF_NUM_STATES) {
            pfcfState_ = PFCF_DRIVE_STRAIGHT;
          }

        }

        return RESULT_OK;
      }

      // flags: See DriveTestFlags
      // powerStepPercent: The percent amount by which to increase power
      //                   (Only applies if DTF_ENABLE_DIRECT_HAL_TEST and DTF_ENABLE_CYCLE_SPEED_TESTS are set)
      // wheelSpeedOrPower: Wheel speed to command in mm/s (when DTF_ENABLE_DIRECT_HAL_TEST is not set)
      //                    or power to command as a percent (when DTF_ENABLE_DIRECT_HAL_TEST is set)
      Result DriveTestInit(s32 flags, s32 powerStepPercent, s32 wheelSpeedOrPower)
      {
        AnkiInfo( "TestModeController.DriveTestInit", "%x, %d, %d", flags, powerStepPercent, wheelSpeedOrPower);
        enableDirectHALTest_ = flags & EnumToUnderlyingType(DriveTestFlags::DTF_ENABLE_DIRECT_HAL_TEST);
        enableCycleSpeedsTest_ = flags & EnumToUnderlyingType(DriveTestFlags::DTF_ENABLE_CYCLE_SPEEDS_TEST);
        enableToggleDir_ = flags & EnumToUnderlyingType(DriveTestFlags::DTF_ENABLE_TOGGLE_DIR);
        wheelPowerStep_ = powerStepPercent != 0 ? powerStepPercent : DEFAULT_WHEEL_POWER_STEP;
        wheelSpeed_mmps_ = wheelSpeedOrPower != 0 ? wheelSpeedOrPower : WHEEL_SPEED_CMD_MMPS;
        wheelPower_ = 0;
        wheelTargetPower_ = enableCycleSpeedsTest_ ? 0 : wheelSpeedOrPower;
        wheelTargetDir_ = 1;

        // If CycleSpeedsTest is on, then enableToggleDir is on by default
        if (enableCycleSpeedsTest_) {
          enableToggleDir_ = true;
        }

        ticCnt_ = 0;
        return RESULT_OK;
      }

      Result DriveTestUpdate()
      {
        // Change direction (or at least print speed
        if (ticCnt_++ >= WHEEL_TOGGLE_DIRECTION_PERIOD_MS / ROBOT_TIME_STEP_MS) {

          f32 lSpeed_filt, rSpeed_filt;
          WheelController::GetFilteredWheelSpeeds(lSpeed_filt,rSpeed_filt);


          // Toggle wheel direction
          if (enableToggleDir_) {
            wheelTargetPower_ *= -1;
            wheelSpeed_mmps_ *= -1;
          }

          // Cycle through different power levels
          if (enableCycleSpeedsTest_) {
            if (wheelTargetPower_ >= 0) {
              wheelTargetPower_ += wheelPowerStep_;
              if (wheelTargetPower_ > 100) {
                wheelTargetPower_ = 0;
              }
            }
          }

          wheelTargetDir_ = wheelPower_ <= wheelTargetPower_ ? 1 : -1;

          f32 lSpeed = HAL::MotorGetSpeed(MotorID::MOTOR_LEFT_WHEEL);
          f32 rSpeed = HAL::MotorGetSpeed(MotorID::MOTOR_RIGHT_WHEEL);
          if (enableDirectHALTest_) {
            AnkiInfo( "TestModeController.DriveTestUpdate", "Applying %.3f power (currSpeed %.2f %.2f, filtSpeed %.2f %.2f)",
                  wheelTargetPower_*0.01f,
                  lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
          } else {
            AnkiInfo( "TestModeController.DriveTestUpdate", "Going %.3f mm/s (currSpeed %.2f %.2f, filtSpeed %.2f %.2f)",
                  wheelSpeed_mmps_,
                  lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
          }

          ticCnt_ = 0;

          // To prevent compiler error if AnkiInfo compiled out 
          (void)lSpeed;
          (void)rSpeed;
        }


        // Accelerate to desired power/speed.
        // Doing this to avoid slamming motors as this causes huge power spikes.
        if (enableDirectHALTest_) {
          WheelController::Disable();
          if (wheelTargetDir_ > 0 && wheelPower_ < wheelTargetPower_) {
            wheelPower_ += WHEEL_POWER_PERCENT_ACCEL;
            if (wheelPower_ > wheelTargetPower_) {
              wheelPower_ = wheelTargetPower_;
            }
            HAL::MotorSetPower(MotorID::MOTOR_LEFT_WHEEL, 0.01f*wheelPower_);
            HAL::MotorSetPower(MotorID::MOTOR_RIGHT_WHEEL, 0.01f*wheelPower_);
            //PRINT("WheelPower = %f (increasing)\n", 0.01f*wheelPower_);
          } else if (wheelTargetDir_ < 0 && wheelPower_ > wheelTargetPower_) {
            wheelPower_ -= WHEEL_POWER_PERCENT_ACCEL;
            if (wheelPower_ < wheelTargetPower_) {
              wheelPower_ = wheelTargetPower_;
            }
            HAL::MotorSetPower(MotorID::MOTOR_LEFT_WHEEL, 0.01f*wheelPower_);
            HAL::MotorSetPower(MotorID::MOTOR_RIGHT_WHEEL, 0.01f*wheelPower_);
            //PRINT("WheelPower = %f (decreasing)\n", 0.01f*wheelPower_);
          }
        } else {
          SteeringController::ExecuteDirectDrive(wheelSpeed_mmps_,
                                                 wheelSpeed_mmps_,
                                                 accel_mmps2,accel_mmps2);
        }


       return RESULT_OK;
      }


      Result LiftTestInit(s32 mode, s32 noddingCycleTime_ms, s32 powerPercent)
      {
        AnkiInfo( "TestModeController.LiftTestInit", "flags = %d, powerPercent = %d", mode, powerPercent);
        ticCnt_ = 0;
        ticCnt2_ = 0;
        printCyclePeriod_ = 250 / ROBOT_TIME_STEP_MS;

        increasePower_ = powerPercent == 0;
        if (!increasePower_) {
          liftPower_ = powerPercent * 0.01;
        } else {
          liftPower_ = LIFT_POWER_CMD;
        }
        
        liftTestMode_ = (LiftTestFlags)mode;
        liftNodCycleTime_ms_ = noddingCycleTime_ms == 0 ? 2000 : noddingCycleTime_ms;
        if (liftTestMode_ == LiftTestFlags::LiftTF_TEST_POWER || liftTestMode_ == LiftTestFlags::LiftTF_DISABLE_MOTOR) {
          LiftController::Disable();
        }
        avgLiftSpeed_ = 0;
        startLiftHeightMM_ = 0;
        maxLiftSpeed_radPerSec_ = 0;
        return RESULT_OK;
      }


      Result LiftTestUpdate()
      {

        switch (liftTestMode_) {
          case LiftTestFlags::LiftTF_NODDING: {
            static Radians theta = 0;
            const f32 lowHeight = LIFT_HEIGHT_LOWDOCK;
            const f32 highHeight = LIFT_HEIGHT_CARRY;
            const f32 midHeight = 0.5f * (highHeight + lowHeight);
            const f32 amplitude = highHeight - midHeight;

            const f32 thetaStep = 2*M_PI_F / liftNodCycleTime_ms_ * ROBOT_TIME_STEP_MS;

            f32 newHeight = amplitude * sinf(theta.ToFloat()) + midHeight;
            LiftController::SetDesiredHeight(newHeight);
            theta += thetaStep;

            break;
          }
          case LiftTestFlags::LiftTF_TEST_POWER:
          case LiftTestFlags::LiftTF_TEST_HEIGHTS:
          {
            static bool up = false;

            // As long as the lift is moving reset the counter
            f32 rotSpeed_radPerSec = fabsf(LiftController::GetAngularVelocity());
            if (rotSpeed_radPerSec > 0.1f) {
              ticCnt_ = 0;
            }
            
            // Check for max speed
            if (rotSpeed_radPerSec > maxLiftSpeed_radPerSec_) {
              maxLiftSpeed_radPerSec_ = rotSpeed_radPerSec;
            }

            // Change direction
            if (ticCnt_++ >= 1000 / ROBOT_TIME_STEP_MS) {

              if (liftTestMode_ == LiftTestFlags::LiftTF_TEST_HEIGHTS) {
                up = !up;
                if (up) {
                  AnkiInfo( "TestModeController.LiftTestUpdate", "Lift HIGH %f mm (maxSpeed %f rad/s)", LIFT_DES_HIGH_HEIGHT, maxLiftSpeed_radPerSec_);
                  LiftController::SetDesiredHeight(LIFT_DES_HIGH_HEIGHT);
                } else {
                  AnkiInfo( "TestModeController.LiftTestUpdate", "Lift LOW %f mm (maxSpeed %f rad/s)", LIFT_DES_LOW_HEIGHT, maxLiftSpeed_radPerSec_);
                  LiftController::SetDesiredHeight(LIFT_DES_LOW_HEIGHT);
                }

              } else {
                up = !up;
                if (up) {
                  AnkiInfo( "TestModeController.LiftTestUpdate", "Lift UP %f power (maxSpeed %f rad/s)", liftPower_, maxLiftSpeed_radPerSec_);
                  HAL::MotorSetPower(MotorID::MOTOR_LIFT, liftPower_);
                } else {
                  AnkiInfo( "TestModeController.LiftTestUpdate", "Lift DOWN %f power (maxSpeed %f rad/s)", -liftPower_, maxLiftSpeed_radPerSec_);
                  HAL::MotorSetPower(MotorID::MOTOR_LIFT, -liftPower_);
                }


                // Cycle through different power levels
                if (increasePower_) {
                  if (!up) {
                    liftPower_ += 0.1f;
                    if (liftPower_ >=1.01f) {
                      liftPower_ = LIFT_POWER_CMD;
                    }
                  }
                }
              }

              ticCnt_ = 0;
            }
            break;
          }
          case LiftTestFlags::LiftTF_DISABLE_MOTOR:
            break;
          default:
          {
            AnkiWarn( "TestModeController.LiftTestUpdate", "WARN: Unknown lift test mode %hhu", liftTestMode_);
            Reset();
          }
        }



        // Print speed at the end of a continuous segment of non-zero speeds
        f32 lSpeed = HAL::MotorGetSpeed(MotorID::MOTOR_LIFT);
        if (ABS(lSpeed) > 0.001f) {
          // Is this the start of a sequence of non-zero lift speeds?
          if (avgLiftSpeed_ == 0) {
            startLiftHeightMM_ = LiftController::GetHeightMM();
          }
          avgLiftSpeed_ += lSpeed;
          ++ticCnt2_;
        } else if (avgLiftSpeed_ != 0) {
          avgLiftSpeed_ /= ticCnt2_;
          AnkiInfo( "TestModeController.LiftTestUpdate", "Avg lift speed %f rad/s, height change %f mm", avgLiftSpeed_, LiftController::GetHeightMM() - startLiftHeightMM_);
          avgLiftSpeed_ = 0;
          ticCnt2_ = 0;
        }


        return RESULT_OK;
      }

      Result LiftToggleTestInit()
      {
        AnkiInfo( "TestModeController.LiftToggleTestInit", "");
        printCyclePeriod_ = 200 / ROBOT_TIME_STEP_MS;
        return RESULT_OK;
      }


      Result LiftToggleTestUpdate()
      {

        static f32 liftHeight = LIFT_HEIGHT_LOWDOCK;
        static u32 inPositionCycles = 0;

        if (LiftController::IsInPosition()) {
          if (inPositionCycles++ > 500) {

            if(liftHeight == LIFT_HEIGHT_LOWDOCK) {
              liftHeight = LIFT_HEIGHT_CARRY;
            } else if (liftHeight == LIFT_HEIGHT_CARRY) {
              liftHeight = LIFT_HEIGHT_HIGHDOCK;
            } else if (liftHeight == LIFT_HEIGHT_HIGHDOCK) {
              liftHeight = LIFT_HEIGHT_LOWDOCK;
            }
            AnkiInfo( "TestModeController.LiftToggleTestUpdate", "SET LIFT TO %f mm", liftHeight);
            LiftController::SetDesiredHeight(liftHeight);
            inPositionCycles = 0;
          }
        }

        // Print height and speed
        if (ticCnt2_++ >= printCyclePeriod_) {
          f32 lSpeed = HAL::MotorGetSpeed(MotorID::MOTOR_LIFT);
          f32 lPos = LiftController::GetHeightMM();

          AnkiInfo( "TestModeController.LiftToggleTestUpdate", "Lift speed %f rad/s, height %f mm", lSpeed, lPos);
          ticCnt2_ = 0;

          // To prevent compiler error if AnkiInfo compiled out
          (void)lSpeed;
          (void)lPos;
        }

        return RESULT_OK;
      }


      Result HeadTestInit(s32 mode, s32 noddingCycleTime_ms, s32 powerPercent)
      {
        AnkiInfo( "TestModeController.HeadTestInit", "flags = %d, powerPercent = %d", mode, powerPercent);
        ticCnt_ = 0;
        ticCnt2_ = 0;
        printCyclePeriod_ = 250 / ROBOT_TIME_STEP_MS;
        
        increasePower_ = powerPercent == 0;
        if (!increasePower_) {
          headPower_ = powerPercent * 0.01;
        } else {
          headPower_ = HEAD_POWER_CMD;
        }
        
        headTestMode_ = (HeadTestFlags)mode;
        headNodCycleTime_ms_ = noddingCycleTime_ms == 0 ? 3000 : noddingCycleTime_ms;
        if (headTestMode_ == HeadTestFlags::HTF_TEST_POWER) {
          HeadController::Disable();
        }
        maxHeadSpeed_radPerSec_ = 0;
        return RESULT_OK;
      }


      Result HeadTestUpdate()
      {

        switch(headTestMode_) {
          case HeadTestFlags::HTF_TEST_POWER:
          case HeadTestFlags::HTF_TEST_ANGLES:
          {
            static bool up = false;

            // Check for max speed
            f32 rotSpeed_radPerSec = ABS(HeadController::GetAngularVelocity());
            if (rotSpeed_radPerSec > maxHeadSpeed_radPerSec_) {
              maxHeadSpeed_radPerSec_ = rotSpeed_radPerSec;
            }
            
            // Change direction
            if (ticCnt_++ >= headNodCycleTime_ms_ / ROBOT_TIME_STEP_MS) {

              if (headTestMode_ == HeadTestFlags::HTF_TEST_ANGLES) {
                up = !up;
                if (up) {
                  AnkiInfo( "TestModeController.HeadTestUpdate", "Head HIGH %f rad (maxSpeed %f rad/s)", HEAD_DES_HIGH_ANGLE, maxHeadSpeed_radPerSec_);
                  HeadController::SetDesiredAngle(HEAD_DES_HIGH_ANGLE);
                } else {
                  AnkiInfo( "TestModeController.HeadTestUpdate", "Head LOW %f rad (maxSpeed %f rad/s)", HEAD_DES_LOW_ANGLE, maxHeadSpeed_radPerSec_);
                  HeadController::SetDesiredAngle(HEAD_DES_LOW_ANGLE);
                }
              } else {
                up = !up;
                if (up) {
                  AnkiInfo( "TestModeController.HeadTestUpdate", "Head UP %f power (maxSpeed %f rad/s)", headPower_, maxHeadSpeed_radPerSec_);
                  HAL::MotorSetPower(MotorID::MOTOR_HEAD, headPower_);
                } else {
                  AnkiInfo( "TestModeController.HeadTestUpdate", "Head DOWN %f power (maxSpeed %f rad/s)", -headPower_, maxHeadSpeed_radPerSec_);
                  HAL::MotorSetPower(MotorID::MOTOR_HEAD, -headPower_);
                }

                if (increasePower_) {
                  // Cycle through different power levels
                  if (!up) {
                    headPower_ += 0.1f;
                    if (headPower_ >=1.01f) {
                      headPower_ = HEAD_POWER_CMD;
                    }
                  }
                }
              }

              ticCnt_ = 0;
            }

            break;
          }
          case HeadTestFlags::HTF_NODDING:
          {
            static Radians theta = 0;
            const f32 lowAngle = MAX_HEAD_ANGLE;
            const f32 highAngle = MIN_HEAD_ANGLE;
            const f32 midAngle = 0.5f * (highAngle + lowAngle);
            const f32 amplitude = highAngle - midAngle;

            const f32 thetaStep = 2*M_PI_F / headNodCycleTime_ms_ * ROBOT_TIME_STEP_MS;

            f32 newAngle = amplitude * sinf(theta.ToFloat()) + midAngle;
            HeadController::SetDesiredAngle(newAngle);
            theta += thetaStep;

            break;
          }
          default:
            AnkiWarn( "TestModeController.HeadTestUpdate", "Unknown head test mode %hhu", headTestMode_);
            break;
        }


        // Print speed
        if (ticCnt2_++ >= printCyclePeriod_) {
          f32 hSpeed = HAL::MotorGetSpeed(MotorID::MOTOR_HEAD);
          f32 hSpeed_filt = HeadController::GetAngularVelocity();
          f32 hPos = HeadController::GetAngleRad();

          if (ABS(hSpeed) > 0 || ABS(hSpeed_filt) > 0) {
            AnkiInfo( "TestModeController.HeadTestUpdate", "Head speed %f rad/s (filt %f rad/s), angle %f rad", hSpeed, hSpeed_filt, hPos);
          }
          ticCnt2_ = 0;

          // To prevent compiler error if AnkiInfo compiled out
          (void)hPos;
        }

        return RESULT_OK;
      }


      Result IMUTestInit(s32 flags)
      {
        AnkiInfo( "TestModeController.IMUTestInit", "");
        imuTestDoTurns_ = flags & EnumToUnderlyingType(IMUTestFlags::ITF_DO_TURNS);
        ticCnt_ = 0;
        IT_turnLeft = false;
        return RESULT_OK;
      }

      Result IMUTestUpdate()
      {
        if (imuTestDoTurns_) {
          if (SteeringController::GetMode() != SteeringController::SM_POINT_TURN) {
            if (IT_turnLeft) {
              // Turn left 180 degrees
              AnkiInfo( "TestModeController.IMUTestUpdate", "Turning to 180");
              SteeringController::ExecutePointTurn(IT_TARGET_ANGLE, IT_MAX_ROT_VEL, IT_ROT_ACCEL, IT_ROT_ACCEL, IT_ROT_TOL, false);
            } else {
              // Turn right 180 degrees
              AnkiInfo( "TestModeController.IMUTestUpdate", "Turning to 0");
              SteeringController::ExecutePointTurn(0.f, -IT_MAX_ROT_VEL, IT_ROT_ACCEL, IT_ROT_ACCEL, IT_ROT_TOL, false);
            }
            IT_turnLeft = !IT_turnLeft;
          }
        }

        // Print gyro readings
        if (++ticCnt_ >= 200 / ROBOT_TIME_STEP_MS) {

          // Raw HAL readings
          HAL::IMU_DataStructure data = IMUFilter::GetLatestRawData();

          AnkiInfo( "TestModeController.IMUTestUpdate", "Gyro (%f,%f,%f) rad/s, (%f,%f,%f) mm/s^2",
                data.rate_x, data.rate_y, data.rate_z,
                data.acc_x, data.acc_y, data.acc_z);


          // IMUFilter readings
          f32 rot_imu = IMUFilter::GetRotation();
          AnkiInfo( "TestModeController.IMUTestUpdate", "Rot(IMU): %f deg",
                RAD_TO_DEG_F32(rot_imu)
                );

          ticCnt_ = 0;

          // To prevent compiler error if AnkiInfo compiled out
          (void)data; 
          (void)rot_imu;
        }

        return RESULT_OK;
      }


      // flags: See LightTestFlags
      // ledID: The LED channel to activate (applies if LTF_CYCLE_ALL not enabled)
      // color: The color to set it to (applies if LTF_CYCLE_ALL not enabled)
      Result LightTestInit(s32 flags, s32 ledID, s32 color)
      {
        AnkiInfo( "TestModeController.LightTestInit", "flags = %x, ledID = %d, color = %x", flags, ledID, color);

        ledCycleTest_ = flags & EnumToUnderlyingType(LightTestFlags::LTF_CYCLE_ALL);

        if (ledCycleTest_) {
          ledID_ = (LEDId)0;
          ledColorIdx_ = 0;
        } else {
          //HAL::SetLED((LEDId)ledID, color);
        }

        ticCnt_ = 0;
        return RESULT_OK;
      }


      Result LightTestUpdate()
      {
        if (!ledCycleTest_) {
          Reset();
          return RESULT_OK;
        }

        // Cycle through all channels
        if (ticCnt_++ > 2000 / ROBOT_TIME_STEP_MS) {
          AnkiInfo( "TestModeController.LightTestUpdate", "LED channel %hhu, color 0x%x", ledID_, LEDColorList_[ledColorIdx_]);
          //HAL::SetLED(ledID_, LEDColorList_[ledColorIdx_]);
          (void)LEDColorList_; // Prevent compiler error. Don't need this when SetLED call restored.

          // Increment led
          ledID_ = (LEDId)((u8)ledID_+1);
          if (ledID_ == LEDId::NUM_BACKPACK_LEDS) {
            ledID_ = (LEDId)0;

            // Increment color
            if (++ledColorIdx_ == LED_COLOR_LIST_SIZE) {
              ledColorIdx_ = 0;
            }
          }

          ticCnt_ = 0;
        }

        return RESULT_OK;
      }


      Result StopTestInit(s32 slowSpeed_mmps, s32 fastSpeed_mmps, s32 period_ms)
      {
        AnkiInfo( "TestModeController.StopTestInit", "");
        ticCnt_ = 0;
        ST_go = false;
        ST_speed = 0.f;
        ST_slowingDown = false;
        
        ST_slowSpeed_mmps = slowSpeed_mmps;
        ST_fastSpeed_mmps = fastSpeed_mmps;
        ST_period_ms = period_ms;
        
        return RESULT_OK;
      }


      Result StopTestUpdate()
      {
        if (ticCnt_++ > ST_period_ms / ROBOT_TIME_STEP_MS) {
          ST_go = !ST_go;
          if(ST_go) {
            // Toggle speed
            if (ST_speed == ST_fastSpeed_mmps)
              ST_speed = ST_slowSpeed_mmps;
            else
              ST_speed = ST_fastSpeed_mmps;
            
            AnkiInfo( "TestModeController.StopTestUpdate", "GO: %f mm/s", ST_speed);
            SteeringController::ExecuteDirectDrive(ST_speed, ST_speed);
          } else {
            // Stopping
            SteeringController::ExecuteDirectDrive(0, 0);
            HAL::MotorResetPosition(MotorID::MOTOR_LEFT_WHEEL);
            HAL::MotorResetPosition(MotorID::MOTOR_RIGHT_WHEEL);
            ST_prevLeftPos = 1000;
            ST_prevRightPos = 1000;
            ST_slowDownTics = 0;
            ST_slowingDown = true;
          }
          ticCnt_ = 0;
        }

        // Report stopping distance once it has come to a complete stop
        if(ST_slowingDown) {
          if (HAL::MotorGetPosition(MotorID::MOTOR_LEFT_WHEEL) == ST_prevLeftPos &&
              HAL::MotorGetPosition(MotorID::MOTOR_RIGHT_WHEEL) == ST_prevRightPos) {
            AnkiInfo( "TestModeController.StopTestUpdate", "STOPPED: (%f, %f) mm in %d tics", ST_prevLeftPos, ST_prevRightPos, ST_slowDownTics);
            ST_slowingDown = false;
          }
          ST_prevLeftPos = HAL::MotorGetPosition(MotorID::MOTOR_LEFT_WHEEL);
          ST_prevRightPos = HAL::MotorGetPosition(MotorID::MOTOR_RIGHT_WHEEL);
          ST_slowDownTics++;
        }

        return RESULT_OK;
      }

      Result MaxPowerTestInit()
      {
        AnkiInfo( "TestModeController.MaxPowerTestInit", "");
        ticCnt_ = 0;

        // Disable all controllers so that they can be overridden with HAL power commands
        WheelController::Disable();
        LiftController::Disable();

        return RESULT_OK;
      }


      Result MaxPowerTestUpdate()
      {
        static f32 pwr = 1.0;
        if (ticCnt_++ > 5000 / ROBOT_TIME_STEP_MS) {
          AnkiInfo( "TestModeController.MaxPowerTestUpdate", "SWITCHING POWER: %f", pwr);
          HAL::MotorSetPower(MotorID::MOTOR_LEFT_WHEEL, 1.0);
          HAL::MotorSetPower(MotorID::MOTOR_RIGHT_WHEEL, 1.0);
          //HAL::MotorSetPower(MotorID::MOTOR_LIFT, pwr);
          //HAL::MotorSetPower(MotorID::MOTOR_HEAD, pwr);
          pwr *= -1;
          ticCnt_ = 0;
        }
        return RESULT_OK;
      }

      Result Start(const TestMode mode, s32 p1, s32 p2, s32 p3)
      {
        Result ret = RESULT_OK;
        
        testMode_ = mode;

        switch(testMode_) {
          case TestMode::TM_NONE:
            ret = Reset();
            updateFunc = NULL;
            break;
          case TestMode::TM_PLACE_BLOCK_ON_GROUND:
            ret = PlaceOnGroundTestInit(p1,p2,p3);
            updateFunc = PlaceOnGroundTestUpdate;
            break;
          case TestMode::TM_DOCK_PATH:
            ret = DockPathTestInit();
            updateFunc = DockPathTestUpdate;
            break;
          case TestMode::TM_PATH_FOLLOW:
            ret = PathFollowTestInit();
            updateFunc = PathFollowTestUpdate;
            break;
          case TestMode::TM_PATH_FOLLOW_CONVENIENCE_FUNCTIONS:
            ret = PathFollowConvenienceFuncTestInit();
            updateFunc = PathFollowConvenienceFuncTestUpdate;
            break;
          case TestMode::TM_DIRECT_DRIVE:
            ret = DriveTestInit(p1,p2,p3);
            updateFunc = DriveTestUpdate;
            break;
          case TestMode::TM_LIFT:
            ret = LiftTestInit(p1,p2,p3);
            updateFunc = LiftTestUpdate;
            break;
          case TestMode::TM_LIFT_TOGGLE:
            ret = LiftToggleTestInit();
            updateFunc = LiftToggleTestUpdate;
            break;
          case TestMode::TM_HEAD:
            ret = HeadTestInit(p1,p2,p3);
            updateFunc = HeadTestUpdate;
            break;
          case TestMode::TM_IMU:
            ret = IMUTestInit(p1);
            updateFunc = IMUTestUpdate;
            break;
          case TestMode::TM_LIGHTS:
            ret = LightTestInit(p1,p2,p3);
            updateFunc = LightTestUpdate;
            break;
          case TestMode::TM_STOP_TEST:
            ret = StopTestInit(p1,p2,p3);
            updateFunc = StopTestUpdate;
            break;
          case TestMode::TM_MAX_POWER_TEST:
            ret = MaxPowerTestInit();
            updateFunc = MaxPowerTestUpdate;
            break;
          default:
            AnkiWarn( "TestModeController.Start", "Undefined test mode %hhu\n", testMode_);
            Reset();
            ret = RESULT_FAIL;
            break;
        }

        return ret;
      }


      Result Update()
      {
        // Don't run Update until robot is finished initializing
        if (HeadController::IsCalibrated() &&
            LiftController::IsCalibrated() &&
            IMUFilter::IsBiasFilterComplete()) {
          if (updateFunc) {
            return updateFunc();
          }
        }
        return RESULT_OK;
      }      

    } // namespace TestModeController
  } // namespace Vector
} // namespace Anki
