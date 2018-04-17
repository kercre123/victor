/**
 * File: ConditionObjectInitialDetection.cpp
 *
 * Author: Matt Michini
 * Created: 1/10/2018
 *
 * Description: Strategy for responding to an object being seen
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionObjectInitialDetection.h"

#include "engine/aiComponent/beiConditions/beiConditionMessageHelper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/blockWorld/blockWorld.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Cozmo {

namespace{
  const char* kObjectTypeKey = "objectType";
  
  const char* kFirstTimeOnlyKey = "firstTimeOnly";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionObjectInitialDetection::ConditionObjectInitialDetection(const Json::Value& config, BEIConditionFactory& factory)
  : IBEICondition(config, factory)
{
  const auto& objectTypeStr = JsonTools::ParseString(config,
                                                    kObjectTypeKey,
                                                    "ConditionObjectInitialDetection.ConfigError.ObjectType");
  _targetType = ObjectTypeFromString(objectTypeStr);
  
  _firstTimeOnly = JsonTools::ParseBool(config,
                                        kFirstTimeOnlyKey,
                                        "ConditionObjectInitialDetection.ConfigError.FirstTimeOnly");
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionObjectInitialDetection::~ConditionObjectInitialDetection()
{
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectInitialDetection::InitInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  _messageHelper.reset(new BEIConditionMessageHelper(this, behaviorExternalInterface));
  
  _messageHelper->SubscribeToTags({
    EngineToGameTag::RobotObservedObject
  });
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionObjectInitialDetection::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if (_firstTimeOnly && _reacted) {
    return false;
  }
  
  if (_tickOfLastObservation == BaseStationTimer::getInstance()->GetTickCount()) {
    _reacted = true;
    return true;
  }

  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectInitialDetection::HandleEvent(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  switch(event.GetData().GetTag()){
    case EngineToGameTag::RobotObservedObject:
    {
      const auto& observedType = event.GetData().Get_RobotObservedObject().objectType;
      if (observedType == _targetType) {
        _tickOfLastObservation = BaseStationTimer::getInstance()->GetTickCount();
      }
      break;
    }
    default:
      break;
  }
}


} // namespace Cozmo
} // namespace Anki
