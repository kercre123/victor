/**
 * File: behaviorSelfTestDockWithCharger.cpp
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Robot docks with the charger
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestDockWithCharger.h"

#include "engine/actions/chargerActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/robot.h"

namespace Anki {
namespace Vector {

BehaviorSelfTestDockWithCharger::BehaviorSelfTestDockWithCharger(const Json::Value& config)
: IBehaviorSelfTest(config, SelfTestResultCode::DOCK_WITH_CHARGER_TIMEOUT)
{
  SubscribeToTags({ExternalInterface::MessageEngineToGameTag::ChargerEvent});
}

Result BehaviorSelfTestDockWithCharger::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Check for the object in the world
  BlockWorldFilter filter;
  filter.AddAllowedType(ObjectType::Charger_Basic);
  std::vector<ObservableObject*> objects;
  robot.GetBlockWorld().FindLocatedMatchingObjects(filter, objects);

  if(objects.empty())
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestDockWithCharger.GetExpectedObjectMarkerPoseWrtRobot.NullObject",
                        "Expected object of type %s does not exist or am not seeing it",
                        ObjectTypeToString(ObjectType::Charger_Basic));
    SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::CHARGER_NOT_FOUND, RESULT_FAIL);
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
      SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::CHARGER_NOT_FOUND, RESULT_FAIL);
    }

    const bool useCliffSensorCorrection = true;
    const bool enableDockingAnims = false;
    const bool doPositionCheckOnPathCompletion = false;
    DriveToAndMountChargerAction* action = new DriveToAndMountChargerAction(object->GetID(),
                                                                            useCliffSensorCorrection,
                                                                            enableDockingAnims,
                                                                            doPositionCheckOnPathCompletion);

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

  const float batteryVolts = robot.GetBatteryComponent().GetBatteryVoltsRaw();
  const float chargerVolts = robot.GetBatteryComponent().GetChargerVoltsRaw();
  const bool disconnected = robot.GetBatteryComponent().IsBatteryDisconnectedFromCharger();
  const bool batteryVoltageGood = batteryVolts >= SelfTestConfig::kMinBatteryVoltage;
  const bool chargerVoltageGood = chargerVolts >= 4;

  // Only check battery voltage if it isn't disconnected
  if(!disconnected && !batteryVoltageGood)
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestDockWithCharger.OnActivated.BatteryTooLow", "%fv", batteryVolts);
    SELFTEST_SET_RESULT(SelfTestResultCode::BATTERY_TOO_LOW);
  }

  if(!chargerVoltageGood)
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestDockWithCharger.OnActivated.ChargerTooLow", "%fv", chargerVolts);
    SELFTEST_SET_RESULT(SelfTestResultCode::CHARGER_VOLTAGE_TOO_LOW);
  }


  SELFTEST_SET_RESULT(SelfTestResultCode::SUCCESS);
}

}
}


