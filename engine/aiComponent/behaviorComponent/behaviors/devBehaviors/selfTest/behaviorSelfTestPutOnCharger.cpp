/**
 * File: behaviorSelfTestPutOnCharger.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestPutOnCharger.h"

#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/robot.h"

#include "coretech/vision/engine/image.h"

namespace Anki {
namespace Cozmo {

BehaviorSelfTestPutOnCharger::BehaviorSelfTestPutOnCharger(const Json::Value& config)
: IBehaviorSelfTest(config)
{
  SubscribeToTags(std::set<ExternalInterface::MessageEngineToGameTag>
                  {ExternalInterface::MessageEngineToGameTag::ChargerEvent,
                   ExternalInterface::MessageEngineToGameTag::RobotOffTreadsStateChanged});
}

Result BehaviorSelfTestPutOnCharger::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  DrawTextOnScreen(robot, {"Put on charger"});
  
  AddTimer(30000, [this](){
    SELFTEST_SET_RESULT(FactoryTestResultCode::TEST_TIMED_OUT);
  });
  
  return RESULT_OK;
}

IBehaviorSelfTest::SelfTestStatus BehaviorSelfTestPutOnCharger::SelfTestUpdateInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  const bool onCharger = robot.IsOnCharger();
  const bool charging = robot.IsCharging();

  if(onCharger || charging)
  {
    SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, SelfTestStatus::Complete);
  }
  
  return SelfTestStatus::Running;
}

void BehaviorSelfTestPutOnCharger::OnBehaviorDeactivated()
{

}

}
}


