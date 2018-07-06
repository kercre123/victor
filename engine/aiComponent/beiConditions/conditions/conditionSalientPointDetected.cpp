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
#include "engine/aiComponent/beiConditions/conditions/conditionSalientPointDetected.h"
#include "engine/aiComponent/salientPointsComponent.h"

namespace Anki {
namespace Cozmo {

ConditionSalientPointDetected::ConditionSalientPointDetected(const Json::Value& config)
    : IBEICondition(config)
{

  const std::string& targetSalientPoint = JsonTools::ParseString(config, "targetSalientPoint",
                                                                 "ConditionSalientPointDetected.Config");
  ANKI_VERIFY(Vision::SalientPointTypeFromString(targetSalientPoint, _targetSalientPointType),
              "ConditionSalientPointDetected.Config.IncorrectString",
              "%s is not a valid SalientPointType",
              targetSalientPoint.c_str());

}

ConditionSalientPointDetected::~ConditionSalientPointDetected()
{

}

void ConditionSalientPointDetected::InitInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // no need to subscribe to messages here, the SalientPointsComponent will do that for us
}

bool ConditionSalientPointDetected::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  PRINT_CH_DEBUG("Behaviors", "ConditionSalientPointDetected.AreConditionsMetInternal.Called", "");

  const auto& component = behaviorExternalInterface.GetAIComponent().GetComponent<SalientPointsComponent>();
  return component.SalientPointDetected(_targetSalientPointType);

}

void
Anki::Cozmo::ConditionSalientPointDetected::GetRequiredVisionModes(std::set<Anki::Cozmo::VisionModeRequest>& requiredVisionModes) const
{
  requiredVisionModes.insert( {VisionMode::RunningNeuralNet, EVisionUpdateFrequency::Low} );
}

} // namespace Cozmo
} // namespace Anki


