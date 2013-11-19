#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
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
        
        ////// DriveTest defines /////////
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
        // Power   LSpeed  RSpeed     (approx,In-air speeds)
        // 1.0     290     300
        // 0.9     255     275
        // 0.8     233     248
        // 0.7     185     200
        // 0.6     160     175
        // 0.5     120     140
        // 0.4     90      112
        // 0.3     55      80
        // 0.25    40      65
        const f32 WHEEL_POWER_CMD = 0.4;
        const f32 WHEEL_SPEED_CMD_MMPS = 100;
        ////// End of DriveTest defines ////////
        
        
        /////// LiftTest defines /////////
        // 0: Set power directly with MotorSetPower
        // 1: Command a desired lift height (i.e. use LiftController)
        #define LIFT_HEIGHT_TEST 1
        
        const f32 LIFT_POWER_CMD = 0.3;
        const f32 LIFT_DES_HIGH_HEIGHT = 50.f;
        const f32 LIFT_DES_LOW_HEIGHT = 22.f;
        //// End of LiftTest defines //////
        
        
        /////// HeadTest defines /////////
        // 0: Set power directly with MotorSetPower
        // 1: Command a desired head angle (i.e. use HeadController)
        #define HEAD_POSITION_TEST 0
        
        const f32 HEAD_POWER_CMD = 0.3;
        const f32 HEAD_DES_HIGH_ANGLE = 0.5f;
        const f32 HEAD_DES_LOW_ANGLE = -0.2f;
        //// End of HeadTest defines //////
        

        // Current test mode
        TestMode testMode_ = TM_NONE;
        
        // Pointer to update function for current test mode
        ReturnCode (*updateFunc)() = NULL;
        
      } // private namespace
      
      
      
      // Commands a path and executes it
      ReturnCode PathFollowTestInit()
      {
        PRINT("=== Starting Path Follow test mode ===\n");
        return EXIT_SUCCESS;
      }
      
      ReturnCode PathFollowTestUpdate()
      {
        const u32 startDriveTime_us = 500000;
        static bool testPathStarted = false;
        if (not PathFollower::IsTraversingPath() &&
            HAL::GetMicroCounter() > startDriveTime_us &&
            !testPathStarted) {
          SpeedController::SetUserCommandedAcceleration( MAX(ONE_OVER_CONTROL_DT + 1, 500) );  // This can't be smaller than 1/CONTROL_DT!
          SpeedController::SetUserCommandedDesiredVehicleSpeed(160);
          PRINT("Speed commanded: %d mm/s\n",
                SpeedController::GetUserCommandedDesiredVehicleSpeed() );
          
          // Create a path and follow it
          PathFollower::AppendPathSegment_Line(0, 0.0, 0.0, 0.3, -0.3);
          
          //PathFollower::AppendPathSegment_PointTurn(0, -PIDIV2-0.05, PIDIV2, PIDIV2, PIDIV2);
          
          float arc1_radius = sqrt(0.005);  // Radius of sqrt(0.05^2 + 0.05^2)
          PathFollower::AppendPathSegment_Arc(0, 0.35, -0.25, arc1_radius, -0.75*PI, 0);
          PathFollower::AppendPathSegment_Line(0, 0.35 + arc1_radius, -0.25, 0.35 + arc1_radius, 0.2);
          float arc2_radius = sqrt(0.02); // Radius of sqrt(0.1^2 + 0.1^2)
          PathFollower::AppendPathSegment_Arc(0, 0.35 + arc1_radius - arc2_radius, 0.2, arc2_radius, 0, PIDIV2);
          PathFollower::StartPathTraversal();
          testPathStarted = true;
        }
        
        return EXIT_SUCCESS;
      }
      
      
      
      ReturnCode DriveTestInit()
      {
        PRINT("=== Starting Direct drive test mode ===\n");
        return EXIT_SUCCESS;
      }
      
      ReturnCode DriveTestUpdate()
      {
        static bool fwd = false;
        static bool firstSpeedCommanded = false;
        static u32 cnt = 0;

        // Change direction (or at least print speed
        if (cnt++ >= WHEEL_TOGGLE_DIRECTION_PERIOD_MS / TIME_STEP) {

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
            cnt = 0;
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
          cnt = 0;
          
#if(TOGGLE_DIRECTION_TEST == 0)
          firstSpeedCommanded = true;
#endif
       }

       return EXIT_SUCCESS;
      }
      
      
      ReturnCode LiftTestInit()
      {
#if(!LIFT_HEIGHT_TEST)
        LiftController::Disable();
#endif
        return EXIT_SUCCESS;
      }
      
      
      ReturnCode LiftTestUpdate()
      {
        static bool up = false;
        static u32 cnt = 0, printCnt = 0;
        
        // Change direction
        if (cnt++ >= 4000 / TIME_STEP) {
          

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
          
          cnt = 0;
        }
        
        // Print speed
        if (printCnt++ >= 200 / TIME_STEP) {
          f32 lSpeed = HAL::MotorGetSpeed(HAL::MOTOR_LIFT);
          f32 lSpeed_filt = LiftController::GetAngularVelocity();
          f32 lPos = LiftController::GetAngleRad(); // HAL::MotorGetPosition(HAL::MOTOR_LIFT);
          f32 lHeight = LiftController::GetHeightMM();
          
          //f32 lSpeed_filt;
          //          WheelController::GetFilteredWheelSpeeds(&lSpeed_filt,&rSpeed_filt);
          //PRINT("Lift speed %f rad/s, filt_speed %f rad/s, position %f rad, %f mm\n", lSpeed, lSpeed_filt, lPos, lHeight);
          printCnt = 0;
        }
        
        
        return EXIT_SUCCESS;
      }
      
      
      ReturnCode HeadTestInit()
      {
#if(!HEAD_POSITION_TEST)
        HeadController::Disable();
#endif
        return EXIT_SUCCESS;
      }
      
      
      ReturnCode HeadTestUpdate()
      {
        static bool up = false;
        static u32 cnt = 0, printCnt = 0;
        
        // Change direction
        if (cnt++ >= 4000 / TIME_STEP) {
          
          
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
          
          cnt = 0;
        }
        
        // Print speed
        if (printCnt++ >= 200 / TIME_STEP) {
          f32 hSpeed = HAL::MotorGetSpeed(HAL::MOTOR_HEAD);
          f32 hPos = HeadController::GetAngleRad();
          
          PRINT("Head speed %f rad/s, angle %f rad\n", hSpeed, hPos);
          printCnt = 0;
        }
        
        
        return EXIT_SUCCESS;
      }
      
      ReturnCode GripperTestInit()
      {
        return EXIT_SUCCESS;
      }
      
      
      ReturnCode GripperTestUpdate()
      {
        static bool up = false;
        static u32 cnt = 0;
        
        // Change direction
        if (cnt++ >= 2000 / TIME_STEP) {
          
          up = !up;
          if (up) {
            PRINT("Gripper ENGAGED\n");
            GripController::EngageGripper();
          } else {
            PRINT("Gripper DISENGAGED\n");
            GripController::DisengageGripper();
          }
          
          cnt = 0;
        }
        
        return EXIT_SUCCESS;
      }
      
      ReturnCode MaxPowerTestInit()
      {
        // Disable all controllers so that they can be overriden with HAL power commands
        WheelController::Disable();
        LiftController::Disable();
        
        return EXIT_SUCCESS;
      }
      
      
      ReturnCode MaxPowerTestUpdate()
      {
        static u16 cnt = 0;
        static f32 pwr = 1.0;
        if (cnt++ > 5000 / TIME_STEP) {
          PRINT("SWITCHING POWER: %f\n", pwr);
          HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, 1.0);
          HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, 1.0);
          //HAL::MotorSetPower(HAL::MOTOR_LIFT, pwr);
          //HAL::MotorSetPower(HAL::MOTOR_HEAD, pwr);
          //HAL::MotorSetPower(HAL::MOTOR_GRIP, pwr);
          pwr *= -1;
          cnt = 0;
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
          case TM_HEAD:
            ret = HeadTestInit();
            updateFunc = HeadTestUpdate;
            break;
          case TM_GRIPPER:
            ret = GripperTestInit();
            updateFunc = GripperTestUpdate;
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
        if (updateFunc) {
          return updateFunc();
        }
        return EXIT_SUCCESS;
      }
      
      
    } // namespace TestModeController
  } // namespace Cozmo
} // namespace Anki
