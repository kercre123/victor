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
, _knownBlockIndex(0)
, _ghostStackedObject(new ActiveCube(ObservableObject::InvalidActiveID,
                                     ObservableObject::InvalidFactoryID,
                                     ActiveObjectType::OBJECT_CUBE1))
{
  SetDefaultName("CheckForStackAtInterval");
  
  _delayBetweenChecks_s = config.get(kDelayBetweenChecksKey_s, kDefaultDelayBetweenChecks_s).asFloat();

}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCheckForStackAtInterval::IsRunnableInternal(const Robot& robot) const
{
  const float currTime =  BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(currTime > _nextCheckTime_s){
    UpdateTargetBlocks(robot);
    return !_knownBlocks.empty();
  }
  
  return false;
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorCheckForStackAtInterval::InitInternal(Robot& robot)
{
  
  if(!_ghostStackedObject->GetID().IsSet()){
    _ghostStackedObject->SetID();
  }
  
  if(!_knownBlocks.empty()){
    TransitionToSetup(robot);
    return Result::RESULT_OK;
  }
  
  return Result::RESULT_FAIL;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::StopInternal(Robot& robot)
{
  _knownBlocks.clear();
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
  IActionRunner* action = new TurnTowardsObjectAction(robot,
                                  _knownBlocks[_knownBlockIndex]->GetID(),
                                  M_PI);
  
  StartActing(action, &BehaviorCheckForStackAtInterval::TransitionToCheckingAboveBlock);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCheckForStackAtInterval::TransitionToCheckingAboveBlock(Robot& robot)
{
  const ObservableObject* obj = _knownBlocks[_knownBlockIndex];
  Pose3d ghostPose = obj->GetPose().GetWithRespectToOrigin();
  ghostPose.SetTranslation({
    ghostPose.GetTranslation().x(),
    ghostPose.GetTranslation().y(),
    ghostPose.GetTranslation().z() + obj->GetSize().z()});
  
  robot.GetObjectPoseConfirmer().AddObjectRelativeObservation(_ghostStackedObject.get(), ghostPose, obj);
  
  // use 0 for max turn angle so we only look with the head
  TurnTowardsObjectAction* turnAction = new TurnTowardsObjectAction(robot, ObjectID{}, 0.f);
  turnAction->UseCustomObject(_ghostStackedObject.get());
  
  StartActing(turnAction, [this, &robot](const ActionResult& result){
    _knownBlockIndex += 1;
    if(_knownBlocks.size() <= _knownBlockIndex){
      TransitionToReturnToSearch(robot);
    }else{
      TransitionToFacingBlock(robot);
    }
    
  });
  
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
void BehaviorCheckForStackAtInterval::UpdateTargetBlocks(const Robot& robot) const
{
  
  BlockWorldFilter knownBlockFilter;
  knownBlockFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  knownBlockFilter.AddFilterFcn([](const ObservableObject* blockPtr)
                                 {
                                   if(!blockPtr->IsPoseStateKnown()){
                                     return false;
                                   }
                                   
                                   return true;
                                 });
  
  robot.GetBlockWorld().FindMatchingObjects(knownBlockFilter, _knownBlocks);
}


}
}
