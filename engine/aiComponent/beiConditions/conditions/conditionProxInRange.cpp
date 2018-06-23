/**
 * File: conditionProxInRange.cpp
 *
 * Author: Kevin M. Karol
 * Created: 1/26/18
 *
 * Description: Determine whether the current proximity sensor reading is valid
 * and within the given range
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/beiConditions/conditions/conditionProxInRange.h"

#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/components/sensors/proxSensorComponent.h"

#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

namespace {

const char* kMinDistKey = "minProxDist_mm";
const char* kMaxDistKey = "maxProxDist_mm";
const char* kInvalidSensorReturnValue = "invalidSensorReturn";

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionProxInRange::ConditionProxInRange(const Json::Value& config)
: IBEICondition(config)
{
  const bool gotBegin = JsonTools::GetValueOptional(config, kMinDistKey, _params.minDist_mm);
  const bool gotEnd = JsonTools::GetValueOptional(config, kMaxDistKey, _params.maxDist_mm);
  JsonTools::GetValueOptional(config, kInvalidSensorReturnValue, _params.invalidSensorReturn);

  ANKI_VERIFY( gotBegin || gotEnd, "ConditionTimerInRange.BadConfig", "Need to specify at least one of '%s' or '%s'",
               kMinDistKey,
               kMaxDistKey );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionProxInRange::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  auto& proxSensor = bei.GetComponentWrapper(BEIComponentID::ProxSensor).GetComponent<ProxSensorComponent>();

  u16 proxDist_mm = 0;
  const bool isValid = proxSensor.GetLatestDistance_mm(proxDist_mm);
  if(!isValid){
    return _params.invalidSensorReturn;
  }

  return Util::InRange((float) proxDist_mm, _params.minDist_mm, _params.maxDist_mm);
}

}
}
