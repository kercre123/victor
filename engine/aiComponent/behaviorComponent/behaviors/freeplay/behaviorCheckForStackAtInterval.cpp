/**
 * File: BehaviorCheckForStackAtInterval.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2016-16-11
 *
 * Description: Check all known cubes at a set interval to see 
 * if blocks have been stacked on top of them
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorCheckForStackAtInterval.h"

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/blockWorld/blockWorld.h"
#include "anki/common/basestation/utils/timer.h"

namespace {

static const char* const kDelayBetweenChecksKey_s = "delayBetweenChecks";
static const float kDefaultDelayBetweenChecks_s = 15.f;

}

namespace Anki {
namespace Cozmo {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCheckForStackAtInterval::BehaviorCheckForStackAtInterval(const Json::Value& config)
: ICozmoBehavior(config)
, _delayBetweenChecks_s(0)
, _nextCheckTime_s(0)
, _knownBlockIndex(-1)
, _ghostStackedObject(new Block_Cube1x1(ObjectType::Block_LIGHTCUBE_GHOST))
{
  
  _delayBetweenChecks_s = config.get(kDelayBetweenChecksKey_s, kDefaultDelayBetweenChecks_s).asFloat();

}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCheckForStackAtInterval::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(currTime > _nextCheckTime_s)
  {
    UpdateTargetBlocks(behaviorExternalInterface);
    const bool hasBlocks = !_knownBlockIDs.empty();
    return hasBlocks;
  }
  
  return false;
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  
  if(!_ghostStackedObject->GetID().IsSet()){
    _ghostStackedObject->SetID();
  }
  
  if(!_knownBlockIDs.empty()) {
    TransitionToSetup(behaviorExternalInterface);
    
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _knownBlockIndex = -1;
  _knownBlockIDs.clear();
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::TransitionToSetup(BehaviorExternalInterface& behaviorExternalInterface)
{
  const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  _initialRobotPose = robotInfo.GetPose();
  _knownBlockIndex = 0;
  TransitionToFacingBlock(behaviorExternalInterface);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::TransitionToFacingBlock(BehaviorExternalInterface& behaviorExternalInterface)
{
  const ObservableObject* obj = GetKnownObject(behaviorExternalInterface, _knownBlockIndex);
  if(nullptr != obj)
  {
    IActionRunner* action = new TurnTowardsObjectAction(obj->GetID());
    
    // check above even if turn action fails
    DelegateIfInControl(action, &BehaviorCheckForStackAtInterval::TransitionToCheckingAboveBlock);
  }
  else
  {
    TransitionToCheckingAboveBlock(behaviorExternalInterface);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::TransitionToCheckingAboveBlock(BehaviorExternalInterface& behaviorExternalInterface)
{
  const ObservableObject* obj = GetKnownObject(behaviorExternalInterface, _knownBlockIndex);
  if ( nullptr != obj )
  {
    Pose3d ghostPose = obj->GetPose().GetWithRespectToRoot();
    ghostPose.SetTranslation({
      ghostPose.GetTranslation().x(),
      ghostPose.GetTranslation().y(),
      ghostPose.GetTranslation().z() + obj->GetDimInParentFrame<'Z'>(ghostPose)});
    
    auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
    robotInfo.GetObjectPoseConfirmer().SetGhostObjectPose(_ghostStackedObject.get(), ghostPose, PoseState::Dirty);
    
    // use 0 for max turn angle so we only look with the head
    TurnTowardsObjectAction* turnAction = new TurnTowardsObjectAction(ObjectID{}, 0.f);
    turnAction->UseCustomObject(_ghostStackedObject.get());
    
    // after checking the object, go back to the rotation we had at start, or continue with next block if any left
    DelegateIfInControl(turnAction, [this, &behaviorExternalInterface](const ActionResult& result) {
      ++_knownBlockIndex;
      const bool hasMoreBlocks = _knownBlockIndex < _knownBlockIDs.size();
      if (hasMoreBlocks) {
        TransitionToFacingBlock(behaviorExternalInterface);
      } else {
        TransitionToReturnToSearch(behaviorExternalInterface);
      }
    });
  }
  else
  {
    PRINT_CH_INFO("Behaviors",
                  "BehaviorCheckForStackAtInterval.TransitionToCheckingAboveBlock.NullObject",
                  "Could not get object from index:%d, ID:%d. The list may be dirty or the cube disconnected. Ending prematurely",
                  _knownBlockIndex,
                  GetKnownObjectID(_knownBlockIndex).GetValue());
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::TransitionToReturnToSearch(BehaviorExternalInterface& behaviorExternalInterface)
{
  const Radians initialRobotRotation = _initialRobotPose.GetWithRespectToRoot()
                                           .GetRotation().GetAngleAroundZaxis();

  IActionRunner* action = new PanAndTiltAction(initialRobotRotation, 0, true, false);
  DelegateIfInControl(action, [this](){
    _nextCheckTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _delayBetweenChecks_s;
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID BehaviorCheckForStackAtInterval::GetKnownObjectID(int index) const
{
  // validate index
  if ( index < 0 || index >= _knownBlockIDs.size() ) {
    PRINT_NAMED_ERROR("BehaviorCheckForStackAtInterval.GetKnownObject.InvalidIndex",
    "%d is not within range of known objects [0,%zu]", index, _knownBlockIDs.size());
    return ObjectID();
  }
  
  const ObjectID& retID = _knownBlockIDs[index];
  return retID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObservableObject* BehaviorCheckForStackAtInterval::GetKnownObject(const BehaviorExternalInterface& behaviorExternalInterface, int index) const
{
  // get id from the given index
  const ObjectID& objID = GetKnownObjectID(index);
  if ( objID.IsSet() ) {
    // find object currently in world
    const ObservableObject* ret = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID( objID );
    return ret;
  } else {
    return nullptr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::UpdateTargetBlocks(const BehaviorExternalInterface& behaviorExternalInterface) const
{
  
  BlockWorldFilter knownBlockFilter;
  knownBlockFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
 
  std::vector<const ObservableObject*> objectList;
  behaviorExternalInterface.GetBlockWorld().FindLocatedMatchingObjects(knownBlockFilter, objectList);
  for( const auto& objPtr : objectList ) {
    _knownBlockIDs.emplace_back( objPtr->GetID() );
  }
}


}
}
