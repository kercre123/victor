/**
 * File: behaviorPickupCube.cpp
 *
 * Author: Molly Jameson
 * Created: 2016-07-26
 *
 * Description:  Behavior which picks up a cube
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorPickupCube.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/retryWrapperAction.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageFromActiveObject.h"


namespace Anki {
namespace Cozmo {

namespace{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPickUpCube::BehaviorPickUpCube(const Json::Value& config)
: ICozmoBehavior(config)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject,
  });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPickUpCube::WantsToBeActivatedBehavior() const
{
  // check even if we haven't seen a block so that we can pickup blocks we know of
  // that are outside FOV  
  UpdateTargetBlocks();
  return _targetBlockID.IsSet();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::OnBehaviorActivated()
{
  if(!ShouldStreamline()){
    TransitionToDoingInitialReaction();
  }else{
    TransitionToPickingUpCube();
  }
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::UpdateTargetBlocks() const
{
  _targetBlockID.UnSet();
  
  auto& objInfoCache = GetBEI().GetAIComponent().GetObjectInteractionInfoCache();
  const ObjectInteractionIntention intent = ObjectInteractionIntention::PickUpObjectNoAxisCheck;
  const ObjectID& possiblyBestObjID = objInfoCache.GetBestObjectForIntention(intent);

  _targetBlockID = possiblyBestObjID;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::TransitionToDoingInitialReaction()
{
  DEBUG_SET_STATE(DoingInitialReaction);
  
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(
                     AnimationTrigger::SparkPickupInitialCubeReaction),
              &BehaviorPickUpCube::TransitionToPickingUpCube);

}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::TransitionToPickingUpCube()
{
  DEBUG_SET_STATE(PickingUpCube);
  
  auto& factory = GetBEI().GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  PickupBlockParamaters params;
  params.sayNameBeforePickup = !ShouldStreamline();
  params.maxTurnTowardsFaceAngle_rad = ShouldStreamline() ? 0 : M_PI_2;
  params.allowedToRetryFromDifferentPose = true;
  HelperHandle pickupHelper = factory.CreatePickupBlockHelper(*this, _targetBlockID, params);
  SmartDelegateToHelper(pickupHelper, &BehaviorPickUpCube::TransitionToSuccessReaction);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::TransitionToSuccessReaction()
{
  if(!ShouldStreamline()){
    DEBUG_SET_STATE(DoingFinalReaction);
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ReactToBlockPickupSuccess));
  }
}

  
} // namespace Cozmo
} // namespace Anki

