/**
* File: anonymousBehaviorFactory.cpp
*
* Author: Kevin M. Karol
* Created: 11/3/17
*
* Description: Component that allows anonymous behaviors to be created at runtime
* Anonymous behaviors are not stored in the behavior container and will be destroyed
* if the pointer returned from this factory leaves scope
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorComponent/anonymousBehaviorFactory.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"

namespace Anki {
namespace Cozmo {

namespace {
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnonymousBehaviorFactory::AnonymousBehaviorFactory(const BehaviorContainer& bContainer)
: _behaviorContainer(bContainer)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnonymousBehaviorFactory::~AnonymousBehaviorFactory()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr AnonymousBehaviorFactory::CreateBehavior(BehaviorClass behaviorType, Json::Value& parameters)
{
  ICozmoBehavior::InjectBehaviorClassAndIDIntoConfig(behaviorType, BEHAVIOR_ID(Anonymous), parameters);
  return _behaviorContainer.CreateAnonymousBehavior(behaviorType, parameters);
}



} // namespace Cozmo
} // namespace Anki

