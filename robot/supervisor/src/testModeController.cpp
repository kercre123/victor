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
#include "visionSystem.h"
#include "animationController.h"

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
        #define TOGGLE_DIRECTION_TEST 1   // 0: Only drive forward
                                          // 1: Switch between driving forward and reverse
        
        #define DIRECT_HAL_MOTOR_TEST 0   // 0: Test WheelController
                                          // 1: Test direct drive via HAL::MotorSetPower
        
        const u32 WHEEL_TOGGLE_DIRECTION_PERIOD_MS = 2000;
        
        // Acceleration
        const f32 accel_mmps2 = 0;  // 0 for infinite acceleration.
                                    // Only works in WheelController mode (i.e. DIRECT_HAL_MOTOR_TEST == 0)
        
        // Measurements taken from a prototype cozmo.
        // Used to model open loop motor command in WheelController.
        //
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
#if DIRECT_HAL_MOTOR_TEST
        f32 wheelPower_ = 0;
        const f32 WHEEL_POWER_CMD = 0.2;
#endif
        const f32 WHEEL_SPEED_CMD_MMPS = 60;
        ////// End of DriveTest ////////
        
        
        /////// LiftTest /////////
        // 0: Set power directly with MotorSetPower
        // 1: Command a desired lift height (i.e. use LiftController)
        #define LIFT_HEIGHT_TEST 1
        
        // Open-loop lift speeds (unloaded)
        //
        // Power   UpSpeed (rad/s)  DownSpeed (rad/s)
        // 0.2     0.3              0.5
        // 0.3     0.7              0.9
        // 0.4     1.1              1.3
        // 0.5     1.5              1.7
        // 0.6     2.0              2.2
        // 0.7     2.4              2.6
        // 0.8     2.8              3.0
        // 0.9     3.2              3.4
        // 1.0     3.6              3.8
        
        f32 liftPower_ = 1;
        const f32 LIFT_POWER_CMD = 0.2f;
        const f32 LIFT_DES_HIGH_HEIGHT = LIFT_HEIGHT_CARRY - 10;
        const f32 LIFT_DES_LOW_HEIGHT = LIFT_HEIGHT_LOWDOCK + 10;
        //// End of LiftTest  //////
        
        
        //////// HeadTest /////////
        // 0: Set power directly with MotorSetPower
        // 1: Command a desired head angle (i.e. use HeadController)
        #define HEAD_POSITION_TEST 1
        
        f32 headPower_ = 0;
        const f32 HEAD_POWER_CMD = 0.2;
        const f32 HEAD_DES_HIGH_ANGLE = MAX_HEAD_ANGLE;
        const f32 HEAD_DES_LOW_ANGLE = MIN_HEAD_ANGLE;
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
        
        ////// End of PickAndPlaceTest ////
        
        
        //////// StopTest /////////
        bool ST_go;
        f32 ST_speed;
        bool ST_slowingDown;
        f32 ST_prevLeftPos, ST_prevRightPos;
        u16 ST_slowDownTics;
        ///// End of StopTest /////

        
        ///////// IMUTest ////////
        bool IT_turnLeft;
        const f32 IT_TARGET_ANGLE = 3.14;
        const f32 IT_MAX_ROT_VEL = 1.5f;
        const f32 IT_ROT_ACCEL = 10.f;
        ///// End of IMUTest /////
        
        ///////// AnimationTest ////////
        AnimationID_t AT_currAnim;
        const u32 AT_periodTics = 2000;
        
        
        /////// LightTest ////////
        LEDId ledID = (LEDId)0;
        u8 ledColorIdx = 0;
        const u8 LED_COLOR_LIST_SIZE = 3;
        const LEDColor LEDColorList[LED_COLOR_LIST_SIZE] = {LED_RED, LED_GREEN, LED_BLUE};
        ///// End of LightTest ///
        
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
        
        return RESULT_OK;
      }
      
      
      Result PickAndPlaceTestInit()
      {
        PRINT("\n==== Starting PickAndPlaceTest =====\n");
        ticCnt_ = 0;
        pickAndPlaceState_ = PAP_WAITING_FOR_PICKUP_BLOCK;
        return RESULT_OK;
      }
      
      Result PickAndPlaceTestUpdate()
      {
        switch(pickAndPlaceState_)
        {
          case PAP_WAITING_FOR_PICKUP_BLOCK:
          {
            PRINT("PAPT: Docking to block %d\n", BLOCK_TO_PICK_UP);
            PickAndPlaceController::DockToBlock(BLOCK_TO_PICK_UP, Vision::MARKER_UNKNOWN, BLOCK_MARKER_WIDTH, PICKUP_ACTION);
            pickAndPlaceState_ = PAP_DOCKING;
            break;
          }
          case PAP_DOCKING:
            if (!PickAndPlaceController::IsBusy()) {
              if (PickAndPlaceController::DidLastActionSucceed()) {
                if (PICKUP_ACTION == DA_PICKUP_LOW) {
                  PRINT("PAPT: Placing on other block %d\n", BLOCK_TO_PLACE_ON);
                  PickAndPlaceController::DockToBlock(BLOCK_TO_PLACE_ON, Vision::MARKER_UNKNOWN, BLOCK_MARKER_WIDTH, DA_PLACE_HIGH);
                } else {
                  PRINT("PAPT: Placing on ground\n");
                  PickAndPlaceController::PlaceOnGround(PLACE_ON_GROUND_DIST_X, PLACE_ON_GROUND_DIST_Y, PLACE_ON_GROUND_DIST_ANG);
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
                  PickAndPlaceController::DockToBlock(BLOCK_TO_PLACE_ON, Vision::MARKER_UNKNOWN, BLOCK_MARKER_WIDTH, DA_PLACE_HIGH);
                  //pickAndPlaceState_ = PAP_PLACING;
                } else {
                  PickAndPlaceController::PlaceOnGround(PLACE_ON_GROUND_DIST_X, PLACE_ON_GROUND_DIST_Y, PLACE_ON_GROUND_DIST_ANG);
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
        Localization::SetCurrentMatPose(0, 0, Radians(-PIDIV2_F));
        return RESULT_OK;
      }
      
      Result PathFollowTestUpdate()
      {
        const u32 startDriveTime_us = 500000;
        if (!pathStarted_ && HAL::GetMicroCounter() > startDriveTime_us) {
          
          // Create a path and follow it
          PathFollower::ClearPath();
#if(PATH_FOLLOW_ALIGNED_START)
          Localization::SetCurrentMatPose(0, 0, PIDIV2_F * (PATH_FOLLOW_BACKWARDS ? 1 : -1));
          
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
      
      
      Result DriveTestInit()
      {
        PRINT("\n=== Starting DirectDriveTest ===\n");
        #if DIRECT_HAL_MOTOR_TEST
        wheelPower_ = WHEEL_POWER_CMD;
        #endif
        ticCnt_ = 0;
        return RESULT_OK;
      }
      
      Result DriveTestUpdate()
      {
        static bool fwd = false;
        static bool firstSpeedCommanded = false;

        // Change direction (or at least print speed
        if (ticCnt_++ >= WHEEL_TOGGLE_DIRECTION_PERIOD_MS / TIME_STEP) {

          f32 lSpeed = HAL::MotorGetSpeed(HAL::MOTOR_LEFT_WHEEL);
          f32 rSpeed = HAL::MotorGetSpeed(HAL::MOTOR_RIGHT_WHEEL);
          
          f32 lSpeed_filt, rSpeed_filt;
          WheelController::GetFilteredWheelSpeeds(lSpeed_filt,rSpeed_filt);


          if (firstSpeedCommanded){
#if(DIRECT_HAL_MOTOR_TEST)
            PRINT("Going forward %f power (currSpeed %f %f, filtSpeed %f %f)\n", WHEEL_POWER_CMD, lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
#else
            PRINT("Going forward %f mm/s (currSpeed %f %f, filtSpeed %f %f)\n", WHEEL_SPEED_CMD_MMPS, lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
#endif
            ticCnt_ = 0;
            return RESULT_OK;
          }
          
          
          
          fwd = !fwd;
          if (fwd) {
#if(DIRECT_HAL_MOTOR_TEST)
            PRINT("Going forward %f power (currSpeed %f %f, filtSpeed %f %f)\n", wheelPower_, lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
            HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, wheelPower_);
            HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, wheelPower_);
            WheelController::Disable();
#else
            PRINT("Going forward %f mm/s (currSpeed %f %f, filtSpeed %f %f)\n", WHEEL_SPEED_CMD_MMPS, lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
            SteeringController::ExecuteDirectDrive(WHEEL_SPEED_CMD_MMPS,WHEEL_SPEED_CMD_MMPS,accel_mmps2,accel_mmps2);
#endif
          } else {
#if(DIRECT_HAL_MOTOR_TEST)
            PRINT("Going reverse %f power (currSpeed %f %f, filtSpeed %f %f)\n", wheelPower_, lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
            HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, -wheelPower_);
            HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, -wheelPower_);
            WheelController::Disable();
#else
            PRINT("Going reverse %f mm/s (currSpeed %f %f, filtSpeed %f %f)\n", WHEEL_SPEED_CMD_MMPS, lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
            SteeringController::ExecuteDirectDrive(-WHEEL_SPEED_CMD_MMPS,-WHEEL_SPEED_CMD_MMPS,accel_mmps2,accel_mmps2);
#endif
            
          }
          ticCnt_ = 0;
          
#if(TOGGLE_DIRECTION_TEST == 0)
          firstSpeedCommanded = true;
#endif

          #if(DIRECT_HAL_MOTOR_TEST)
          // Cycle through different power levels
          if (!fwd) {
            wheelPower_ += 0.1;
            if (wheelPower_ >=1.01f) {
              wheelPower_ = WHEEL_POWER_CMD;
            }
          }
          #endif
          
       }

       return RESULT_OK;
      }
      
      
      Result LiftTestInit()
      {
        PRINT("\n==== Starting LiftTest =====\n");
        PRINT("!!! REMOVE JTAG CABLE !!!\n");
        ticCnt_ = 0;
        ticCnt2_ = 0;
        liftPower_ = LIFT_POWER_CMD;
#if(!LIFT_HEIGHT_TEST)
        LiftController::Disable();
#endif
        return RESULT_OK;
      }
      
      
      Result LiftTestUpdate()
      {
        static bool up = false;
        
        // Change direction
        if (ticCnt_++ >= 3000 / TIME_STEP) {
          

#if(LIFT_HEIGHT_TEST)
          up = !up;
          if (up) {
            PRINT("Lift HIGH %f mm\n", LIFT_DES_HIGH_HEIGHT);
            LiftController::SetDesiredHeight(LIFT_DES_HIGH_HEIGHT);
          } else {
            PRINT("Lift LOW %f mm\n", LIFT_DES_LOW_HEIGHT);
            LiftController::SetDesiredHeight(LIFT_DES_LOW_HEIGHT);
          }
          
#else
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
            liftPower_ += 0.1;
            if (liftPower_ >=1.01f) {
              liftPower_ = LIFT_POWER_CMD;
            }
          }

#endif
          
          ticCnt_ = 0;
        }
        

        // Print speed
        if (ticCnt2_++ >= 40) {
          f32 lSpeed = HAL::MotorGetSpeed(HAL::MOTOR_LIFT);
          f32 lSpeed_filt = LiftController::GetAngularVelocity();
          f32 lPos = LiftController::GetAngleRad(); // HAL::MotorGetPosition(HAL::MOTOR_LIFT);
          f32 lHeight = LiftController::GetHeightMM();
          PRINT("Lift speed %f rad/s, filt_speed %f rad/s, position %f rad, %f mm\n", lSpeed, lSpeed_filt, lPos, lHeight);
          ticCnt2_ = 0;
        }

        
        return RESULT_OK;
      }
      
      Result LiftToggleTestInit()
      {
        PRINT("\n==== Starting LiftToggleTest =====\n");
        PRINT("!!! REMOVE JTAG CABLE !!!\n");
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
        if (ticCnt2_++ >= 200 / TIME_STEP) {
          f32 lSpeed = HAL::MotorGetSpeed(HAL::MOTOR_LIFT);
          f32 lPos = LiftController::GetHeightMM();
          
          PRINT("Lift speed %f rad/s, height %f mm\n", lSpeed, lPos);
          ticCnt2_ = 0;
        }


        
        return RESULT_OK;
      }
      
      
      Result HeadTestInit()
      {
        PRINT("\n==== Starting HeadTest =====\n");
        ticCnt_ = 0;
        ticCnt2_ = 0;
        headPower_ = HEAD_POWER_CMD;
#if(!HEAD_POSITION_TEST)
        HeadController::Disable();
#endif
        return RESULT_OK;
      }
      
      
      Result HeadTestUpdate()
      {
        static bool up = false;
        
        // Change direction
        if (ticCnt_++ >= 3000 / TIME_STEP) {
          
          
#if(HEAD_POSITION_TEST)
          up = !up;
          if (up) {
            PRINT("Head HIGH %f rad\n", HEAD_DES_HIGH_ANGLE);
            HeadController::SetDesiredAngle(HEAD_DES_HIGH_ANGLE);
          } else {
            PRINT("Head LOW %f rad\n", HEAD_DES_LOW_ANGLE);
            HeadController::SetDesiredAngle(HEAD_DES_LOW_ANGLE);
          }
#else
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

#endif
          
          ticCnt_ = 0;
        }
        
        // Print speed
        if (ticCnt2_++ >= 5) {
          f32 hSpeed = HAL::MotorGetSpeed(HAL::MOTOR_HEAD);
          f32 hPos = HeadController::GetAngleRad();
          
          PRINT("Head speed %f rad/s, angle %f rad\n", hSpeed, hPos);
          ticCnt2_ = 0;
        }
        
        
        return RESULT_OK;
      }
      
      Result IMUTestInit()
      {
        PRINT("\n==== Starting IMUTest =====\n");
        ticCnt_ = 0;
        IT_turnLeft = false;
        return RESULT_OK;
      }
      
      Result IMUTestUpdate()
      {
        
        if (SteeringController::GetMode() != SteeringController::SM_POINT_TURN) {
          if (IT_turnLeft) {
            // Turn left 180 degrees
            PRINT("Turning to 180\n");
            SteeringController::ExecutePointTurn(IT_TARGET_ANGLE, IT_MAX_ROT_VEL, IT_ROT_ACCEL, IT_ROT_ACCEL);
          } else {
            // Turn right 180 degrees
            PRINT("Turning to 0\n");
            SteeringController::ExecutePointTurn(0.f, -IT_MAX_ROT_VEL, IT_ROT_ACCEL, IT_ROT_ACCEL);
          }
          IT_turnLeft = !IT_turnLeft;
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
      
      Result LightTestInit()
      {
        PRINT("\n==== Starting LightTest =====\n");
        ticCnt_ = 0;
        ledID = (LEDId)0;
        ledColorIdx = 0;
        return RESULT_OK;
      }
      
      
      Result LightTestUpdate()
      {
        if (ticCnt_++ > 2000 / TIME_STEP) {
          
          PRINT("LED channel %d, color 0x%x\n", ledID, LEDColorList[ledColorIdx]);
          HAL::SetLED(ledID, LEDColorList[ledColorIdx]);
          
          // Increment led
          ledID = (LEDId)((u8)ledID+1);
          if (ledID == NUM_LEDS) {
            ledID = (LEDId)0;
            
            // Increment color
            if (++ledColorIdx == LED_COLOR_LIST_SIZE) {
              ledColorIdx = 0;
            }
          }
          
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
      
      
      Result FaceTrackTestInit()
      {
        HeadController::SetDesiredAngle(0.1f);
        return FaceTrackingController::StartTracking(FaceTrackingController::CENTERED, 500);
      }
      
      Result FaceTrackTestUpdate()
      {
        return RESULT_OK;
      }
      
      
      Result Start(const TestMode mode)
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
            ret = DriveTestInit();
            updateFunc = DriveTestUpdate;
            break;
          case TM_LIFT:
            ret = LiftTestInit();
            updateFunc = LiftTestUpdate;
            break;
          case TM_LIFT_TOGGLE:
            ret = LiftToggleTestInit();
            updateFunc = LiftToggleTestUpdate;
            break;
          case TM_HEAD:
            ret = HeadTestInit();
            updateFunc = HeadTestUpdate;
            break;
          case TM_IMU:
            ret = IMUTestInit();
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
            ret = LightTestInit();
            updateFunc = LightTestUpdate;
            break;
          case TM_STOP_TEST:
            ret = StopTestInit();
            updateFunc = StopTestUpdate;
            break;
          case TM_MAX_POWER_TEST:
            ret = MaxPowerTestInit();
            updateFunc = MaxPowerTestUpdate;
            break;
          case TM_FACE_TRACKING:
            ret = FaceTrackTestInit();
            updateFunc = FaceTrackTestUpdate;
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
