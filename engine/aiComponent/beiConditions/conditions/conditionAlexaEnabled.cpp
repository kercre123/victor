/**
 * File: conditionAlexaEnabled.cpp
 *
 * Author: Jarrod
 * Created: 2018 november 11th
 *
 * Description: True if a Alexa is enabled
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionAlexaEnabled.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/alexaComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/cozmoContext.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Vector {
  
namespace {
  const char* const kExpectedKey    = "expected"; // optional bool, defaults to true
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionAlexaEnabled::ConditionAlexaEnabled( const Json::Value& config ) :
  IBEICondition( config )
{
  _expected = config.get( kExpectedKey, true ).asBool();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionAlexaEnabled::AreConditionsMetInternal( BehaviorExternalInterface& bei ) const
{
  const AlexaComponent& alexaComponent = bei.GetAIComponent().GetComponent<AlexaComponent>();
  const bool isAuthenticated = alexaComponent.IsAuthenticated();
  return ( isAuthenticated == _expected );
}

}
}
