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

#include <algorithm>
#include <iterator>

namespace Anki {
namespace Vector {

namespace {

const char* const kRequiredVisionModesKey = "requiredVisionModes";

} /* anonymous namespace */

ConditionSalientPointDetected::ConditionSalientPointDetected(const Json::Value& config)
    : IBEICondition(config)
{

  const std::string& targetSalientPoint = JsonTools::ParseString(config, "targetSalientPoint",
                                                                 "ConditionSalientPointDetected.Config");
  ANKI_VERIFY(Vision::SalientPointTypeFromString(targetSalientPoint, _targetSalientPointType),
              "ConditionSalientPointDetected.Config.IncorrectString",
              "%s is not a valid SalientPointType",
              targetSalientPoint.c_str());

  if(config.isMember(kRequiredVisionModesKey))
  {
    auto const & visionModes = config[kRequiredVisionModesKey];

    auto setVisionModeHelper = [this](const Json::Value& jsonVisionMode)
    {
      const std::string& visionModeStr = jsonVisionMode.asString();
      VisionMode visionMode;
      const bool success = VisionModeFromString(visionModeStr, visionMode);
      if(success)
      {
         _requiredVisionModes.push_back(visionMode);
      }
      else
      {
        LOG_WARNING("ConditionSalientPointDetected.Constructor.InvalidVisionMode", "%s", visionModeStr.c_str());
      }
    };

    if(visionModes.isArray())
    {
      std::for_each(visionModes.begin(), visionModes.end(), setVisionModeHelper);
    }
    else if(visionModes.isString())
    {
      setVisionModeHelper(visionModes);
    }
    else
    {
      LOG_WARNING("ConditionSalientPointDetected.Constructor.InvalidVisionModeEntry", "");
    }
  }

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
Anki::Vector::ConditionSalientPointDetected::GetRequiredVisionModes(std::set<Anki::Vector::VisionModeRequest>& requiredVisionModes) const
{
  // TODO: Allow for update frequency to be set via configuration
  std::transform(_requiredVisionModes.begin(), _requiredVisionModes.end(),
                 std::inserter(requiredVisionModes, requiredVisionModes.end()),
                 [](VisionMode mode) -> VisionModeRequest { return {mode, EVisionUpdateFrequency::Low}; });
}

} // namespace Vector
} // namespace Anki


