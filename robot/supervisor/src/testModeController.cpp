#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "pickAndPlaceController.h"
#include "dockingController.h"
#include "gripController.h"
#include "headController.h"
#include "liftController.h"
#include "testModeController.h"
#include "anki/cozmo/robot/debug.h"
#include "anki/cozmo/robot/localization.h"
#include "anki/cozmo/robot/pathFollower.h"
#include "anki/cozmo/robot/speedController.h"
#include "anki/cozmo/robot/steeringController.h"
#include "anki/cozmo/robot/wheelController.h"

namespace Anki {
  namespace Cozmo {
    namespace TestModeController {
      
      // "Private Member Variables"
      namespace {
        
        // Some common vars that can be used across multiple tests
        u32 ticCnt_, ticCnt2_;   // Multi-purpose tic counters
        u8 dir_;                 // Multi-purpose direction flag
        bool pathStarted_;       // Flag for whether or not we started to traverse a path
        
        
        //////// DriveTest /////////
        #define TOGGLE_DIRECTION_TEST 0   // 0: Only drive forward
                                          // 1: Switch between driving forward and reverse
        
        #define DIRECT_HAL_MOTOR_TEST 0   // 0: Test WheelController
                                          // 1: Test direct drive via HAL::MotorSetPower
        
        const u32 WHEEL_TOGGLE_DIRECTION_PERIOD_MS = 2000;
        
        // Acceleration
        const f32 accel_mmps2 = 0;  // 0 for infinite acceleration.
                                    // Only works in WheelController mode (i.e. DIRECT_HAL_MOTOR_TEST == 0)
        
        // COAST MODE
        // Measurements taken from a prototype cozmo.
        // Used to model open loop motor command in WheelController.
        //
        // Power   LSpeed  RSpeed,  2x reduc (diff robot): LSpeed RSpeed  (approx,In-air speeds)
        // 1.0     290     300    .......................  175    155
        // 0.9     255     275
        // 0.8     233     248    .......................
        // 0.7     185     200
        // 0.6     160     175    .......................  96     80
        // 0.5     120     140
        // 0.4     90      112    .......................  60     38
        // 0.3     55      80     .......................  40     20
        // 0.25    40      65
        const f32 WHEEL_POWER_CMD = 0.4;
        const f32 WHEEL_SPEED_CMD_MMPS = 100;
        ////// End of DriveTest ////////
        
        
        /////// LiftTest /////////
        // 0: Set power directly with MotorSetPower
        // 1: Command a desired lift height (i.e. use LiftController)
        #define LIFT_HEIGHT_TEST 1
        
        const f32 LIFT_POWER_CMD = 0.3;
        const f32 LIFT_DES_HIGH_HEIGHT = 50.f;
        const f32 LIFT_DES_LOW_HEIGHT = 22.f;
        //// End of LiftTest  //////
        
        
        //////// HeadTest /////////
        // 0: Set power directly with MotorSetPower
        // 1: Command a desired head angle (i.e. use HeadController)
        #define HEAD_POSITION_TEST 0
        
        const f32 HEAD_POWER_CMD = 0.3;
        const f32 HEAD_DES_HIGH_ANGLE = 0.5f;
        const f32 HEAD_DES_LOW_ANGLE = -0.2f;
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

        
        //////// PickAndPlaceTest /////////
        enum {
          PAP_WAITING_FOR_PICKUP_BLOCK,
          PAP_WAITING_FOR_PLACEMENT_BLOCK,
          PAP_DOCKING,
          PAP_PLACING
        };
        u8 pickAndPlaceState_ = PAP_WAITING_FOR_PICKUP_BLOCK;
        
        const u16 BLOCK_TO_PICK_UP = 60;
        const u16 BLOCK_TO_PLACE_ON = 50;
        ////// End of PickAndPlaceTest ////
        
        
        //////// StopTest /////////
        bool ST_go;
        f32 ST_speed;
        bool ST_slowingDown;
        f32 ST_prevLeftPos, ST_prevRightPos;
        u16 ST_slowDownTics;
        ///// End of StopTest /////
        
        
        
        // Current test mode
        TestMode testMode_ = TM_NONE;
        
        // Pointer to update function for current test mode
        ReturnCode (*updateFunc)() = NULL;
        
      } // private namespace
      

      TestMode GetMode()
      {
        return testMode_;
      }
      

      ReturnCode PickAndPlaceTestInit()
      {
        PRINT("\n==== Starting PickAndPlaceTest =====\n");
        ticCnt_ = 0;
        return EXIT_SUCCESS;
      }
      
      ReturnCode PickAndPlaceTestUpdate()
      {
        switch(pickAndPlaceState_)
        {
          case PAP_WAITING_FOR_PICKUP_BLOCK:
            PickAndPlaceController::PickUpBlock(BLOCK_TO_PICK_UP, 0);
            pickAndPlaceState_ = PAP_DOCKING;
            break;
          case PAP_DOCKING:
            if (!PickAndPlaceController::IsBusy()) {
              if (PickAndPlaceController::DidLastActionSucceed()) {
                PickAndPlaceController::PlaceOnBlock(BLOCK_TO_PLACE_ON, 0, 0);
                pickAndPlaceState_ = PAP_PLACING;
              } else {
                pickAndPlaceState_ = PAP_WAITING_FOR_PICKUP_BLOCK;
              }
            }
            break;
          case PAP_PLACING:
            if (!PickAndPlaceController::IsBusy()) {
              if (PickAndPlaceController::DidLastActionSucceed()) {
                pickAndPlaceState_ = PAP_WAITING_FOR_PICKUP_BLOCK;
              } else {
                PickAndPlaceController::PlaceOnBlock(BLOCK_TO_PLACE_ON, 0, 0);
                pickAndPlaceState_ = PAP_PLACING;
              }
            }
            break;
          default:
            PRINT("WTF?\n");
            break;
        }
        return EXIT_SUCCESS;
      }

      
      
      
      // Commands a path and executes it
      ReturnCode DockPathTestInit()
      {
        PRINT("\n==== Starting DockPathTest =====\n");
        ticCnt_ = 0;
        dockPathState_ = DT_STOP;
        return EXIT_SUCCESS;
      }
      
      ReturnCode DockPathTestUpdate()
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
        
        return EXIT_SUCCESS;
      }

      
      
      // Commands a path and executes it
      ReturnCode PathFollowTestInit()
      {
        PRINT("\n=== Starting PathFollowTest ===\n");
        pathStarted_ = false;
        return EXIT_SUCCESS;
      }
      
      ReturnCode PathFollowTestUpdate()
      {
        const u32 startDriveTime_us = 500000;
        if (!pathStarted_ && HAL::GetMicroCounter() > startDriveTime_us) {
          SpeedController::SetUserCommandedAcceleration( MAX(ONE_OVER_CONTROL_DT + 1, 500) );  // This can't be smaller than 1/CONTROL_DT!
          SpeedController::SetUserCommandedDesiredVehicleSpeed(160);
          PRINT("Speed commanded: %d mm/s\n",
                SpeedController::GetUserCommandedDesiredVehicleSpeed() );
          
          // Create a path and follow it
          PathFollower::AppendPathSegment_Line(0, 0.0, 0.0, 0.3, -0.3);
          
          //PathFollower::AppendPathSegment_PointTurn(0, 0.3, -0.3, -PIDIV2-0.05, PIDIV2, PIDIV2, PIDIV2);
          
          float arc1_radius = sqrt(0.005);  // Radius of sqrt(0.05^2 + 0.05^2)
          PathFollower::AppendPathSegment_Arc(0, 0.35, -0.25, arc1_radius, -0.75*PI, 0.75*PI);
          PathFollower::AppendPathSegment_Line(0, 0.35 + arc1_radius, -0.25, 0.35 + arc1_radius, 0.2);
          float arc2_radius = sqrt(0.02); // Radius of sqrt(0.1^2 + 0.1^2)
          //PathFollower::AppendPathSegment_Arc(0, 0.35 + arc1_radius - arc2_radius, 0.2, arc2_radius, 0, PIDIV2);
          PathFollower::AppendPathSegment_Arc(0, 0.35 + arc1_radius - arc2_radius, 0.2, arc2_radius, 0, 3*PIDIV2);
          PathFollower::AppendPathSegment_Arc(0, 0.35 + arc1_radius - arc2_radius, 0.2 - 2*arc2_radius, arc2_radius, PIDIV2, -3.5*PIDIV2);
          PathFollower::StartPathTraversal();
          pathStarted_ = true;
        }
        
        return EXIT_SUCCESS;
      }
      
      
      
      ReturnCode DriveTestInit()
      {
        PRINT("\n=== Starting DirectDriveTest ===\n");
        ticCnt_ = 0;
        return EXIT_SUCCESS;
      }
      
      ReturnCode DriveTestUpdate()
      {
        static bool fwd = false;
        static bool firstSpeedCommanded = false;

        // Change direction (or at least print speed
        if (ticCnt_++ >= WHEEL_TOGGLE_DIRECTION_PERIOD_MS / TIME_STEP) {

          f32 lSpeed = HAL::MotorGetSpeed(HAL::MOTOR_LEFT_WHEEL);
          f32 rSpeed = HAL::MotorGetSpeed(HAL::MOTOR_RIGHT_WHEEL);
          
          f32 lSpeed_filt, rSpeed_filt;
          WheelController::GetFilteredWheelSpeeds(&lSpeed_filt,&rSpeed_filt);


          if (firstSpeedCommanded){
#if(DIRECT_HAL_MOTOR_TEST)
            PRINT("Going forward %f power (currSpeed %f %f, filtSpeed %f %f)\n", WHEEL_POWER_CMD, lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
#else
            PRINT("Going forward %f mm/s (currSpeed %f %f, filtSpeed %f %f)\n", WHEEL_SPEED_CMD_MMPS, lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
#endif
            ticCnt_ = 0;
            return EXIT_SUCCESS;
          }
          
          
          
          fwd = !fwd;
          if (fwd) {
#if(DIRECT_HAL_MOTOR_TEST)
            PRINT("Going forward %f power (currSpeed %f %f, filtSpeed %f %f)\n", WHEEL_POWER_CMD, lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
            HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, WHEEL_POWER_CMD);
            HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, WHEEL_POWER_CMD);
            WheelController::Disable();
#else
            PRINT("Going forward %f mm/s (currSpeed %f %f, filtSpeed %f %f)\n", WHEEL_SPEED_CMD_MMPS, lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
            SteeringController::ExecuteDirectDrive(WHEEL_SPEED_CMD_MMPS,WHEEL_SPEED_CMD_MMPS,accel_mmps2,accel_mmps2);
#endif
          } else {
#if(DIRECT_HAL_MOTOR_TEST)
            PRINT("Going reverse %f power (currSpeed %f %f, filtSpeed %f %f)\n", WHEEL_POWER_CMD, lSpeed, rSpeed, lSpeed_filt, rSpeed_filt);
            HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, -WHEEL_POWER_CMD);
            HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, -WHEEL_POWER_CMD);
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
       }

       return EXIT_SUCCESS;
      }
      
      
      ReturnCode LiftTestInit()
      {
        PRINT("\n==== Starting LiftTest =====\n");
        ticCnt_ = 0;
        ticCnt2_ = 0;
        dir_ = 0;
#if(!LIFT_HEIGHT_TEST)
        LiftController::Disable();
#endif
        return EXIT_SUCCESS;
      }
      
      
      ReturnCode LiftTestUpdate()
      {
        static bool up = false;
        
        // Change direction
        if (ticCnt_++ >= 4000 / TIME_STEP) {
          

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
            PRINT("Lift UP %f power\n", LIFT_POWER_CMD);
            HAL::MotorSetPower(HAL::MOTOR_LIFT, LIFT_POWER_CMD);
          } else {
            PRINT("Lift DOWN %f power\n", -LIFT_POWER_CMD);
            HAL::MotorSetPower(HAL::MOTOR_LIFT, -LIFT_POWER_CMD);
          }
#endif
          
          ticCnt_ = 0;
        }
        
        // Print speed
        if (ticCnt2_++ >= 200 / TIME_STEP) {
          // TODO: Unused. Remove?
          /*
          f32 lSpeed = HAL::MotorGetSpeed(HAL::MOTOR_LIFT);
          f32 lSpeed_filt = LiftController::GetAngularVelocity();
          f32 lPos = LiftController::GetAngleRad(); // HAL::MotorGetPosition(HAL::MOTOR_LIFT);
          f32 lHeight = LiftController::GetHeightMM();
          */
          //f32 lSpeed_filt;
          //          WheelController::GetFilteredWheelSpeeds(&lSpeed_filt,&rSpeed_filt);
          //PRINT("Lift speed %f rad/s, filt_speed %f rad/s, position %f rad, %f mm\n", lSpeed, lSpeed_filt, lPos, lHeight);
          ticCnt2_ = 0;
        }
        
        
        return EXIT_SUCCESS;
      }
      
      ReturnCode LiftToggleTestInit()
      {
        PRINT("\n==== Starting LiftToggleTest =====\n");
        return EXIT_SUCCESS;
      }
      
      
      ReturnCode LiftToggleTestUpdate()
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
        
        return EXIT_SUCCESS;
      }
      
      
      ReturnCode HeadTestInit()
      {
        PRINT("\n==== Starting HeadTest =====\n");
        ticCnt_ = 0;
        ticCnt2_ = 0;
#if(!HEAD_POSITION_TEST)
        HeadController::Disable();
#endif
        return EXIT_SUCCESS;
      }
      
      
      ReturnCode HeadTestUpdate()
      {
        static bool up = false;
        
        // Change direction
        if (ticCnt_++ >= 4000 / TIME_STEP) {
          
          
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
            PRINT("Head UP %f power\n", HEAD_POWER_CMD);
            HAL::MotorSetPower(HAL::MOTOR_HEAD, HEAD_POWER_CMD);
          } else {
            PRINT("Head DOWN %f power\n", -HEAD_POWER_CMD);
            HAL::MotorSetPower(HAL::MOTOR_HEAD, -HEAD_POWER_CMD);
          }
#endif
          
          ticCnt_ = 0;
        }
        
        // Print speed
        if (ticCnt2_++ >= 200 / TIME_STEP) {
          f32 hSpeed = HAL::MotorGetSpeed(HAL::MOTOR_HEAD);
          f32 hPos = HeadController::GetAngleRad();
          
          PRINT("Head speed %f rad/s, angle %f rad\n", hSpeed, hPos);
          ticCnt2_ = 0;
        }
        
        
        return EXIT_SUCCESS;
      }
      
      ReturnCode GripperTestInit()
      {
        PRINT("\n==== Starting GripperTest =====\n");
        ticCnt_ = 0;
        return EXIT_SUCCESS;
      }
      
      
      ReturnCode GripperTestUpdate()
      {
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
        
        return EXIT_SUCCESS;
      }
      
      ReturnCode StopTestInit()
      {
        PRINT("\n==== Starting StopTest =====\n");
        ticCnt_ = 0;
        ST_go = false;
        ST_speed = 0.f;
        ST_slowingDown = false;
        return EXIT_SUCCESS;
      }
      
      
      ReturnCode StopTestUpdate()
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
        
        return EXIT_SUCCESS;
      }

      ReturnCode MaxPowerTestInit()
      {
        ticCnt_ = 0;
        
        // Disable all controllers so that they can be overriden with HAL power commands
        WheelController::Disable();
        LiftController::Disable();
        
        return EXIT_SUCCESS;
      }
      
      
      ReturnCode MaxPowerTestUpdate()
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
        return EXIT_SUCCESS;
      }
      
      ReturnCode Init(TestMode mode)
      {
        ReturnCode ret = EXIT_SUCCESS;
        testMode_ = mode;
        
        switch(testMode_) {
          case TM_NONE:
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
          case TM_GRIPPER:
            ret = GripperTestInit();
            updateFunc = GripperTestUpdate;
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
            ret = EXIT_FAILURE;
            break;
        }
        
        return ret;
        
      }
      
      
      ReturnCode Update()
      {
        // Don't run Update until robot is finished initializing
        if (Robot::GetOperationMode() != Robot::INITIALIZING) {
          if (updateFunc) {
            return updateFunc();
          }
        }
        return EXIT_SUCCESS;
      }
      
      
    } // namespace TestModeController
  } // namespace Cozmo
} // namespace Anki
