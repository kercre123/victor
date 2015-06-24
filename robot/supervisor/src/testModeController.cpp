#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/debug.h"
#include "pickAndPlaceController.h"
#include "dockingController.h"
#include "faceTrackingController.h"
#include "gripController.h"
#include "headController.h"
#include "liftController.h"
#include "imuFilter.h"
#include "testModeController.h"
#include "localization.h"
#include "messages.h"
#include "pathFollower.h"
#include "speedController.h"
#include "steeringController.h"
#include "wheelController.h"
#include "animationController.h"
#include "backpackLightController.h"

#include "anki/common/robot/trig_fast.h"

namespace Anki {
  namespace Cozmo {
    namespace TestModeController {
      
      // "Private Member Variables"
      namespace {
        
        // Some common vars that can be used across multiple tests
        u32 ticCnt_, ticCnt2_;   // Multi-purpose tic counters
        bool pathStarted_;       // Flag for whether or not we started to traverse a path
        
        
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
        
        f32 liftPower_ = 1;
        const f32 LIFT_POWER_CMD = 0.2f;
        const f32 LIFT_DES_HIGH_HEIGHT = LIFT_HEIGHT_CARRY - 10;
        const f32 LIFT_DES_LOW_HEIGHT = LIFT_HEIGHT_LOWDOCK + 10;
        LiftTestFlags liftTestMode_ = LiftTF_TEST_POWER;
        s32 liftNodCycleTime_ms_ = 2000;
        //// End of LiftTest  //////
        
        
        //////// HeadTest /////////
        // 0: Set power directly with MotorSetPower
        // 1: Command a desired head angle (i.e. use HeadController)
        #define HEAD_POSITION_TEST 1
        
        f32 headPower_ = 0;
        const f32 HEAD_POWER_CMD = 0.2;
        const f32 HEAD_DES_HIGH_ANGLE = MAX_HEAD_ANGLE;
        const f32 HEAD_DES_LOW_ANGLE = MIN_HEAD_ANGLE;
        HeadTestFlags headTestMode_ = HTF_TEST_POWER;
        s32 headNodCycleTime_ms_ = 2000;
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
        
        
        //////// PickAndPlaceTest /////////
        enum {
          PAP_WAITING_FOR_PICKUP_BLOCK,
          PAP_WAITING_FOR_PLACEMENT_BLOCK,
          PAP_DOCKING,
          PAP_PLACING
        };
        u8 pickAndPlaceState_ = PAP_WAITING_FOR_PICKUP_BLOCK;
        
        // The block to pick up
        const Vision::MarkerType BLOCK_TO_PICK_UP = Vision::MARKER_FIRE;
        
        // The block to place the picked up block on
        //const Vision::MarkerType BLOCK_TO_PLACE_ON = Vision::MARKER_ANGRYFACE;
        const Vision::MarkerType BLOCK_TO_PLACE_ON = Vision::MARKER_SQUAREPLUSCORNERS;
        
        // The width of the marker
        const f32 BLOCK_MARKER_WIDTH = DEFAULT_BLOCK_MARKER_WIDTH_MM;
        
        // This pick up action depends on whether the block is expected to be on
        // the ground or on top of another block.
        // If DA_PICKUP_HIGH, the place part of this test will just the block on the ground.
        const DockAction_t PICKUP_ACTION = DA_PICKUP_HIGH;
        
        // Relative pose of desired block placement on ground
        const f32 PLACE_ON_GROUND_DIST_X = 100;
        const f32 PLACE_ON_GROUND_DIST_Y = -10;
        const f32 PLACE_ON_GROUND_DIST_ANG = 0;
        
        // Set to true if you want to manually specify the speed at which to dock to block
        bool useManualSpeed_ = false;
        ////// End of PickAndPlaceTest ////
        
        
        //////// StopTest /////////
        bool ST_go;
        f32 ST_speed;
        bool ST_slowingDown;
        f32 ST_prevLeftPos, ST_prevRightPos;
        u16 ST_slowDownTics;
        ///// End of StopTest /////

        
        ///////// IMUTest ////////
        bool imuTestDoTurns_ = false;
        bool IT_turnLeft;
        const f32 IT_TARGET_ANGLE = 3.14;
        const f32 IT_MAX_ROT_VEL = 1.5f;
        const f32 IT_ROT_ACCEL = 10.f;
        ///// End of IMUTest /////
        
        ///////// AnimationTest ////////
        AnimationID_t AT_currAnim;
        const u32 AT_periodTics = 2000;
        
        
        /////// LightTest ////////
        LEDId ledID_ = (LEDId)0;
        u8 ledColorIdx_ = 0;
        const u8 LED_COLOR_LIST_SIZE = 4;
        const LEDColor LEDColorList_[LED_COLOR_LIST_SIZE] = {LED_RED, LED_GREEN, LED_BLUE, LED_OFF};
        
        bool ledCycleTest_ = true;
        ///// End of LightTest ///
        
        
        /////// FaceDisplayTest ////////
        u8 faceFrame_[180];
        s32 facePosX_ = 0;
        s32 facePosY_ = 0;
        ///// End of FaceDisplayTest ///
        
        
        // The number of cycles in between printouts
        // in those tests that print something out.
        u32 printCyclePeriod_;
        
        // Current test mode
        TestMode testMode_ = TM_NONE;
        
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
        PRINT("TestMode reset\n");
        
        // Stop animations that might be playing
        AnimationController::Stop();
        
        // Stop wheels and vision system
        WheelController::Enable();
        PickAndPlaceController::Reset();
        
        // Stop lift and head
        LiftController::Enable();
        LiftController::SetAngularVelocity(0);
        HeadController::Enable();
        HeadController::SetAngularVelocity(0);
        
        // Backpack lights
        BackpackLightController::Enable();
        
        return RESULT_OK;
      }
      
      
      Result PickAndPlaceTestInit()
      {
        /*
        PRINT("\n==== Starting PickAndPlaceTest =====\n");
        ticCnt_ = 0;
        pickAndPlaceState_ = PAP_WAITING_FOR_PICKUP_BLOCK;
        return RESULT_OK;
         */
        
        PRINT("\n==== PickAndPlaceTest DISABLED =====\n");
        Reset();
        return RESULT_OK;
      }
      
      Result PickAndPlaceTestUpdate()
      {
        switch(pickAndPlaceState_)
        {
          case PAP_WAITING_FOR_PICKUP_BLOCK:
          {
            PRINT("PAPT: Docking to block %d\n", BLOCK_TO_PICK_UP);
            //PickAndPlaceController::DockToBlock(BLOCK_TO_PICK_UP, Vision::MARKER_UNKNOWN, BLOCK_MARKER_WIDTH, useManualSpeed_, PICKUP_ACTION);
            pickAndPlaceState_ = PAP_DOCKING;
            break;
          }
          case PAP_DOCKING:
            if (!PickAndPlaceController::IsBusy()) {
              if (PickAndPlaceController::DidLastActionSucceed()) {
                if (PICKUP_ACTION == DA_PICKUP_LOW) {
                  PRINT("PAPT: Placing on other block %d\n", BLOCK_TO_PLACE_ON);
                  //PickAndPlaceController::DockToBlock(BLOCK_TO_PLACE_ON, Vision::MARKER_UNKNOWN, BLOCK_MARKER_WIDTH, useManualSpeed_, DA_PLACE_HIGH);
                } else {
                  PRINT("PAPT: Placing on ground\n");
                  PickAndPlaceController::PlaceOnGround(PLACE_ON_GROUND_DIST_X, PLACE_ON_GROUND_DIST_Y, PLACE_ON_GROUND_DIST_ANG, false);
                }
                pickAndPlaceState_ = PAP_PLACING;
              } else {
                pickAndPlaceState_ = PAP_WAITING_FOR_PICKUP_BLOCK;
              }
            }
            break;
          case PAP_PLACING:
            if (!PickAndPlaceController::IsBusy()) {
              if (PickAndPlaceController::DidLastActionSucceed()) {
                PRINT("PAPT: Success\n");
                pickAndPlaceState_ = PAP_WAITING_FOR_PICKUP_BLOCK;
              } else {
                if (PICKUP_ACTION == DA_PICKUP_LOW) {
                  //PickAndPlaceController::DockToBlock(BLOCK_TO_PLACE_ON, Vision::MARKER_UNKNOWN, BLOCK_MARKER_WIDTH, useManualSpeed_, DA_PLACE_HIGH);
                  //pickAndPlaceState_ = PAP_PLACING;
                } else {
                  PickAndPlaceController::PlaceOnGround(PLACE_ON_GROUND_DIST_X, PLACE_ON_GROUND_DIST_Y, PLACE_ON_GROUND_DIST_ANG, false);
                }
              }
            }
            break;
          default:
            PRINT("WTF?\n");
            break;
        }
        return RESULT_OK;
      }

      
      
      
      // Commands a path and executes it
      Result DockPathTestInit()
      {
        PRINT("\n==== Starting DockPathTest =====\n");
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
          PRINT("DOCKING TEST STATE: %d\n", dockPathState_);
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
        PRINT("\n=== Starting PathFollowTest ===\n");
        pathStarted_ = false;
        Localization::SetDriveCenterPose(0, 0, Radians(-PIDIV2_F));
        return RESULT_OK;
      }
      
      Result PathFollowTestUpdate()
      {
        const u32 startDriveTime_us = 500000;
        if (!pathStarted_ && HAL::GetMicroCounter() > startDriveTime_us) {

          /*
          // Small circle drive test
          PathFollower::ClearPath();
          Localization::SetDriveCenterPose(0, 0, PIDIV2_F);
          f32 speed = 20;
          f32 radius = WHEEL_DIST_HALF_MM;
          PathFollower::AppendPathSegment_Arc(0, -radius, 0, radius, 0,    PI_F, speed, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          PathFollower::AppendPathSegment_Arc(0, -radius, 0, radius, PI_F, PI_F, speed, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);

          PathFollower::StartPathTraversal();
          pathStarted_ = true;
          return RESULT_OK;
          */
          
          // Create a path and follow it
          PathFollower::ClearPath();
#if(PATH_FOLLOW_ALIGNED_START)
          Localization::SetDriveCenterPose(0, 0, PIDIV2_F * (PATH_FOLLOW_BACKWARDS ? 1 : -1));
          
          //PathFollower::AppendPathSegment_PointTurn(0, 0, 0, -PIDIV2_F, -1.5f, 2.f, 2.f);
          
          float arc1_radius = sqrt((float)5000);  // Radius of sqrt(50^2 + 50^2)
          f32 sweepAng = atan_fast((350-arc1_radius)/250);
          
          PathFollower::AppendPathSegment_Arc(0, arc1_radius, 0, arc1_radius, -PI, sweepAng,
                                              PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          
          f32 firstConnectionPt_x = arc1_radius - arc1_radius*cos(sweepAng);
          f32 firstConnectionPt_y = - arc1_radius*sin(sweepAng);
          f32 secondConnectionPt_x = firstConnectionPt_x + (350 - arc1_radius);
          f32 secondConnectionPt_y = firstConnectionPt_y - 250;
          
          PathFollower::AppendPathSegment_Line(0, firstConnectionPt_x, firstConnectionPt_y, secondConnectionPt_x, secondConnectionPt_y,
                                               PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          
          PathFollower::AppendPathSegment_Arc(0, 350, -250, arc1_radius, -PI + sweepAng, PI - sweepAng,
                                              PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
#else
          PathFollower::AppendPathSegment_Line(0, 0.0, 0.0, 300, -300,
                                               PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          float arc1_radius = sqrt((float)5000);  // Radius of sqrt(50^2 + 50^2)
          PathFollower::AppendPathSegment_Arc(0, 350, -250, arc1_radius, -0.75f*PI, 0.75f*PI,
                                              PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
#endif
          PathFollower::AppendPathSegment_Line(0, 350 + arc1_radius, -250, 350 + arc1_radius, 200,
                                               PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          float arc2_radius = sqrt((float)20000); // Radius of sqrt(100^2 + 100^2)
          //PathFollower::AppendPathSegment_Arc(0, 0.35 + arc1_radius - arc2_radius, 0.2, arc2_radius, 0, PIDIV2);
          PathFollower::AppendPathSegment_Arc(0, 350 + arc1_radius - arc2_radius, 200, arc2_radius, 0, 3*PIDIV2,
                                              PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          PathFollower::AppendPathSegment_Arc(0, 350 + arc1_radius - arc2_radius, 200 - 2*arc2_radius, arc2_radius, PIDIV2, -3*PIDIV2,
                                              PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          
          PathFollower::AppendPathSegment_Line(0, 350 + arc1_radius - 2*arc2_radius, 200 - 2*arc2_radius, 350 + arc1_radius - 2*arc2_radius, 0,
                                               PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          float arc3_radius = 0.5f * (350 + arc1_radius - 2*arc2_radius);
          PathFollower::AppendPathSegment_Arc(0, arc3_radius, 0, arc3_radius, 0, PI_F,
                                              PF_TARGET_SPEED_MMPS, PF_ACCEL_MMPS2, PF_DECEL_MMPS2);
          
          
          PathFollower::StartPathTraversal();
          pathStarted_ = true;
        }
        
        return RESULT_OK;
      }
      

      
      
      Result PathFollowConvenienceFuncTestInit()
      {
        PRINT("\n=== Starting PathFollowConvenienceFuncTest ===\n");
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
              PRINT("TM: DriveStraight\n");
              if (!PathFollower::DriveStraight(100, 0.25, 0.25, 2)) {
                PRINT("TM: DriveStraight failed\n");
                return RESULT_FAIL;
              }
              break;
              
            case PFCF_DRIVE_ARC:
              PRINT("TM: DriveArc\n");
              if (!PathFollower::DriveArc(-PIDIV2_F, 40, 0.25, 0.25, 1)) {
                PRINT("TM: DriveArc failed\n");
                return RESULT_FAIL;
              }

              
              break;
            case PFCF_DRIVE_POINT_TURN:
              PRINT("TM: DrivePointTurn\n");
              if (!PathFollower::DrivePointTurn(-PIDIV2_F, 0.25, 0.25, 1)) {
                PRINT("TM: DrivePointTurn failed\n");
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
        PRINT("\n=== Starting DirectDriveTest (%x, %d, %d) ===\n", flags, powerStepPercent, wheelSpeedOrPower);
        enableDirectHALTest_ = flags & DTF_ENABLE_DIRECT_HAL_TEST;
        enableCycleSpeedsTest_ = flags & DTF_ENABLE_CYCLE_SPEEDS_TEST;
        enableToggleDir_ = flags & DTF_ENABLE_TOGGLE_DIR;
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
        if (ticCnt_++ >= WHEEL_TOGGLE_DIRECTION_PERIOD_MS / TIME_STEP) {

          f32 lSpeed = HAL::MotorGetSpeed(HAL::MOTOR_LEFT_WHEEL);
          f32 rSpeed = HAL::MotorGetSpeed(HAL::MOTOR_RIGHT_WHEEL);
          
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
          
          if (enableDirectHALTest_) {
            PRINT("Going %s %.3f power (currSpeed %.2f %.2f, filtSpeed %.2f %.2f)\n",
                  wheelTargetDir_ > 0 ? "forward" : "reverse",
                  wheelTargetPower_*0.01f,
                  lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
          } else {
            PRINT("Going %s %.3f mm/s (currSpeed %.2f %.2f, filtSpeed %.2f %.2f)\n",
                  wheelTargetDir_ > 0 ? "forward" : "reverse",
                  wheelSpeed_mmps_,
                  lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
          }
          
          ticCnt_ = 0;
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
            HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, 0.01f*wheelPower_);
            HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, 0.01f*wheelPower_);
            //PRINT("WheelPower = %f (increasing)\n", 0.01f*wheelPower_);
          } else if (wheelTargetDir_ < 0 && wheelPower_ > wheelTargetPower_) {
            wheelPower_ -= WHEEL_POWER_PERCENT_ACCEL;
            if (wheelPower_ < wheelTargetPower_) {
              wheelPower_ = wheelTargetPower_;
            }
            HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, 0.01f*wheelPower_);
            HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, 0.01f*wheelPower_);
            //PRINT("WheelPower = %f (decreasing)\n", 0.01f*wheelPower_);
          }
        } else {
          SteeringController::ExecuteDirectDrive(wheelSpeed_mmps_,
                                                 wheelSpeed_mmps_,
                                                 accel_mmps2,accel_mmps2);
        }
        

       return RESULT_OK;
      }
      
      
      Result LiftTestInit(s32 mode, s32 noddingCycleTime_ms, s32 printPeriod_ms)
      {
        PRINT("\n==== Starting LiftTest (flags = %d, printPeriod_ms = %d)=====\n", mode, printPeriod_ms);
        PRINT("!!! REMOVE JTAG CABLE !!!\n");
        ticCnt_ = 0;
        ticCnt2_ = 0;
        printCyclePeriod_ = printPeriod_ms < TIME_STEP ? 10 : printPeriod_ms / TIME_STEP;
        liftPower_ = LIFT_POWER_CMD;
        liftTestMode_ = (LiftTestFlags)mode;
        liftNodCycleTime_ms_ = noddingCycleTime_ms == 0 ? 2000 : noddingCycleTime_ms;
        if (liftTestMode_ == LiftTF_TEST_POWER || liftTestMode_ == LiftTF_DISABLE_MOTOR) {
          LiftController::Disable();
        }
        return RESULT_OK;
      }
      
      
      Result LiftTestUpdate()
      {
        
        switch (liftTestMode_) {
          case LiftTF_NODDING: {
            static Radians theta = 0;
            const f32 lowHeight = LIFT_HEIGHT_LOWDOCK;
            const f32 highHeight = LIFT_HEIGHT_CARRY;
            const f32 midHeight = 0.5f * (highHeight + lowHeight);
            const f32 amplitude = highHeight - midHeight;
            
            const f32 thetaStep = 2*PI_F / liftNodCycleTime_ms_ * TIME_STEP;
            
            f32 newHeight = amplitude * sinf(theta.ToFloat()) + midHeight;
            LiftController::SetDesiredHeight(newHeight);
            theta += thetaStep;
            
            break;
          }
          case LiftTF_TEST_POWER:
          case LiftTF_TEST_HEIGHTS:
          {
            static bool up = false;
            
            // As long as the lift is moving reset the counter
            if (ABS(LiftController::GetAngularVelocity()) > 0.1f) {
              ticCnt_ = 0;
            }
            
            // Change direction
            if (ticCnt_++ >= 1000 / TIME_STEP) {
              
              if (liftTestMode_ == LiftTF_TEST_HEIGHTS) {
                up = !up;
                if (up) {
                  PRINT("Lift HIGH %f mm\n", LIFT_DES_HIGH_HEIGHT);
                  LiftController::SetDesiredHeight(LIFT_DES_HIGH_HEIGHT);
                } else {
                  PRINT("Lift LOW %f mm\n", LIFT_DES_LOW_HEIGHT);
                  LiftController::SetDesiredHeight(LIFT_DES_LOW_HEIGHT);
                }
                
              } else {
                up = !up;
                if (up) {
                  PRINT("Lift UP %f power\n", liftPower_);
                  HAL::MotorSetPower(HAL::MOTOR_LIFT, liftPower_);
                } else {
                  PRINT("Lift DOWN %f power\n", -liftPower_);
                  HAL::MotorSetPower(HAL::MOTOR_LIFT, -liftPower_);
                }
                
                
                // Cycle through different power levels
                if (!up) {
                  liftPower_ += 0.05;
                  if (liftPower_ >=1.01f) {
                    liftPower_ = LIFT_POWER_CMD;
                  }
                }
              }
              
              ticCnt_ = 0;
            }
            break;
          }
          case LiftTF_DISABLE_MOTOR:
            break;
          default:
          {
            PRINT("WARN: Unknown lift test mode %d\n", liftTestMode_);
            Reset();
          }
        }
        
        

        // Print speed
        if (ticCnt2_++ >= printCyclePeriod_) {
          f32 lSpeed = HAL::MotorGetSpeed(HAL::MOTOR_LIFT);
          f32 lSpeed_filt = LiftController::GetAngularVelocity();
          f32 lPos = LiftController::GetAngleRad(); // HAL::MotorGetPosition(HAL::MOTOR_LIFT);
          f32 lHeight = LiftController::GetHeightMM();
          if (ABS(lSpeed) > 0.001 || ABS(lSpeed_filt) > 0.001) {
            PRINT("Lift speed %f rad/s, filt_speed %f rad/s, position %f rad, %f mm\n", lSpeed, lSpeed_filt, lPos, lHeight);
          }
          ticCnt2_ = 0;
        }

        
        return RESULT_OK;
      }
      
      Result LiftToggleTestInit()
      {
        PRINT("\n==== Starting LiftToggleTest =====\n");
        PRINT("!!! REMOVE JTAG CABLE !!!\n");
        printCyclePeriod_ = 200 / TIME_STEP;
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
          
            PRINT("SET LIFT TO %f mm\n", liftHeight);
            LiftController::SetDesiredHeight(liftHeight);
            inPositionCycles = 0;
          }
        }


        // Print height and speed
        if (ticCnt2_++ >= printCyclePeriod_) {
          f32 lSpeed = HAL::MotorGetSpeed(HAL::MOTOR_LIFT);
          f32 lPos = LiftController::GetHeightMM();
          
          PRINT("Lift speed %f rad/s, height %f mm\n", lSpeed, lPos);
          ticCnt2_ = 0;
        }


        
        return RESULT_OK;
      }
      
      
      Result HeadTestInit(s32 mode, s32 noddingCycleTime_ms, s32 printPeriod_ms)
      {
        PRINT("\n==== Starting HeadTest (flags = %d, printPeriod_ms = %d)=====\n", mode, printPeriod_ms);
        ticCnt_ = 0;
        ticCnt2_ = 0;
        headPower_ = HEAD_POWER_CMD;
        printCyclePeriod_ = printPeriod_ms < TIME_STEP ? 5 : printPeriod_ms / TIME_STEP;
        headTestMode_ = (HeadTestFlags)mode;
        headNodCycleTime_ms_ = noddingCycleTime_ms == 0 ? 2000 : noddingCycleTime_ms;
        if (headTestMode_ == HTF_TEST_POWER) {
          HeadController::Disable();
        }
        return RESULT_OK;
      }
      
      
      Result HeadTestUpdate()
      {
        
        switch(headTestMode_) {
          case HTF_TEST_POWER:
          case HTF_TEST_ANGLES:
          {
            static bool up = false;
            
            // Change direction
            if (ticCnt_++ >= 3000 / TIME_STEP) {
              
              if (headTestMode_ == HTF_TEST_ANGLES) {
                up = !up;
                if (up) {
                  PRINT("Head HIGH %f rad\n", HEAD_DES_HIGH_ANGLE);
                  HeadController::SetDesiredAngle(HEAD_DES_HIGH_ANGLE);
                } else {
                  PRINT("Head LOW %f rad\n", HEAD_DES_LOW_ANGLE);
                  HeadController::SetDesiredAngle(HEAD_DES_LOW_ANGLE);
                }
              } else {
                up = !up;
                if (up) {
                  PRINT("Head UP %f power\n", headPower_);
                  HAL::MotorSetPower(HAL::MOTOR_HEAD, headPower_);
                } else {
                  PRINT("Head DOWN %f power\n", -headPower_);
                  HAL::MotorSetPower(HAL::MOTOR_HEAD, -headPower_);
                }
                
                // Cycle through different power levels
                if (!up) {
                  headPower_ += 0.1;
                  if (headPower_ >=1.01f) {
                    headPower_ = HEAD_POWER_CMD;
                  }
                }
              }
              
              ticCnt_ = 0;
            }

            break;
          }
          case HTF_NODDING:
          {
            static Radians theta = 0;
            const f32 lowAngle = MAX_HEAD_ANGLE;
            const f32 highAngle = MIN_HEAD_ANGLE;
            const f32 midAngle = 0.5f * (highAngle + lowAngle);
            const f32 amplitude = highAngle - midAngle;
            
            const f32 thetaStep = 2*PI_F / headNodCycleTime_ms_ * TIME_STEP;
            
            f32 newAngle = amplitude * sinf(theta.ToFloat()) + midAngle;
            HeadController::SetDesiredAngle(newAngle);
            theta += thetaStep;
            
            break;
          }
          default:
            PRINT("WARN: Unknown head test mode %d\n", headTestMode_);
            break;
        }
        
        
        // Print speed
        if (ticCnt2_++ >= printCyclePeriod_) {
          f32 hSpeed = HAL::MotorGetSpeed(HAL::MOTOR_HEAD);
          f32 hSpeed_filt = HeadController::GetAngularVelocity();
          f32 hPos = HeadController::GetAngleRad();
          
          if (ABS(hSpeed) > 0 || ABS(hSpeed_filt) > 0) {
            PRINT("Head speed %f rad/s (filt %f rad/s), angle %f rad\n", hSpeed, hSpeed_filt, hPos);
          }
          ticCnt2_ = 0;
        }
        
        
        return RESULT_OK;
      }
      
      Result IMUTestInit(s32 flags)
      {
        PRINT("\n==== Starting IMUTest =====\n");
        imuTestDoTurns_ = flags & ITF_DO_TURNS;
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
              PRINT("Turning to 180\n");
              SteeringController::ExecutePointTurn(IT_TARGET_ANGLE, IT_MAX_ROT_VEL, IT_ROT_ACCEL, IT_ROT_ACCEL, false);
            } else {
              // Turn right 180 degrees
              PRINT("Turning to 0\n");
              SteeringController::ExecutePointTurn(0.f, -IT_MAX_ROT_VEL, IT_ROT_ACCEL, IT_ROT_ACCEL, false);
            }
            IT_turnLeft = !IT_turnLeft;
          }
        }
        
        // Print gyro readings
        if (++ticCnt_ >= 200 / TIME_STEP) {
          
          // Raw HAL readings
          HAL::IMU_DataStructure data;
          HAL::IMUReadData(data);
          
          PRINT("Gyro (%f,%f,%f) rad/s, (%f,%f,%f) mm/s^2\n",
                data.rate_x, data.rate_y, data.rate_z,
                data.acc_x, data.acc_y, data.acc_z);
          
          
          // IMUFilter readings
          f32 rot_imu = IMUFilter::GetRotation();
          PRINT("Rot(IMU): %f deg\n",
                RAD_TO_DEG_F32(rot_imu)
                );
          
          ticCnt_ = 0;
        }

        return RESULT_OK;
      }
      
      Result AnimTestInit()
      {
        PRINT("\n==== Starting AnimationTest =====\n");
        AT_currAnim = 0;
        AnimationController::Play(AT_currAnim, 0);
        ticCnt_ = 0;
        return RESULT_OK;
      }
      
      Result AnimTestUpdate()
      {
        if (ticCnt_++ > AT_periodTics) {
          ticCnt_ = 0;
          
          AT_currAnim = (AnimationID_t)(AT_currAnim + 1);
          
          // Skip undefined animIDs
          while(!AnimationController::IsDefined(AT_currAnim)) {
            if (AT_currAnim == AnimationController::MAX_CANNED_ANIMATIONS) {
              // Go back to start
              AT_currAnim = 0;
            } else {
              // otherwise just incrememnt
              AT_currAnim = (AnimationID_t)(AT_currAnim + 1);
            }
          }
          
          PRINT("Playing animation %d\n", AT_currAnim);
          AnimationController::Play(AT_currAnim, 0);
        }
        
        return RESULT_OK;
      }
      
      Result GripperTestInit()
      {
        PRINT("\n==== Starting GripperTest =====\n");
        ticCnt_ = 0;
        return RESULT_OK;
      }
      
      
      Result GripperTestUpdate()
      {
#if defined(HAVE_ACTIVE_GRIPPER) && HAVE_ACTIVE_GRIPPER
        static bool up = false;
        
        // Change direction
        if (ticCnt_++ >= 4000 / TIME_STEP) {
          
          up = !up;
          if (up) {
            PRINT("Gripper ENGAGED\n");
            GripController::EngageGripper();
          } else {
            PRINT("Gripper DISENGAGED\n");
            GripController::DisengageGripper();
          }
          
          ticCnt_ = 0;
        }
#endif
        return RESULT_OK;
      }
      
      // flags: See LightTestFlags
      // ledID: The LED channel to activate (applies if LTF_CYCLE_ALL not enabled)
      // color: The color to set it to (applies if LTF_CYCLE_ALL not enabled)
      Result LightTestInit(s32 flags, s32 ledID, s32 color)
      {
        PRINT("\n==== Starting LightTest  (flags = %x, ledID = %d, color = %x) =====\n", flags, ledID, color);
        BackpackLightController::Disable();
        
        ledCycleTest_ = flags & LTF_CYCLE_ALL;

        if (ledCycleTest_) {
          ledID_ = (LEDId)0;
          ledColorIdx_ = 0;
        } else {
          HAL::SetLED((LEDId)ledID, color);
        }
        
        ticCnt_ = 0;
        return RESULT_OK;
      }
      
      
      Result LightTestUpdate()
      {
        if (!ledCycleTest_) {
          Start(TM_NONE);
          return RESULT_OK;
        }
        
        // Cycle through all channels
        if (ticCnt_++ > 2000 / TIME_STEP) {
          
          PRINT("LED channel %d, color 0x%x\n", ledID_, LEDColorList_[ledColorIdx_]);
          HAL::SetLED(ledID_, LEDColorList_[ledColorIdx_]);
          
          // Increment led
          ledID_ = (LEDId)((u8)ledID_+1);
          if (ledID_ == NUM_BACKPACK_LEDS) {
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

      
      Result FaceDisplayTestInit()
      {
        PRINT("\n==== Starting FaceDisplayTest =====\n");
        ticCnt_ = 0;
        
        // Generate a test face
        u32 i = 0;
        u8 numLines = 20;
        while (numLines-- > 0) {
          faceFrame_[i++] = 128;
          faceFrame_[i++] = 0;
        }
        
        numLines = 24;
        faceFrame_[i++] = 29;
        while (numLines-- > 0) {
          faceFrame_[i++] = 25;
          faceFrame_[i++] = 20;
          faceFrame_[i++] = 25;
          faceFrame_[i++] = 58;
        }
        faceFrame_[i++] = 0;
        
        numLines = 20;
        while (numLines-- > 0) {
          faceFrame_[i++] = 128;
          faceFrame_[i++] = 0;
        }

        // Draw face
        HAL::FaceAnimate(faceFrame_);
        
        return RESULT_OK;
      }
      
      Result FaceDisplayTestUpdate()
      {
        if (ticCnt_++ == 200) {
          
          if (facePosX_ == 0) {
            facePosX_ = 10;
            facePosY_ = -10;
          } else if (facePosX_ == 10) {
            facePosX_ = -10;
            facePosY_ = -10;
          } else {
            facePosX_ = 0;
            facePosY_ = 0;
          }
          HAL::FaceMove(facePosX_, facePosY_);
          PRINT("Face move to %d %d\n", facePosX_, facePosY_);
          
        } else if (ticCnt_ > 400) {
          PRINT("Face blink\n");
          HAL::FaceBlink();
          ticCnt_ = 0;
        }
        
        return RESULT_OK;
      }
      
      
      Result StopTestInit()
      {
        PRINT("\n==== Starting StopTest =====\n");
        ticCnt_ = 0;
        ST_go = false;
        ST_speed = 0.f;
        ST_slowingDown = false;
        return RESULT_OK;
      }
      
      
      Result StopTestUpdate()
      {
        if (ticCnt_++ > 2000 / TIME_STEP) {
          ST_go = !ST_go;
          if(ST_go) {
            // Toggle speed
            if (ST_speed == 100.f)
              ST_speed = 20.f;
            else
              ST_speed = 100.f;
            
            PRINT("\nGO: %f mm/s\n", ST_speed);
            SteeringController::ExecuteDirectDrive(ST_speed, ST_speed);
          } else {
            // Stopping
            SteeringController::ExecuteDirectDrive(0, 0);
            HAL::MotorResetPosition(HAL::MOTOR_LEFT_WHEEL);
            HAL::MotorResetPosition(HAL::MOTOR_RIGHT_WHEEL);
            ST_prevLeftPos = 1000;
            ST_prevRightPos = 1000;
            ST_slowDownTics = 0;
            ST_slowingDown = true;
          }
          ticCnt_ = 0;
        }
        
        // Report stopping distance once it has come to a complete stop
        if(ST_slowingDown) {
          if (HAL::MotorGetPosition(HAL::MOTOR_LEFT_WHEEL) == ST_prevLeftPos &&
              HAL::MotorGetPosition(HAL::MOTOR_RIGHT_WHEEL) == ST_prevRightPos) {
            PRINT("STOPPED: (%f, %f) mm in %d tics\n", ST_prevLeftPos, ST_prevRightPos, ST_slowDownTics);
            ST_slowingDown = false;
          }
          ST_prevLeftPos = HAL::MotorGetPosition(HAL::MOTOR_LEFT_WHEEL);
          ST_prevRightPos = HAL::MotorGetPosition(HAL::MOTOR_RIGHT_WHEEL);
          ST_slowDownTics++;
        }
        
        return RESULT_OK;
      }

      Result MaxPowerTestInit()
      {
        ticCnt_ = 0;
        
        // Disable all controllers so that they can be overriden with HAL power commands
        WheelController::Disable();
        LiftController::Disable();
        
        return RESULT_OK;
      }
      
      
      Result MaxPowerTestUpdate()
      {
        static f32 pwr = 1.0;
        if (ticCnt_++ > 5000 / TIME_STEP) {
          PRINT("SWITCHING POWER: %f\n", pwr);
          HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, 1.0);
          HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, 1.0);
          //HAL::MotorSetPower(HAL::MOTOR_LIFT, pwr);
          //HAL::MotorSetPower(HAL::MOTOR_HEAD, pwr);
          //HAL::MotorSetPower(HAL::MOTOR_GRIP, pwr);
          pwr *= -1;
          ticCnt_ = 0;
        }
        return RESULT_OK;
      }
      
/*      
      Result FaceTrackTestInit()
      {
        HeadController::SetDesiredAngle(0.1f);
        return FaceTrackingController::StartTracking(FaceTrackingController::CENTERED, 500);
      }
      
      Result FaceTrackTestUpdate()
      {
        return RESULT_OK;
      }
*/      
      
      Result Start(const TestMode mode, s32 p1, s32 p2, s32 p3)
      {
        Result ret = RESULT_OK;
#if(!FREE_DRIVE_DUBINS_TEST)
        testMode_ = mode;
        
        switch(testMode_) {
          case TM_NONE:
            ret = Reset();
            updateFunc = NULL;
            break;
          case TM_PICK_AND_PLACE:
            ret = PickAndPlaceTestInit();
            updateFunc = PickAndPlaceTestUpdate;
            break;
          case TM_DOCK_PATH:
            ret = DockPathTestInit();
            updateFunc = DockPathTestUpdate;
            break;
          case TM_PATH_FOLLOW:
            ret = PathFollowTestInit();
            updateFunc = PathFollowTestUpdate;
            break;
          case TM_PATH_FOLLOW_CONVENIENCE_FUNCTIONS:
            ret = PathFollowConvenienceFuncTestInit();
            updateFunc = PathFollowConvenienceFuncTestUpdate;
            break;
          case TM_DIRECT_DRIVE:
            ret = DriveTestInit(p1,p2,p3);
            updateFunc = DriveTestUpdate;
            break;
          case TM_LIFT:
            ret = LiftTestInit(p1,p2,p3);
            updateFunc = LiftTestUpdate;
            break;
          case TM_LIFT_TOGGLE:
            ret = LiftToggleTestInit();
            updateFunc = LiftToggleTestUpdate;
            break;
          case TM_HEAD:
            ret = HeadTestInit(p1,p2,p3);
            updateFunc = HeadTestUpdate;
            break;
          case TM_IMU:
            ret = IMUTestInit(p1);
            updateFunc = IMUTestUpdate;
            break;
          case TM_ANIMATION:
            ret = AnimTestInit();
            updateFunc = AnimTestUpdate;
            break;
#if defined(HAVE_ACTIVE_GRIPPER) && HAVE_ACTIVE_GRIPPER
          case TM_GRIPPER:
            ret = GripperTestInit();
            updateFunc = GripperTestUpdate;
            break;
#endif
          case TM_LIGHTS:
            ret = LightTestInit(p1,p2,p3);
            updateFunc = LightTestUpdate;
            break;
          case TM_FACE_DISPLAY:
            ret = FaceDisplayTestInit();
            updateFunc = FaceDisplayTestUpdate;
            break;
          case TM_STOP_TEST:
            ret = StopTestInit();
            updateFunc = StopTestUpdate;
            break;
          case TM_MAX_POWER_TEST:
            ret = MaxPowerTestInit();
            updateFunc = MaxPowerTestUpdate;
            break;
          default:
            PRINT("ERROR (TestModeController): Undefined mode %d\n", testMode_);
            ret = RESULT_FAIL;
            break;
        }
#endif
        return ret;
        
      }
      
      
      Result Update()
      {
#if(!FREE_DRIVE_DUBINS_TEST)
        // Don't run Update until robot is finished initializing
        if (Robot::GetOperationMode() == Robot::WAITING) {
          if (updateFunc) {
            return updateFunc();
          }
        }
#endif
        return RESULT_OK;
      }
      
      
    } // namespace TestModeController
  } // namespace Cozmo
} // namespace Anki
