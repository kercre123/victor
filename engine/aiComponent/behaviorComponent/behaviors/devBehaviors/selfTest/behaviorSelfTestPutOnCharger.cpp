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
  : IBehaviorSelfTest(config, SelfTestResultCode::PUT_ON_CHARGER_TIMEOUT)
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

  const bool isPickedUp = robot.IsPickedUp();
  //const bool isBeingHeld = robot.IsBeingHeld();

  float textAngle = 0;
  if(isPickedUp/* && isBeingHeld*/)
  {
    const AccelData& accel = robot.GetHeadAccelData();
    if(accel.z <= SelfTestConfig::kUpsideDownZAccel)
    {
      textAngle = 180;
    }
  }

  DrawTextOnScreen(robot,
                   {"Put on charger"},
                   NamedColors::WHITE,
                   NamedColors::BLACK,
                   textAngle);

  AddTimer(30000, [this](){
    SELFTEST_SET_RESULT(SelfTestResultCode::TEST_TIMED_OUT);
  });

  return RESULT_OK;
}

IBehaviorSelfTest::SelfTestStatus BehaviorSelfTestPutOnCharger::SelfTestUpdateInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  const bool isPickedUp = robot.IsPickedUp();
  //const bool isBeingHeld = robot.IsBeingHeld();

  bool upsideDown = false;
  if(isPickedUp/* && isBeingHeld*/)
  {
    const AccelData& accel = robot.GetHeadAccelData();
    if(accel.z <= SelfTestConfig::kUpsideDownZAccel)
    {
      upsideDown = true;
    }
  }

  float textAngle = 0;
  if(!_isUpsideDown && upsideDown)
  {
    textAngle = 180;
  }

  if(_isUpsideDown != upsideDown)
  {
    DrawTextOnScreen(robot,
                     {"Put on charger"},
                     NamedColors::WHITE,
                     NamedColors::BLACK,
                     textAngle);
  }

  _isUpsideDown = upsideDown;

  const bool onCharger = robot.IsOnChargerPlatform();
  const bool charging = robot.IsCharging();

  if(onCharger || charging)
  {
    SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::SUCCESS, SelfTestStatus::Complete);
  }

  return SelfTestStatus::Running;
}

void BehaviorSelfTestPutOnCharger::OnBehaviorDeactivated()
{
  _isUpsideDown = false;
}

}
}
