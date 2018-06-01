/**
 * File: conditionPersonDetected.cpp
 *
 * Author: Lorenzo Riano
 * Created: 5/31/18
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionPersonDetected.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Cozmo {

ConditionPersonDetected::ConditionPersonDetected(const Json::Value& config)
    : IBEICondition(config)
{
  // TODO need something from the json file here?
}

ConditionPersonDetected::ConditionPersonDetected()
    : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::PersonDetected))
{
}


ConditionPersonDetected::~ConditionPersonDetected()
{

}

void
ConditionPersonDetected::InitInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  _messageHelper.reset(new BEIConditionMessageHelper(this, behaviorExternalInterface));

  // TODO get the right tag for person detection
//  _messageHelper->SubscribeToTags({EngineToGameTag::RobotObservedMotion});

}

bool ConditionPersonDetected::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  PRINT_CH_INFO("Behaviors", "ConditionPersonDetected.AreConditionsMetInternal.Called", "");
  // TODO Here I'm cheating: test if x seconds have passed and if so make the condition true

  float currentTickCount = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if ((currentTickCount - _timeSinceLastObservation) > 3 ) { // 3 seconds
    PRINT_CH_INFO("Behaviors", "ConditionPersonDetected.AreConditionsMetInternal.ConditionTrue", "");
    _timeSinceLastObservation = currentTickCount;
    return true;
  }
  else {
    PRINT_CH_INFO("Behaviors", "ConditionPersonDetected.AreConditionsMetInternal.ConditionFalse", "");
    return false;
  }
}

void
Anki::Cozmo::ConditionPersonDetected::GetRequiredVisionModes(std::set<Anki::Cozmo::VisionModeRequest>& requiredVisionModes) const
{
  //TODO we need person detection mode here
//  requiredVisionModes.insert( {VisionMode::DetectingMotion, EVisionUpdateFrequency::High} );
}

void Anki::Cozmo::ConditionPersonDetected::HandleEvent(const EngineToGameEvent& event,
                                                       BehaviorExternalInterface& bei)
{

  switch (event.GetData().GetTag()) {
    // TODO react to the right tag

    default:
      PRINT_NAMED_WARNING("ConditionPersonDetected.HandleEvent.InvalidEvent", "");
      break;
  }

} // namespace Cozmo
}

} // namespace Anki


