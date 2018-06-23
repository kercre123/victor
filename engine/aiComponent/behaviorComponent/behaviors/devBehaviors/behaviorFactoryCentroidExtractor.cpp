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

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/actions/basicActions.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorFactoryCentroidExtractor.h"
#include "engine/components/backpackLights/backpackLightComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/robotToEngineImplMessaging.h"

// This define comments out code that depends on messages that don't exist outside of factory branch.
// This skips a variety of version checks so it should never be used on the line!
#define IS_FACTORY_BRANCH 0


namespace Anki {
namespace Cozmo {

namespace{

// Backpack lights
static const BackpackLightAnimation::BackpackAnimation passLights = {
  .onColors               = {{NamedColors::GREEN,NamedColors::GREEN,NamedColors::GREEN}},
  .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .onPeriod_ms            = {{1000,1000,1000}},
  .offPeriod_ms           = {{100,100,100}},
  .transitionOnPeriod_ms  = {{450,450,450}},
  .transitionOffPeriod_ms = {{450,450,450}},
  .offset                 = {{0,0,0}}
};

static BackpackLightAnimation::BackpackAnimation failLights = {
  .onColors               = {{NamedColors::RED,NamedColors::RED,NamedColors::RED}},
  .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .onPeriod_ms            = {{500,500,500}},
  .offPeriod_ms           = {{500,500,500}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};

static const BackpackLightAnimation::BackpackLEDArray fail_onColorRed{{NamedColors::RED,NamedColors::RED,NamedColors::RED}};
static const BackpackLightAnimation::BackpackLEDArray fail_onColorOrange{{NamedColors::ORANGE,NamedColors::ORANGE,NamedColors::ORANGE}};
static const BackpackLightAnimation::BackpackLEDArray fail_onColorMagenta{{NamedColors::MAGENTA,NamedColors::MAGENTA,NamedColors::MAGENTA}};
static const BackpackLightAnimation::BackpackLEDArray fail_onColorBlue{{NamedColors::BLUE,NamedColors::BLUE,NamedColors::BLUE}};
static const BackpackLightAnimation::BackpackLEDArray fail_offColor{{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}};
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFactoryCentroidExtractor::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFactoryCentroidExtractor::DynamicVariables::DynamicVariables()
{
  waitingForDots = false;
  headCalibrated = false;
  liftCalibrated = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFactoryCentroidExtractor::BehaviorFactoryCentroidExtractor(const Json::Value& config)
: ICozmoBehavior(config)
{
  
  SubscribeToTags({{
    EngineToGameTag::MotorCalibration,
    EngineToGameTag::RobotCompletedFactoryDotTest
  }});
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFactoryCentroidExtractor::WantsToBeActivatedBehavior() const
{
  return !IsControlDelegated() && !_dVars.waitingForDots;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFactoryCentroidExtractor::OnBehaviorActivated()
{
  Robot& robot = GetBEI().GetRobotInfo()._robot;
  std::stringstream serialNumString;
  serialNumString << std::hex << robot.GetHeadSerialNumber();
  _iConfig.factoryTestLogger.StartLog(serialNumString.str() + "_centroids", true, robot.GetContextDataPlatform());
  PRINT_NAMED_INFO("BehaviorFactoryCentroidExtractor.WillLogToDevice",
                    "Log name: %s",
                    _iConfig.factoryTestLogger.GetLogName().c_str());
  
  _dVars.waitingForDots = false;
  _dVars.headCalibrated = false;
  _dVars.liftCalibrated = false;
  
  robot.GetActionList().Cancel();
      
  // Start motor calibration
  robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::StartMotorCalibration(true, true)));
  
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFactoryCentroidExtractor::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if(_dVars.waitingForDots || !_dVars.liftCalibrated || !_dVars.headCalibrated)
  {
    return;
  }
  
  if(!IsControlDelegated()){
    CancelSelf();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFactoryCentroidExtractor::OnBehaviorDeactivated()
{
  _dVars.waitingForDots = false;
  _dVars.liftCalibrated = false;
  _dVars.headCalibrated = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFactoryCentroidExtractor::TransitionToMovingHead(Robot& robot)
{
  MoveHeadToAngleAction* action = new MoveHeadToAngleAction(0);
  DelegateIfInControl(action, [this, &robot](ActionResult res)
              {
                if(res == ActionResult::SUCCESS)
                {
                  _dVars.waitingForDots = true;
                  robot.GetVisionComponent().UseNextImageForFactoryDotTest();
                }
                else
                {
                  PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.MoveHead", "Moving head to 0 degrees failed");
                  failLights.onColors = fail_onColorMagenta;
                  robot.GetBackpackLightComponent().SetBackpackAnimation(failLights);
                }
              });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFactoryCentroidExtractor::HandleWhileActivated(const EngineToGameEvent& event)
{
  Robot& robot = GetBEI().GetRobotInfo()._robot;
  if(event.GetData().GetTag() == EngineToGameTag::RobotCompletedFactoryDotTest)
  {
    const auto& msg = event.GetData().Get_RobotCompletedFactoryDotTest();
    
    if(!msg.success)
    {
      PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.DotTestFailed",
                          "Failed to find all 4 dots");
      failLights.onColors = fail_onColorRed;
      robot.GetBackpackLightComponent().SetBackpackAnimation(failLights);
    }
    else
    {
      if(!msg.didComputePose)
      {
        PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.DidNotComputeCameraPose",
                            "Failed to compute camPose camera is not calibrated");
        failLights.onColors = fail_onColorOrange;
        robot.GetBackpackLightComponent().SetBackpackAnimation(failLights);
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
          robot.GetBackpackLightComponent().SetBackpackAnimation(failLights);
        }
        else
        {
          robot.GetBackpackLightComponent().SetBackpackAnimation(passLights);
        }
      }
    }
    _iConfig.factoryTestLogger.Append(msg);
  }
  else if(event.GetData().GetTag() == EngineToGameTag::MotorCalibration)
  {
    const auto& msg = event.GetData().Get_MotorCalibration();
    
    if(!msg.calibStarted)
    {
      if(msg.motorID == MotorID::MOTOR_HEAD)
      {
        _dVars.headCalibrated = true;
        if(_dVars.liftCalibrated)
        {
          TransitionToMovingHead(robot);
        }
      }
      if(msg.motorID == MotorID::MOTOR_LIFT)
      {
        _dVars.liftCalibrated = true;
        if(_dVars.headCalibrated)
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

} // namespace Cozmo
} // namespace Anki
