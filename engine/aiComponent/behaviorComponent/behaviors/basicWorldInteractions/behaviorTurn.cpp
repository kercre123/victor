/**
 * File: behaviorTurn.cpp
 *
 * Author:  Kevin M. Karol
 * Created: 1/26/18
 *
 * Description:  Behavior which turns the robot a set number of degrees
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorTurn.h"

#include "anki/common/constantsAndMacros.h"
#include "coretech/common/engine/jsonTools.h"
#include "engine/actions/basicActions.h"


namespace Anki {
namespace Vector {

namespace{
const char* kTurnClockwiseKey = "shouldTurnClockwise";
const char* kTurnDegreesKey   = "turnDegrees";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTurn::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTurn::DynamicVariables::DynamicVariables()
{
  turnRad = 0.f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTurn::BehaviorTurn(const Json::Value& config)
: ICozmoBehavior(config)
{
  const std::string& debugName = "BehaviorTurn.BehaviorTurn.LoadConfig";

  const bool turnClockwise = JsonTools::ParseBool(config, kTurnClockwiseKey, debugName);
  _dVars.turnRad = DEG_TO_RAD_F32(JsonTools::ParseFloat(config, kTurnDegreesKey, debugName));
  _dVars.turnRad = turnClockwise ? -_dVars.turnRad : _dVars.turnRad;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurn::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kTurnClockwiseKey,
    kTurnDegreesKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTurn::WantsToBeActivatedBehavior() const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurn::OnBehaviorActivated()
{
  const bool isAbsolute = false;
  DelegateIfInControl(new TurnInPlaceAction(_dVars.turnRad, isAbsolute));
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurn::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

}


} // namespace Vector
} // namespace Anki

