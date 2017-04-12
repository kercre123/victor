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

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorCheckForStackAtInterval.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"

namespace {

static const char* const kDelayBetweenChecksKey_s = "delayBetweenChecks";
static const float kDefaultDelayBetweenChecks_s = 15.f;

}

namespace Anki {
namespace Cozmo {
  
  
BehaviorCheckForStackAtInterval::BehaviorCheckForStackAtInterval(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _delayBetweenChecks_s(0)
, _nextCheckTime_s(0)
, _knownBlockIndex(-1)
, _ghostStackedObject(new Block_Cube1x1(ObjectType::Block_LIGHTCUBE_GHOST))
{
  SetDefaultName("CheckForStackAtInterval");
  
  _delayBetweenChecks_s = config.get(kDelayBetweenChecksKey_s, kDefaultDelayBetweenChecks_s).asFloat();

}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCheckForStackAtInterval::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(currTime > _nextCheckTime_s)
  {
    UpdateTargetBlocks(preReqData.GetRobot());
    const bool hasBlocks = !_knownBlockIDs.empty();
    return hasBlocks;
  }
  
  return false;
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorCheckForStackAtInterval::InitInternal(Robot& robot)
{
  
  if(!_ghostStackedObject->GetID().IsSet()){
    _ghostStackedObject->SetID();
  }
  
  if(!_knownBlockIDs.empty()) {
    TransitionToSetup(robot);
    return Result::RESULT_OK;
  }
  
  return Result::RESULT_FAIL;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::StopInternal(Robot& robot)
{
  _knownBlockIndex = -1;
  _knownBlockIDs.clear();
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::TransitionToSetup(Robot& robot)
{
  _initialRobotPose = robot.GetPose();
  _knownBlockIndex = 0;
  TransitionToFacingBlock(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::TransitionToFacingBlock(Robot& robot)
{
  const ObservableObject* obj = GetKnownObject(robot, _knownBlockIndex);
  if(nullptr != obj)
  {
    IActionRunner* action = new TurnTowardsObjectAction(robot, obj->GetID());
    
    // check above even if turn action fails
    StartActing(action, &BehaviorCheckForStackAtInterval::TransitionToCheckingAboveBlock);
  }
  else
  {
    TransitionToCheckingAboveBlock(robot);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::TransitionToCheckingAboveBlock(Robot& robot)
{
  const ObservableObject* obj = GetKnownObject(robot, _knownBlockIndex);
  if ( nullptr != obj )
  {
    Pose3d ghostPose = obj->GetPose().GetWithRespectToOrigin();
    ghostPose.SetTranslation({
      ghostPose.GetTranslation().x(),
      ghostPose.GetTranslation().y(),
      ghostPose.GetTranslation().z() + obj->GetDimInParentFrame<'Z'>(ghostPose)});
    
    robot.GetObjectPoseConfirmer().SetGhostObjectPose(_ghostStackedObject.get(), ghostPose, PoseState::Dirty);
    
    // use 0 for max turn angle so we only look with the head
    TurnTowardsObjectAction* turnAction = new TurnTowardsObjectAction(robot, ObjectID{}, 0.f);
    turnAction->UseCustomObject(_ghostStackedObject.get());
    
    // after checking the object, go back to the rotation we had at start, or continue with next block if any left
    StartActing(turnAction, [this, &robot](const ActionResult& result) {
      ++_knownBlockIndex;
      const bool hasMoreBlocks = _knownBlockIndex < _knownBlockIDs.size();
      if (hasMoreBlocks) {
        TransitionToFacingBlock(robot);
      } else {
        TransitionToReturnToSearch(robot);
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
void BehaviorCheckForStackAtInterval::TransitionToReturnToSearch(Robot& robot)
{
  const Radians initialRobotRotation = _initialRobotPose.GetWithRespectToOrigin()
                                           .GetRotation().GetAngleAroundZaxis();
  IActionRunner* action = new PanAndTiltAction(robot, initialRobotRotation, 0, true, false);
  StartActing(action, [this](){
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
const ObservableObject* BehaviorCheckForStackAtInterval::GetKnownObject(const Robot& robot, int index) const
{
  // get id from the given index
  const ObjectID& objID = GetKnownObjectID(index);
  if ( objID.IsSet() ) {
    // find object currently in world
    const ObservableObject* ret = robot.GetBlockWorld().GetLocatedObjectByID( objID );
    return ret;
  } else {
    return nullptr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::UpdateTargetBlocks(const Robot& robot) const
{
  
  BlockWorldFilter knownBlockFilter;
  knownBlockFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
 
  std::vector<const ObservableObject*> objectList;
  robot.GetBlockWorld().FindLocatedMatchingObjects(knownBlockFilter, objectList);
  for( const auto& objPtr : objectList ) {
    _knownBlockIDs.emplace_back( objPtr->GetID() );
  }
}


}
}
