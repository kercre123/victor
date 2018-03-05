/**
* File: behaviorObservingEyeContact.cpp
*
* Author: Robert Cosgriff
* Created: 2/5/18
*
* Description: Simple behavior to react to eye contact
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/behaviorComponent/behaviors/observing/behaviorObservingEyeContact.h"

#include "engine/actions/animActions.h"
#include "engine/faceWorld.h"

#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Cozmo {

BehaviorObservingEyeContact::BehaviorObservingEyeContact(const Json::Value& config)
  : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorObservingEyeContact::WantsToBeActivatedBehavior() const
{
  return GetBEI().GetFaceWorld().IsMakingEyeContact();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorObservingEyeContact::OnBehaviorActivated()
{
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::HikingSquintStart));
}

} // namespace Cozmo
} // namespace Anki
