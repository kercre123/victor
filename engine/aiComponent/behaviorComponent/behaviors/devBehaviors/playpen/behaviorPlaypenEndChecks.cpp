/**
 * File: behaviorPlaypenEndChecks.cpp
 *
 * Author: Al Chaussee
 * Created: 08/09/17
 *
 * Description: Checks any final things playpen is interested in like battery voltage and that we have heard
 *              from an active object
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenEndChecks.h"

#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

BehaviorPlaypenEndChecks::BehaviorPlaypenEndChecks(const Json::Value& config)
: IBehaviorPlaypen(config)
{

}


void BehaviorPlaypenEndChecks::InitBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  ICozmoBehavior::SubscribeToTags({RobotInterface::RobotToEngineTag::activeObjectAvailable});
}


Result BehaviorPlaypenEndChecks::OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();

  if(robot.GetBatteryVoltage() < PlaypenConfig::kMinBatteryVoltage)
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::BATTERY_TOO_LOW, RESULT_FAIL);
  }
  
  if(!_heardFromLightCube)
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::NO_ACTIVE_OBJECTS_DISCOVERED, RESULT_FAIL);
  }
  
  PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::SUCCESS, RESULT_OK);
}

void BehaviorPlaypenEndChecks::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _heardFromLightCube = false;
}

void BehaviorPlaypenEndChecks::AlwaysHandle(const RobotToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  const auto& tag = event.GetData().GetTag();
  if(tag == RobotInterface::RobotToEngineTag::activeObjectAvailable)
  {
    const auto& payload = event.GetData().Get_activeObjectAvailable();
    if(IsValidLightCube(payload.objectType, false))
    {
      _heardFromLightCube = true;
    }
  }
}

}
}


