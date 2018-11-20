/**
 * File: behaviorSelfTestWaitToStart.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestWaitToStart.h"

#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

BehaviorSelfTestWaitToStart::BehaviorSelfTestWaitToStart(const Json::Value& config)
: IBehaviorSelfTest(config)
{
  // Subscribe to all of the base class's failure tags to prevent us from automatically failing
  // if we get put on the charger, motor calibration on startup, etc...
  std::set<ExternalInterface::MessageEngineToGameTag> tags = GetFailureTags();
  SubscribeToTags(std::move(tags));

  ICozmoBehavior::SubscribeToTags({RobotInterface::RobotToEngineTag::startSelfTest});
}

Result BehaviorSelfTestWaitToStart::OnBehaviorActivatedInternal()
{
  // Remove the default selfTest behavior timeout timer since this behavior will
  // run forever until the conditions for selfTest to start are met
  ClearTimers();

  return RESULT_OK;
}

IBehaviorSelfTest::SelfTestStatus BehaviorSelfTestWaitToStart::SelfTestUpdateInternal()
{
  if(!_startTest)
  {
    return SelfTestStatus::Running;    
  }

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  //Robot& robot = GetBEI().GetRobotInfo()._robot;

  // // Draw nothing on the screen to clear it
  // robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::DrawTextOnScreen(true,	
  //                                                                                  RobotInterface::ColorRGB(0,0,0),	
  //                                                                                  RobotInterface::ColorRGB(0,0,0),	
  //                                                                                  "")));
    
  SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, SelfTestStatus::Complete);
}

void BehaviorSelfTestWaitToStart::OnBehaviorDeactivated()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;
  
  robot.GetBodyLightComponent().SetBackpackLights(robot.GetBodyLightComponent().GetOffBackpackLights());

  _startTest = false;

  // Tell robot process that SelfTest is starting so that it can reset touch sensor valid state
  // and update range values in case they were updated in emr
  //robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::SelfTestStart()));
}

void BehaviorSelfTestWaitToStart::AlwaysHandleInScope(const RobotToEngineEvent& event)
{
  const auto& tag = event.GetData().GetTag();
  PRINT_NAMED_WARNING("","%hhu", tag);
  if(tag != RobotInterface::RobotToEngineTag::startSelfTest)
  {
    return;
  }
  PRINT_NAMED_WARNING("","START TEST MESSAGE");
  // Robot& robot = GetBEI().GetRobotInfo()._robot;
    
  //const auto& payload = event.GetData().Get_startSelfTest();
  _startTest = true;
}


}
}


