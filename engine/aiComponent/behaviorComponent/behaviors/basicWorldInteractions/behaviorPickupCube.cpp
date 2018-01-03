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
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/blockWorld/blockConfigTypeHelpers.h"
#include "engine/blockWorld/blockConfiguration.h"
#include "engine/blockWorld/blockConfigurationManager.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageFromActiveObject.h"


namespace Anki {
namespace Cozmo {

namespace{
static const char* kBlockConfigsToIgnoreKey = "ignoreCubesInBlockConfigTypes";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPickUpCube::BehaviorPickUpCube(const Json::Value& config)
: ICozmoBehavior(config)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject,
  });
  
  // Pull from the config any block configuration types
  // that blocks should not be picked up out of
  const Json::Value& configsToIgnore  = config[kBlockConfigsToIgnoreKey];
  if(!configsToIgnore.isNull()){
    Json::Value::const_iterator configNameIt = configsToIgnore.begin();
    const Json::Value::const_iterator configNameEnd = configsToIgnore.end();
    
    for(; configNameIt != configNameEnd; ++configNameIt){
      _configurationsToIgnore.push_back(
          BlockConfigurations::BlockConfigurationFromString(configNameIt->asCString()));
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPickUpCube::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // check even if we haven't seen a block so that we can pickup blocks we know of
  // that are outside FOV  
  UpdateTargetBlocksInternal(behaviorExternalInterface);
  return _targetBlockID.IsSet();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(!ShouldStreamline()){
    TransitionToDoingInitialReaction(behaviorExternalInterface);
  }else{
    TransitionToPickingUpCube(behaviorExternalInterface);
  }
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(!IsActivated()){
    return;
  }

  // If the block we're going to pickup ever becomes part of an illegal configuration
  // immediately stop the behavior
  for(auto configType: _configurationsToIgnore) {
    if(behaviorExternalInterface.GetBlockWorld().GetBlockConfigurationManager()
                  .IsObjectPartOfConfigurationType(configType, _targetBlockID)){
      CancelSelf();
      return;
    }
  }  
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::UpdateTargetBlocksInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  _targetBlockID.UnSet();
  
  auto& objInfoCache = behaviorExternalInterface.GetAIComponent().GetObjectInteractionInfoCache();
  const ObjectInteractionIntention intent = ObjectInteractionIntention::PickUpObjectNoAxisCheck;
  const ObjectID& possiblyBestObjID = objInfoCache.GetBestObjectForIntention(intent);
  
  if(_configurationsToIgnore.empty())
  {
    _targetBlockID = possiblyBestObjID;
  }else{
    // Filter out blocks that are part of an ignore configuration
    const BlockWorldFilter& defaultFilter = objInfoCache.GetDefaultFilterForIntention( intent );
    
    BlockWorldFilter filter(defaultFilter);
    filter.AddFilterFcn(
      [&behaviorExternalInterface, this](const ObservableObject* object)
      {
        bool isPartOfIllegalConfiguration = false;
        for(auto configType: _configurationsToIgnore){
          const BlockWorld& blockWorld = behaviorExternalInterface.GetBlockWorld();
          if(blockWorld.GetBlockConfigurationManager()
                  .GetCacheByType(configType).AnyConfigContainsObject(object->GetID())){
            isPartOfIllegalConfiguration = true;
            break;
          }
        }

        return !isPartOfIllegalConfiguration;
      });
    
    // If the "best" object is valid, use that one - this allows tapped objects
    // to be given preference when valid
    std::vector<const ObservableObject *> validObjs;
    behaviorExternalInterface.GetBlockWorld().FindLocatedMatchingObjects(filter, validObjs);
    
    const ObservableObject* possiblyBestObj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(possiblyBestObjID);
    if(std::find(validObjs.begin(), validObjs.end(), possiblyBestObj) != validObjs.end()){
      _targetBlockID = possiblyBestObjID;
    }else{
      const ObservableObject* closestObject = behaviorExternalInterface.GetBlockWorld().
              FindLocatedObjectClosestTo(behaviorExternalInterface.GetRobotInfo().GetPose(), filter);
      
      if(closestObject != nullptr)
      {
        _targetBlockID = closestObject->GetID();
      }
    }
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::TransitionToDoingInitialReaction(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(DoingInitialReaction);
  
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(
                     AnimationTrigger::SparkPickupInitialCubeReaction),
              &BehaviorPickUpCube::TransitionToPickingUpCube);

}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::TransitionToPickingUpCube(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(PickingUpCube);
  
  auto& factory = behaviorExternalInterface.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  PickupBlockParamaters params;
  params.sayNameBeforePickup = !ShouldStreamline();
  params.maxTurnTowardsFaceAngle_rad = ShouldStreamline() ? 0 : M_PI_2;
  params.allowedToRetryFromDifferentPose = true;
  HelperHandle pickupHelper = factory.CreatePickupBlockHelper(behaviorExternalInterface, *this, _targetBlockID, params);
  SmartDelegateToHelper(behaviorExternalInterface, pickupHelper, &BehaviorPickUpCube::TransitionToSuccessReaction);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::TransitionToSuccessReaction(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(!ShouldStreamline()){
    DEBUG_SET_STATE(DoingFinalReaction);
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ReactToBlockPickupSuccess));
  }
}

  
} // namespace Cozmo
} // namespace Anki

