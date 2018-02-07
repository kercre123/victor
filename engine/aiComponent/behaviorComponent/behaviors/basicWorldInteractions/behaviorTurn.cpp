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
namespace Cozmo {

namespace{
const char* kTurnClockwiseKey = "shouldTurnClockwise";
const char* kTurnDegreesKey   = "turnDegrees";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTurn::BehaviorTurn(const Json::Value& config)
: ICozmoBehavior(config)
{
  const std::string& debugName = "BehaviorTurn.BehaviorTurn.LoadConfig";

  const bool turnClockwise = JsonTools::ParseBool(config, kTurnClockwiseKey, debugName);
  _params.turnRad = DEG_TO_RAD_F32(JsonTools::ParseFloat(config, kTurnDegreesKey, debugName));
  _params.turnRad = turnClockwise ? -_params.turnRad : _params.turnRad;
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
  DelegateIfInControl(new TurnInPlaceAction(_params.turnRad, isAbsolute));
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurn::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

}


} // namespace Cozmo
} // namespace Anki

