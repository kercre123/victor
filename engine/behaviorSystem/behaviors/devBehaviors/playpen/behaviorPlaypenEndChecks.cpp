/**
 * File: behaviorPlaypenInitChecks.cpp
 *
 * Author: Al Chaussee
 * Created: 08/09/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenEndChecks.h"

#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

BehaviorPlaypenEndChecks::BehaviorPlaypenEndChecks(Robot& robot, const Json::Value& config)
: IBehaviorPlaypen(robot, config)
{
  IBehavior::SubscribeToTags(robot.GetID(),
                             {RobotInterface::RobotToEngineTag::activeObjectAvailable});
}

Result BehaviorPlaypenEndChecks::InternalInitInternal(Robot& robot)
{
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

BehaviorStatus BehaviorPlaypenEndChecks::InternalUpdateInternal(Robot& robot)
{

  return BehaviorStatus::Running;
}

void BehaviorPlaypenEndChecks::StopInternal(Robot& robot)
{
  _heardFromLightCube = false;
}

void BehaviorPlaypenEndChecks::HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot)
{
  
}

void BehaviorPlaypenEndChecks::AlwaysHandle(const RobotToEngineEvent& event, const Robot& robot)
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


