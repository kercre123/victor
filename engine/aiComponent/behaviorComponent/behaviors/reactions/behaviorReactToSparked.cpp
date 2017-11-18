/**
 * File: behaviorReactToSparked.cpp
 *
 * Author:  Kevin M. Karol
 * Created: 2016-09-13
 *
 * Description:  Reaction that cancels other reactions when cozmo is sparked
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToSparked.h"


namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToSparked::BehaviorReactToSparked(const Json::Value& config)
: ICozmoBehavior(config)
{
  
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToSparked::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToSparked::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}

} // namespace Cozmo
} // namespace Anki

