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

  
BehaviorAnimSequenceWithObject::BehaviorAnimSequenceWithObject(const Json::Value& config)
: BaseClass(config)
{
  std::string objectTypeStr;
  if (JsonTools::GetValueOptional(config, kObjectTypeKey, objectTypeStr)) {
    _objectType = ObjectTypeFromString(objectTypeStr);
  }
}
  
  
bool BehaviorAnimSequenceWithObject::WantsToBeActivatedBehavior() const
{
  return (GetLocatedObject() != nullptr);
}
  

void BehaviorAnimSequenceWithObject::OnBehaviorActivated()
{
  const auto* obj = GetLocatedObject();
  
  if (ANKI_VERIFY(obj != nullptr,
                  "BehaviorAnimSequenceWithObject.OnBehaviorActivated.NullObject",
                  "Null object!")) {
    // Attempt to turn toward the specified object, and even if fails, move on to the animations
    DelegateIfInControl(new TurnTowardsObjectAction(obj->GetID()), [this]() {
      BaseClass::StartPlayingAnimations();
    });
  }
}
  

const ObservableObject* BehaviorAnimSequenceWithObject::GetLocatedObject() const
{
  // Find matching objects
  BlockWorldFilter filter;
  if (_objectType != ObjectType::UnknownObject) {
    filter.AddAllowedType(_objectType);
  }
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto* object = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, filter);
  
  return object;
}

}
}
