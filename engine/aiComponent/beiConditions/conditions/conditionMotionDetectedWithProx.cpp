/**
 * File: conditionMotionDetectedWithProx.cpp
 *
 * Author: Guillermo Bautista
 * Created: 04/24/19
 *
 * Description: Condition which is true when motion is detected with the TOF sensor(s)
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/beiConditions/conditions/conditionMotionDetectedWithProx.h"

#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/components/sensors/proxSensorComponent.h"

#include "util/math/math.h"

namespace Anki {
namespace Vector {

namespace {
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionMotionDetectedWithProx::ConditionMotionDetectedWithProx(const Json::Value& config)
: IBEICondition(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionMotionDetectedWithProx::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const auto& proxSensor = bei.GetComponentWrapper(BEIComponentID::ProxSensor).GetComponent<ProxSensorComponent>();
  const auto& proxData = proxSensor.GetLatestProxData();
  if(!proxData.foundObject){
    return false;
  }
  return true;
}

}
}
