/**
 * File: behaviorSelfTestDockWithCharger.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestDockWithCharger.h"

#include "engine/actions/chargerActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

BehaviorSelfTestDockWithCharger::BehaviorSelfTestDockWithCharger(const Json::Value& config)
: IBehaviorSelfTest(config)
{
  SubscribeToTags({ExternalInterface::MessageEngineToGameTag::ChargerEvent});
}

Result BehaviorSelfTestDockWithCharger::OnBehaviorActivatedInternal()
{
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  BlockWorldFilter filter;
  filter.AddAllowedType(ObjectType::Charger_Basic);
  std::vector<ObservableObject*> objects;
  robot.GetBlockWorld().FindLocatedMatchingObjects(filter, objects);

  if(objects.empty())
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestDockWithCharger.GetExpectedObjectMarkerPoseWrtRobot.NullObject",
                        "Expected object of type %s does not exist or am not seeing it",
                        ObjectTypeToString(ObjectType::Charger_Basic));
    SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::CHARGER_UNAVAILABLE, RESULT_FAIL);
  }
  else
  {
    // Get the most recently observed object of the expected object type
    ObservableObject* object = nullptr;
    TimeStamp_t t = 0;
    for(const auto& obj : objects)
    {
      if(obj->GetLastObservedTime() > t)
      {
        object = obj;
        t = obj->GetLastObservedTime();
      }
    }

    if(object == nullptr)
    {
      PRINT_NAMED_INFO("BehaviorSelfTestDockWithCharger.GetExpectedObjectMarkerPoseWrtRobot.NullObject","");
      SELFTEST_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::CHARGER_UNAVAILABLE, RESULT_FAIL);
    }

    DriveToAndMountChargerAction* action = new DriveToAndMountChargerAction(object->GetID());

    DelegateIfInControl(action, [this](){ TransitionToOnChargerChecks(); });
  }
    
  return RESULT_OK;
}

void BehaviorSelfTestDockWithCharger::OnBehaviorDeactivated()
{

}

void BehaviorSelfTestDockWithCharger::TransitionToOnChargerChecks()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  if(robot.GetBatteryVoltage() < PlaypenConfig::kMinBatteryVoltage)
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestInitChecks.OnActivated.BatteryTooLow", "%fv", robot.GetBatteryVoltage());
    SELFTEST_SET_RESULT(FactoryTestResultCode::BATTERY_TOO_LOW);
  }

  // TODO Maybe check cliff sensors for no cliff here
  // Difficult because don't know what kind of surface we are on, may be a dark table
  
  SELFTEST_SET_RESULT(FactoryTestResultCode::SUCCESS);
}

}
}


