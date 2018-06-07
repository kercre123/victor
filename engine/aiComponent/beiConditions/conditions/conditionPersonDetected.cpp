/**
 * File: conditionPersonDetected.cpp
 *
 * Author: Lorenzo Riano
 * Created: 5/31/18
 *
 * Description: Condition which is true when a person is detected. Uses SalientPointDetectorComponent
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
  PRINT_CH_DEBUG("Behaviors", "ConditionPersonDetected.AreConditionsMetInternal.Called", "");

  auto& component = behaviorExternalInterface.GetAIComponent().GetComponent<SalientPointsDetectorComponent>();
  return component.PersonDetected();

}

void
Anki::Cozmo::ConditionPersonDetected::GetRequiredVisionModes(std::set<Anki::Cozmo::VisionModeRequest>& requiredVisionModes) const
{
  //TODO we need person detection mode here
  requiredVisionModes.insert( {VisionMode::RunningNeuralNet, EVisionUpdateFrequency::Low} );
}

} // namespace Cozmo
} // namespace Anki


