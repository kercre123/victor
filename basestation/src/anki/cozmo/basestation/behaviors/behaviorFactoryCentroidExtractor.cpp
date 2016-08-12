/**
 * File: behaviorFactoryCentroidExtractor.cpp
 *
 * Author: Al Chaussee
 * Date:   06/20/2016
 *
 * Description: Logs the four centroid positions and calculated camera pose
 *              using the pattern on the standalone factory test fixture
 *
 *              Failures are indicated by backpack lights
 *                Red: Failure to find centroids
 *                Magenta: Failure to move head to zero degrees
 *                Orange: Failure to compute camera pose
 *                Blue: One or more of the camPose thresholds was exceeded
 *                Green: Passed
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviors/behaviorFactoryCentroidExtractor.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotToEngineImplMessaging.h"

// This define comments out code that depends on messages that don't exist outside of factory branch.
// This skips a variety of version checks so it should never be used on the line!
#define IS_FACTORY_BRANCH 0


namespace Anki {
namespace Cozmo {

  // Backpack lights
  static const size_t NUM_LIGHTS = (size_t)LEDId::NUM_BACKPACK_LEDS;
  static const std::array<u32,NUM_LIGHTS> pass_onColor{{NamedColors::BLACK,NamedColors::GREEN,NamedColors::GREEN,NamedColors::GREEN,NamedColors::BLACK}};
  static const std::array<u32,NUM_LIGHTS> pass_offColor{{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}};
  static const std::array<u32,NUM_LIGHTS> pass_onPeriod_ms{{1000,1000,1000,1000,1000}};
  static const std::array<u32,NUM_LIGHTS> pass_offPeriod_ms{{100,100,100,100,100}};
  static const std::array<u32,NUM_LIGHTS> pass_transitionOnPeriod_ms{{450,450,450,450,450}};
  static const std::array<u32,NUM_LIGHTS> pass_transitionOffPeriod_ms{{450,450,450,450,450}};
  
  static const std::array<u32,NUM_LIGHTS> fail_onColorRed{{NamedColors::BLACK,NamedColors::RED,NamedColors::RED,NamedColors::RED,NamedColors::BLACK}};
  static const std::array<u32,NUM_LIGHTS> fail_onColorOrange{{NamedColors::BLACK,NamedColors::ORANGE,NamedColors::ORANGE,NamedColors::ORANGE,NamedColors::BLACK}};
  static const std::array<u32,NUM_LIGHTS> fail_onColorMagenta{{NamedColors::BLACK,NamedColors::MAGENTA,NamedColors::MAGENTA,NamedColors::MAGENTA,NamedColors::BLACK}};
  static const std::array<u32,NUM_LIGHTS> fail_onColorBlue{{NamedColors::BLACK,NamedColors::BLUE,NamedColors::BLUE,NamedColors::BLUE,NamedColors::BLACK}};
  static const std::array<u32,NUM_LIGHTS> fail_offColor{{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}};
  static const std::array<u32,NUM_LIGHTS> fail_onPeriod_ms{{500,500,500,500,500}};
  static const std::array<u32,NUM_LIGHTS> fail_offPeriod_ms{{500,500,500,500,500}};
  static const std::array<u32,NUM_LIGHTS> fail_transitionOnPeriod_ms{};
  static const std::array<u32,NUM_LIGHTS> fail_transitionOffPeriod_ms{};

  BehaviorFactoryCentroidExtractor::BehaviorFactoryCentroidExtractor(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  {
    SetDefaultName("FactoryCentroidExtractor");
    
    SubscribeToTags({{
      EngineToGameTag::MotorCalibration,
      EngineToGameTag::RobotCompletedFactoryDotTest
    }});
  }
  
  bool BehaviorFactoryCentroidExtractor::IsRunnableInternal(const Robot& robot) const
  {
    return !IsActing() && !_waitingForDots;
  }

  Result BehaviorFactoryCentroidExtractor::InitInternal(Robot& robot)
  {
    std::stringstream serialNumString;
    serialNumString << std::hex << robot.GetSerialNumber();
    _factoryTestLogger.StartLog(serialNumString.str() + "_centroids", true, robot.GetContextDataPlatform());
    PRINT_NAMED_INFO("BehaviorFactoryCentroidExtractor.WillLogToDevice",
                     "Log name: %s",
                     _factoryTestLogger.GetLogName().c_str());
    
    _waitingForDots = false;
    _headCalibrated = false;
    _liftCalibrated = false;
    
    robot.GetActionList().Cancel();
    
    // Mute volume
    auto audioClient = robot.GetRobotAudioClient();
    if (audioClient) {
      audioClient->SetRobotVolume(0);
    }
    
    // Disable reactionary behaviors
    robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::EnableReactionaryBehaviors>(false);
    
#if IS_FACTORY_BRANCH
    // Set robot body to accessory mode
    robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::SetBodyRadioMode(RobotInterface::BodyRadioMode::BODY_ACCESSORY_OPERATING_MODE)));
    
    // Set head device lock so that video will stream down
    robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::SetHeadDeviceLock(true)));
#endif
    
    // Start motor calibration
    robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::StartMotorCalibration(true, true)));
    
    return Result::RESULT_OK;
  }
  
  IBehavior::Status BehaviorFactoryCentroidExtractor::UpdateInternal(Robot& robot)
  {
    if(_waitingForDots || !_liftCalibrated || !_headCalibrated)
    {
      return Status::Running;
    }
    
    return IBehavior::UpdateInternal(robot);
  }
  
  void BehaviorFactoryCentroidExtractor::StopInternal(Robot& robot)
  {
    _waitingForDots = false;
    _liftCalibrated = false;
    _headCalibrated = false;
  }
  
  void BehaviorFactoryCentroidExtractor::TransitionToMovingHead(Robot& robot)
  {
    MoveHeadToAngleAction* action = new MoveHeadToAngleAction(robot, 0);
    StartActing(action, [this, &robot](ActionResult res)
                {
                  if(res == ActionResult::SUCCESS)
                  {
                    _waitingForDots = true;
                    robot.GetVisionComponent().UseNextImageForFactoryDotTest();
                  }
                  else
                  {
                    PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.MoveHead", "Moving head to 0 degrees failed");
                    robot.SetBackpackLights(fail_onColorMagenta, fail_offColor,
                                            fail_onPeriod_ms, fail_offPeriod_ms,
                                            fail_transitionOnPeriod_ms, fail_transitionOffPeriod_ms);
                  }
                });
  }
  
  void BehaviorFactoryCentroidExtractor::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
  {
    if(event.GetData().GetTag() == EngineToGameTag::RobotCompletedFactoryDotTest)
    {
      const auto& msg = event.GetData().Get_RobotCompletedFactoryDotTest();
      
      if(!msg.success)
      {
        PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.DotTestFailed",
                            "Failed to find all 4 dots");
        robot.SetBackpackLights(fail_onColorRed, fail_offColor,
                                fail_onPeriod_ms, fail_offPeriod_ms,
                                fail_transitionOnPeriod_ms, fail_transitionOffPeriod_ms);
      }
      else
      {
        if(!msg.didComputePose)
        {
          PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.DidNotComputeCameraPose",
                              "Failed to compute camPose camera is not calibrated");
          robot.SetBackpackLights(fail_onColorOrange, fail_offColor,
                                  fail_onPeriod_ms, fail_offPeriod_ms,
                                  fail_transitionOnPeriod_ms, fail_transitionOffPeriod_ms);
        }
        else
        {
          static const bool doThresholdCheck = false;
          static const f32 rollThresh_rad = DEG_TO_RAD_F32(5);
          static const f32 pitchThresh_rad = DEG_TO_RAD_F32(5);
          static const f32 yawThresh_rad = DEG_TO_RAD_F32(5);
          static const f32 xThresh_mm = 5;
          static const f32 yThresh_mm = 5;
          static const f32 zThresh_mm = 5;
          
          bool exceedsThresh = false;
          if(!NEAR(msg.camPoseRoll_rad, 0, rollThresh_rad))
          {
            PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.CamPose", "Roll exceeds threshold");
            exceedsThresh = true;
          }
          if(!NEAR(msg.camPosePitch_rad - msg.headAngle, DEG_TO_RAD_F32(-4), pitchThresh_rad))
          {
            PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.CamPose", "Pitch exceeds threshold");
            exceedsThresh = true;
          }
          if(!NEAR(msg.camPoseYaw_rad, 0, yawThresh_rad))
          {
            PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.CamPose", "Yaw exceeds threshold");
            exceedsThresh = true;
          }
          if(!NEAR(msg.camPoseX_mm, 0, xThresh_mm))
          {
            PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.CamPose", "xTrans exceeds threshold");
            exceedsThresh = true;
          }
          if(!NEAR(msg.camPoseY_mm, 0, yThresh_mm))
          {
            PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.CamPose", "yTrans exceeds threshold");
            exceedsThresh = true;
          }
          if(!NEAR(msg.camPoseZ_mm, 0, zThresh_mm))
          {
            PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.CamPose", "zTrans exceeds threshold");
            exceedsThresh = true;
          }
        
          if(doThresholdCheck && exceedsThresh)
          {
            robot.SetBackpackLights(fail_onColorBlue, fail_offColor,
                                    fail_onPeriod_ms, fail_offPeriod_ms,
                                    fail_transitionOnPeriod_ms, fail_transitionOffPeriod_ms);
          }
          else
          {
            robot.SetBackpackLights(pass_onColor, pass_offColor,
                                    pass_onPeriod_ms, pass_offPeriod_ms,
                                    pass_transitionOnPeriod_ms, pass_transitionOffPeriod_ms);
          }
        }
      }
      _factoryTestLogger.Append(msg);
    }
    else if(event.GetData().GetTag() == EngineToGameTag::MotorCalibration)
    {
      const auto& msg = event.GetData().Get_MotorCalibration();
      
      if(!msg.calibStarted)
      {
        if(msg.motorID == MotorID::MOTOR_HEAD)
        {
          _headCalibrated = true;
          if(_liftCalibrated)
          {
            TransitionToMovingHead(robot);
          }
        }
        if(msg.motorID == MotorID::MOTOR_LIFT)
        {
          _liftCalibrated = true;
          if(_headCalibrated)
          {
            TransitionToMovingHead(robot);
          }
        }
      }
    }
    else
    {
      PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.HandleWhileRunning",
                          "Received event with tag %hhu not handling", event.GetData().GetTag());
    }
  }
}
}