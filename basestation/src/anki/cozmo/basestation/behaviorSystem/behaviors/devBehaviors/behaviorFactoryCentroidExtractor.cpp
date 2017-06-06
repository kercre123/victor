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

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/behaviorFactoryCentroidExtractor.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "anki/cozmo/basestation/components/bodyLightComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotToEngineImplMessaging.h"

// This define comments out code that depends on messages that don't exist outside of factory branch.
// This skips a variety of version checks so it should never be used on the line!
#define IS_FACTORY_BRANCH 0


namespace Anki {
namespace Cozmo {

namespace{
static const char* kBehaviorTestName = "Factory centroid extractor";
}

  // Backpack lights
  static const BackpackLights passLights = {
    .onColors               = {{NamedColors::BLACK,NamedColors::GREEN,NamedColors::GREEN,NamedColors::GREEN,NamedColors::BLACK}},
    .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
    .onPeriod_ms            = {{1000,1000,1000,1000,1000}},
    .offPeriod_ms           = {{100,100,100,100,100}},
    .transitionOnPeriod_ms  = {{450,450,450,450,450}},
    .transitionOffPeriod_ms = {{450,450,450,450,450}},
    .offset                 = {{0,0,0,0,0}}
  };
  
  static BackpackLights failLights = {
    .onColors               = {{NamedColors::BLACK,NamedColors::RED,NamedColors::RED,NamedColors::RED,NamedColors::BLACK}},
    .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
    .onPeriod_ms            = {{500,500,500,500,500}},
    .offPeriod_ms           = {{500,500,500,500,500}},
    .transitionOnPeriod_ms  = {{0,0,0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0,0,0}},
    .offset                 = {{0,0,0,0,0}}
  };
  
  static const BackpackLEDArray fail_onColorRed{{NamedColors::BLACK,NamedColors::RED,NamedColors::RED,NamedColors::RED,NamedColors::BLACK}};
  static const BackpackLEDArray fail_onColorOrange{{NamedColors::BLACK,NamedColors::ORANGE,NamedColors::ORANGE,NamedColors::ORANGE,NamedColors::BLACK}};
  static const BackpackLEDArray fail_onColorMagenta{{NamedColors::BLACK,NamedColors::MAGENTA,NamedColors::MAGENTA,NamedColors::MAGENTA,NamedColors::BLACK}};
  static const BackpackLEDArray fail_onColorBlue{{NamedColors::BLACK,NamedColors::BLUE,NamedColors::BLUE,NamedColors::BLUE,NamedColors::BLACK}};
  static const BackpackLEDArray fail_offColor{{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}};

  BehaviorFactoryCentroidExtractor::BehaviorFactoryCentroidExtractor(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  {
    
    SubscribeToTags({{
      EngineToGameTag::MotorCalibration,
      EngineToGameTag::RobotCompletedFactoryDotTest
    }});
  }
  
  bool BehaviorFactoryCentroidExtractor::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
  {
    return !IsActing() && !_waitingForDots;
  }

  Result BehaviorFactoryCentroidExtractor::InitInternal(Robot& robot)
  {
    std::stringstream serialNumString;
    serialNumString << std::hex << robot.GetHeadSerialNumber();
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
    robot.GetBehaviorManager().DisableReactionsWithLock(
                                   kBehaviorTestName,
                                   ReactionTriggerHelpers::kAffectAllArray);
    
    
    
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
                    failLights.onColors = fail_onColorMagenta;
                    robot.GetBodyLightComponent().SetBackpackLights(failLights);
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
        failLights.onColors = fail_onColorRed;
        robot.GetBodyLightComponent().SetBackpackLights(failLights);
      }
      else
      {
        if(!msg.didComputePose)
        {
          PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.DidNotComputeCameraPose",
                              "Failed to compute camPose camera is not calibrated");
          failLights.onColors = fail_onColorOrange;
          robot.GetBodyLightComponent().SetBackpackLights(failLights);
        }
        else
        {
          static const bool doThresholdCheck = false;
          static const f32 rollThresh_rad = DEG_TO_RAD(5.f);
          static const f32 pitchThresh_rad = DEG_TO_RAD(5.f);
          static const f32 yawThresh_rad = DEG_TO_RAD(5.f);
          static const f32 xThresh_mm = 5;
          static const f32 yThresh_mm = 5;
          static const f32 zThresh_mm = 5;
          
          bool exceedsThresh = false;
          if(!NEAR(msg.camPoseRoll_rad, 0, rollThresh_rad))
          {
            PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.CamPose", "Roll exceeds threshold");
            exceedsThresh = true;
          }
          if(!NEAR(msg.camPosePitch_rad - msg.headAngle, DEG_TO_RAD(-4.f), pitchThresh_rad))
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
            failLights.onColors = fail_onColorBlue;
            robot.GetBodyLightComponent().SetBackpackLights(failLights);
          }
          else
          {
            robot.GetBodyLightComponent().SetBackpackLights(passLights);
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
