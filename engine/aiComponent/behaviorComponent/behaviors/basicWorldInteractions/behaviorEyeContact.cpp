/**
* File: behaviorEyeContact.cpp
*
* Author: Robert Cosgriff
* Created: 2/5/18
*
* Description: Simple behavior to react to eye contact
*
* Copyright: Anki, Inc. 2017
*
**/

// TODO these need to stripped down, let's get it compiling 
// and then start to get rid of the one that aren't needed
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorEyeContact.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/faceWorld.h"

#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/imageTypes.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/bodyLightComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"

#include "util/fileUtils/fileUtils.h"

namespace Anki {
namespace Cozmo {

BehaviorEyeContact::BehaviorEyeContact(const Json::Value& config)
  : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEyeContact::WantsToBeActivatedBehavior() const
{
  return GetBEI().GetFaceWorld().IsMakingEyeContact();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEyeContact::OnBehaviorActivated()
{
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::HikingSquintStart));
}

} // namespace Cozmo
} // namespace Anki
