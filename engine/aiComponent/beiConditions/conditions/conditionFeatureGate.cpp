/**
 * File: conditionFeatureGate.cpp
 *
 * Author: ross
 * Created: 2018 Apr 11
 *
 * Description: True if a feature is enabled
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionFeatureGate.h"

#include "clad/types/featureGateTypes.h"
#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/cozmoContext.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const char* const kFeatureTypeKey = "feature";
  const char* const kExpectedKey    = "expected"; // optional bool, defaults to true
}

ConditionFeatureGate::ConditionFeatureGate(const Json::Value& config, BEIConditionFactory& factory)
  : IBEICondition(config, factory)
{
  std::string featureStr = JsonTools::ParseString( config, kFeatureTypeKey, "ConditionFeatureGate" );
  ANKI_VERIFY( FeatureTypeFromString( featureStr, _featureType ),
               "ConditionFeatureGate.Ctor.Invalid",
               "Unknown feature gate type %s",
               featureStr.c_str() );
  _expected = config.get( kExpectedKey, true ).asBool();
}

bool ConditionFeatureGate::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const bool isValid = _featureType != FeatureType::Invalid;
  const bool enabled = bei.GetRobotInfo().GetContext()->GetFeatureGate()->IsFeatureEnabled( _featureType );
  return isValid && (enabled == _expected);
}

}
}
