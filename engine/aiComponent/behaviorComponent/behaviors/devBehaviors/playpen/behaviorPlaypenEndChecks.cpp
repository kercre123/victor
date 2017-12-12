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

#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
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
    PRINT_NAMED_WARNING("BehaviorPlaypenEndChecks.OnActivated.BatteryTooLow", "%fv", robot.GetBatteryVoltage());
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::BATTERY_TOO_LOW, RESULT_FAIL);
  }
  
  if(!PlaypenConfig::kSkipActiveObjectCheck && !_heardFromLightCube)
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::NO_ACTIVE_OBJECTS_DISCOVERED, RESULT_FAIL);
  }

  if(!DidReceiveFFTResult())
  {
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::NO_FFT_RESULT, RESULT_FAIL);
  }

  TurnInPlaceAction* turn = new TurnInPlaceAction(robot, DEG_TO_RAD(180), false);
  MoveHeadToAngleAction* head = new MoveHeadToAngleAction(robot, MAX_HEAD_ANGLE);
  CompoundActionParallel* action = new CompoundActionParallel(robot, {turn, head});

  DelegateIfInControl(action, [this](){ PLAYPEN_SET_RESULT(FactoryTestResultCode::SUCCESS); });

  return RESULT_OK;
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


