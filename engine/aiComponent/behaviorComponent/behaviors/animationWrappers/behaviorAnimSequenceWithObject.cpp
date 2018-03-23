/**
 * File: behaviorAnimSequenceWithObject.cpp
 *
 * Author: Matt Michini
 * Created: 2018-01-11
 *
 * Description:  a sequence of animations after turning to an object (if possible)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimSequenceWithObject.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/actions/basicActions.h"
#include "engine/blockWorld/blockWorld.h"

#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Cozmo {
  
namespace {
const char* kObjectTypeKey = "objectType";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
BehaviorAnimSequenceWithObject::InstanceConfig::InstanceConfig() 
{
  objectType = ObjectType::UnknownObject;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
BehaviorAnimSequenceWithObject::DynamicVariables::DynamicVariables() 
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
BehaviorAnimSequenceWithObject::BehaviorAnimSequenceWithObject(const Json::Value& config)
: BaseClass(config)
{
  std::string objectTypeStr;
  if (JsonTools::GetValueOptional(config, kObjectTypeKey, objectTypeStr)) {
    _iConfig.objectType = ObjectTypeFromString(objectTypeStr);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequenceWithObject::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  BaseClass::GetBehaviorJsonKeys( expectedKeys );
  expectedKeys.insert( kObjectTypeKey );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
bool BehaviorAnimSequenceWithObject::WantsToBeActivatedBehavior() const
{
  return (GetLocatedObject() != nullptr);
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequenceWithObject::OnBehaviorActivated()
{
  const auto* obj = GetLocatedObject();
  
  if( obj != nullptr ) {
    // Attempt to turn toward the specified object, and even if fails, move on to the animations
    DelegateIfInControl(new TurnTowardsObjectAction(obj->GetID()), [this]() {
      BaseClass::StartPlayingAnimations();
    });
  } else {
    // can occur in unit tests
    PRINT_NAMED_WARNING( "BehaviorAnimSequenceWithObject.OnBehaviorActivated.NullObject",
                         "Null object!" );
  }
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObservableObject* BehaviorAnimSequenceWithObject::GetLocatedObject() const
{
  // Find matching objects
  BlockWorldFilter filter;
  if (_iConfig.objectType != ObjectType::UnknownObject) {
    filter.AddAllowedType(_iConfig.objectType);
  }
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto* object = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, filter);
  
  return object;
}

} // namespace Cozmo
} // namespace Anki
