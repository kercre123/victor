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

#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorPickUpCube.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/retryWrapperAction.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/blockWorld/blockWorld.h"


namespace Anki {
namespace Vector {

namespace{
const char* kPickupRetryCountKey = "retryCount";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPickUpCube::InstanceConfig::InstanceConfig()
{
  pickupRetryCount = 1;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPickUpCube::DynamicVariables::DynamicVariables()
{
  idSetExternally = false;
  pickupRetryCount = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPickUpCube::BehaviorPickUpCube(const Json::Value& config)
: ICozmoBehavior(config)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject,
  });

  if(config.isMember(kPickupRetryCountKey)){
    const std::string debug = "BehaviorPickupCube.Constructor.RetryParseIssue";
    _iConfig.pickupRetryCount = JsonTools::ParseUint8(config, kPickupRetryCountKey, debug);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kPickupRetryCountKey,
    kPickupRetryCountKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPickUpCube::WantsToBeActivatedBehavior() const
{
  if( _dVars.idSetExternally ) {
    return _dVars.targetBlockID.IsSet();
  } else {
    // check even if we haven't seen a block so that we can pickup blocks we know of
    // that are outside FOV
    ObjectID targetID;
    CalculateTargetID(targetID);
    return targetID.IsSet();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::OnBehaviorActivated()
{
  ObjectID potentiallyCarryoverID;
  if(_dVars.idSetExternally){
    potentiallyCarryoverID = _dVars.targetBlockID;
  }
  _dVars = DynamicVariables();

  if(potentiallyCarryoverID.IsSet()){
   _dVars.targetBlockID = potentiallyCarryoverID;
  }else{
    CalculateTargetID(_dVars.targetBlockID);
  }
  
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
void BehaviorPickUpCube::CalculateTargetID(ObjectID& outTargetID) const
{  
  auto& objInfoCache = GetAIComp<ObjectInteractionInfoCache>();
  const ObjectInteractionIntention intent = ObjectInteractionIntention::PickUpObjectNoAxisCheck;
  const ObjectID& possiblyBestObjID = objInfoCache.GetBestObjectForIntention(intent);

  outTargetID = possiblyBestObjID;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::TransitionToDoingInitialReaction()
{
  DEBUG_SET_STATE(DoingInitialReaction);
  
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::PickupCubePreperation),
                      &BehaviorPickUpCube::TransitionToPickingUpCube);

}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::TransitionToPickingUpCube()
{
  DEBUG_SET_STATE(PickingUpCube);
  
  DelegateIfInControl(new DriveToPickupObjectAction(_dVars.targetBlockID), [this](ActionResult result){
    if(result == ActionResult::SUCCESS){
      TransitionToSuccessReaction();
    }else if((IActionRunner::GetActionResultCategory(result) == ActionResultCategory::RETRY) &&
             (_dVars.pickupRetryCount < _iConfig.pickupRetryCount)){
      _dVars.pickupRetryCount++;
      TransitionToPickingUpCube();
    }else{
      auto& blockWorld = GetBEI().GetBlockWorld();
      const ObservableObject* pickupObj = blockWorld.GetLocatedObjectByID(_dVars.targetBlockID);
      if(pickupObj != nullptr){
        auto& whiteboard = GetAIComp<AIWhiteboard>();	
        whiteboard.SetFailedToUse(*pickupObj,	
                                  AIWhiteboard::ObjectActionFailure::PickUpObject);
      }
    }
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::TransitionToSuccessReaction()
{
  if(!ShouldStreamline()){
    DEBUG_SET_STATE(DoingFinalReaction);
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToBlockPickupSuccess));
  }
}

  
} // namespace Vector
} // namespace Anki

