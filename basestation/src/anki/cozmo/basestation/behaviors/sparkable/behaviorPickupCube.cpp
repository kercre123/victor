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

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorPickupCube.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/objectInteractionInfoCache.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigTypeHelpers.h"
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageFromActiveObject.h"


namespace Anki {
namespace Cozmo {

namespace{
static const char* kBlockConfigsToIgnoreKey = "ignoreCubesInBlockConfigTypes";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPickUpCube::BehaviorPickUpCube(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("BehaviorPickUpCube");

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
bool BehaviorPickUpCube::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const Robot& robot = preReqData.GetRobot();
  // check even if we haven't seen a block so that we can pickup blocks we know of
  // that are outside FOV
  
  UpdateTargetBlocksInternal(robot);
  return _targetBlockID.IsSet();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorPickUpCube::InitInternal(Robot& robot)
{
  if(!_shouldStreamline){
    TransitionToDoingInitialReaction(robot);
  }else{
    TransitionToPickingUpCube(robot);
  }
  return Result::RESULT_OK;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorPickUpCube::UpdateInternal(Robot& robot)
{
  // If the block we're going to pickup ever becomes part of an illegal configuration
  // immediately stop the behavior
  for(auto configType: _configurationsToIgnore) {
    if(robot.GetBlockWorld().GetBlockConfigurationManager()
                  .IsObjectPartOfConfigurationType(configType, _targetBlockID)){
      StopWithoutImmediateRepetitionPenalty();
      return Status::Complete;
    }
  }
  
  return super::UpdateInternal(robot);
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::UpdateTargetBlocksInternal(const Robot& robot) const
{
  _targetBlockID.UnSet();
  
  auto& objInfoCache = robot.GetAIComponent().GetObjectInteractionInfoCache();
  const ObjectInteractionIntention intent = ObjectInteractionIntention::PickUpAnyObject;
  const ObjectID& possiblyBestObjID = objInfoCache.GetBestObjectForIntention(intent);
  
  if(_configurationsToIgnore.empty())
  {
    _targetBlockID = possiblyBestObjID;
  }else{
    // Filter out blocks that are part of an ignore configuration
    const BlockWorldFilter& defaultFilter = objInfoCache.GetDefaultFilterForIntention( intent );
    
    BlockWorldFilter filter(defaultFilter);
    filter.AddFilterFcn(
      [&robot, this](const ObservableObject* object)
      {
        bool isPartOfIllegalConfiguration = false;
        for(auto configType: _configurationsToIgnore){
          if(robot.GetBlockWorld().GetBlockConfigurationManager()
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
    robot.GetBlockWorld().FindLocatedMatchingObjects(filter, validObjs);
    
    const ObservableObject* possiblyBestObj = robot.GetBlockWorld().GetLocatedObjectByID(possiblyBestObjID);
    if(std::find(validObjs.begin(), validObjs.end(), possiblyBestObj) != validObjs.end()){
      _targetBlockID = possiblyBestObjID;
    }else{
      const ObservableObject* closestObject = robot.GetBlockWorld().
                                FindLocatedObjectClosestTo(robot.GetPose(), filter);
      
      if(closestObject != nullptr)
      {
        _targetBlockID = closestObject->GetID();
      }
    }
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::TransitionToDoingInitialReaction(Robot& robot)
{
  DEBUG_SET_STATE(DoingInitialReaction);
  
  StartActing(new TriggerLiftSafeAnimationAction(robot,
                     AnimationTrigger::SparkPickupInitialCubeReaction),
              &BehaviorPickUpCube::TransitionToPickingUpCube);

}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::TransitionToPickingUpCube(Robot& robot)
{
  DEBUG_SET_STATE(PickingUpCube);
  
  auto& factory = robot.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  PickupBlockParamaters params;
  params.sayNameBeforePickup = !_shouldStreamline;
  params.allowedToRetryFromDifferentPose = true;
  HelperHandle pickupHelper = factory.CreatePickupBlockHelper(robot, *this, _targetBlockID, params);
  SmartDelegateToHelper(robot, pickupHelper, &BehaviorPickUpCube::TransitionToSuccessReaction);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpCube::TransitionToSuccessReaction(Robot& robot)
{
  if(!_shouldStreamline){
    DEBUG_SET_STATE(DoingFinalReaction);
    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::ReactToBlockPickupSuccess));
  }
}

  
} // namespace Cozmo
} // namespace Anki

