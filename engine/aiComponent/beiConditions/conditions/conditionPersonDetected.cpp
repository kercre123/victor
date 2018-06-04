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

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/beiConditions/conditions/conditionPersonDetected.h"
#include "engine/aiComponent/salientPointsDetectorComponent.h"

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
  // no need to subscribe to messages here, the SalientPointsDetectorComponent will do that for us
}

bool ConditionPersonDetected::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  PRINT_CH_INFO("Behaviors", "ConditionPersonDetected.AreConditionsMetInternal.Called", "");

  auto& component = behaviorExternalInterface.GetAIComponent().GetComponent<SalientPointsDetectorComponent>();
  return component.PersonDetected();

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


