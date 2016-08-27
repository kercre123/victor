/**
 * File: behaviorKnockOverCubes.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2016-08-01
 *
 * Description: Behavior to tip over a stack of cubes
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorKnockOverCubes.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/flipBlockAction.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/console/consoleInterface.h"




static const char* const kReachForBlockTrigger = "reachForBlockTrigger";
static const char* const kKnockOverEyesTrigger = "knockOverEyesTrigger";
static const char* const kKnockOverSuccessTrigger = "knockOverSuccessTrigger";
static const char* const kKnockOverFailureTrigger = "knockOverFailureTrigger";
static const char* const kPutDownTrigger = "knockOverPutDownTrigger";
static const char* const kMinimumStackHeight = "minimumStackHeight";

const int kMaxNumRetries = 1;
const int kMinThresholdRealign = 40;
const int kMinBlocksForSuccess = 2;
const float kWaitForBlockUpAxisChangeSecs = 0.5f;
CONSOLE_VAR(f32, kBKS_headAngleForKnockOver_deg, "Behavior.AdmireStack", -14.0f);
CONSOLE_VAR(f32, kBKS_distanceToTryToGrabFrom_mm, "Behavior.AdmireStack", 85.0f);
CONSOLE_VAR(f32, kBKS_searchSpeed_mmps, "Behavior.AdmireStack", 60.0f);

namespace Anki {
namespace Cozmo {
  
  
BehaviorKnockOverCubes::BehaviorKnockOverCubes(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _numRetries(0)
{
  SetDefaultName("KnockOverCubes");
  LoadConfig(config);
  
  SubscribeToTags({
    EngineToGameTag::ObjectUpAxisChanged
  });

}
  
void BehaviorKnockOverCubes::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  
  GetValueOptional(config, kReachForBlockTrigger, _reachForBlockTrigger);
  GetValueOptional(config, kKnockOverEyesTrigger, _knockOverEyesTrigger);
  GetValueOptional(config, kKnockOverSuccessTrigger, _knockOverSuccessTrigger);
  GetValueOptional(config, kKnockOverFailureTrigger, _knockOverFailureTrigger);
  GetValueOptional(config, kPutDownTrigger, _putDownAnimTrigger);
  
  _minStackHeight = config.get(kMinimumStackHeight, 3).asInt();
  
}

bool BehaviorKnockOverCubes::IsRunnableInternal(const Robot& robot) const
{
  UpdateTargetStack(robot);
  
  return _baseBlockID.IsSet() && (_stackHeight >= _minStackHeight) ;
}

Result BehaviorKnockOverCubes::InitInternal(Robot& robot)
{
  if(!_shouldStreamline){
    TransitionToReachingForBlock(robot);
  }else{
    TransitionToKnockingOverStack(robot);
  }
  
  InitializeMemberVars();

  return Result::RESULT_OK;
}
  
Result BehaviorKnockOverCubes::ResumeInternal(Robot& robot)
{
  if(IsRunnableInternal(robot)){
    TransitionToKnockingOverStack(robot);
    return Result::RESULT_OK;
  }
  
  InitializeMemberVars();
  
  return Result::RESULT_FAIL;
}

  
void BehaviorKnockOverCubes::StopInternal(Robot& robot)
{
  for(auto behaviorType: _disabledReactions){
    robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), behaviorType, true);
  }
  
  ResetBehavior(robot);
  
}
  
void BehaviorKnockOverCubes::TransitionToReachingForBlock(Robot& robot)
{
  DEBUG_SET_STATE(ReachingForBlock);
  
  ObservableObject* lastObj = robot.GetBlockWorld().GetObjectByID(_baseBlockID);
  ObservableObject* nextObj;
  
  BOUNDED_WHILE(10, nextObj = robot.GetBlockWorld().FindObjectOnTopOf(*lastObj, STACKED_HEIGHT_TOL_MM)){
    lastObj = nextObj;
  }
  
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  
  Pose3d poseWrtRobot;
  if(lastObj->GetPose().GetWithRespectTo(robot.GetPose(), poseWrtRobot) ) {
    const float fudgeFactor = 10.0f;
    if( poseWrtRobot.GetTranslation().x() + fudgeFactor > kBKS_distanceToTryToGrabFrom_mm) {
      float distToDrive = poseWrtRobot.GetTranslation().x() - kBKS_distanceToTryToGrabFrom_mm;
      action->AddAction(new DriveStraightAction(robot, distToDrive, kBKS_searchSpeed_mmps));
    }
  }
  
  action->AddAction(new TriggerLiftSafeAnimationAction(robot, _reachForBlockTrigger));
  StartActing(action, &BehaviorKnockOverCubes::TransitionToKnockingOverStack);
  
}
  

void BehaviorKnockOverCubes::TransitionToKnockingOverStack(Robot& robot)
{
  DEBUG_SET_STATE(KnockingOverStack);
  
  robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::AcknowledgeObject, false);
  _disabledReactions.insert(BehaviorType::AcknowledgeObject);
  
  auto flipCallback = [this, &robot](const ActionResult& result){
    if(result == ActionResult::SUCCESS){
      //Knocked over stack successfully
      TransitionToPlayingReaction(robot);
      
    }else if(result == ActionResult::FAILURE_RETRY){
      // Failed to knock over stack
      _numRetries += 1;
      if(_numRetries <= kMaxNumRetries){
        StartActing(new TriggerLiftSafeAnimationAction(robot, _knockOverFailureTrigger),
                    &BehaviorKnockOverCubes::TransitionToKnockingOverStack);
      }
    }
  };
  
  const bool sayNameBefore = !_shouldStreamline && _numRetries == 0;
  DriveAndFlipBlockAction* flipAction = new DriveAndFlipBlockAction(robot, _baseBlockID, false, 0, false, 0.f, sayNameBefore, kMinThresholdRealign);
  
  flipAction->SetSayNameAnimationTrigger(AnimationTrigger::KnockOverPreActionNamedFace);
  flipAction->SetNoNameAnimationTrigger(AnimationTrigger::KnockOverPreActionUnnamedFace);
  
  StartActing(new CompoundActionSequential(robot, {flipAction,
                                                   new WaitAction(robot, kWaitForBlockUpAxisChangeSecs)}),
              flipCallback);
}

void BehaviorKnockOverCubes::TransitionToPlayingReaction(Robot& robot)
{
  DEBUG_SET_STATE(PlayingReaction);

  // determine if the robot successfully knocked over the min number of cubes
  auto animationTrigger = _knockOverFailureTrigger;
  if(_objectsFlipped.size() >= kMinBlocksForSuccess){
    BehaviorObjectiveAchieved(BehaviorObjective::KnockedOverBlocks);
    animationTrigger = _knockOverSuccessTrigger;
  }
  
  if(!_shouldStreamline){
    
    StartActing(new TriggerLiftSafeAnimationAction(robot, animationTrigger),
                [this](Robot& robot){
                  robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::AcknowledgeObject, true);
                  _disabledReactions.erase(BehaviorType::AcknowledgeObject);
                });
  }
  
}
  
void BehaviorKnockOverCubes::InitializeMemberVars()
{
  // clear for success state check
  _objectsFlipped.clear();
  _numRetries = 0;
}

  
void BehaviorKnockOverCubes::ResetBehavior(Robot& robot)
{
  _disabledReactions.clear();
  _baseBlockID.UnSet();
}

void BehaviorKnockOverCubes::UpdateTargetStack(const Robot& robot) const
{
   _stackHeight = robot.GetBlockWorld().GetTallestStack(_baseBlockID);
}

void BehaviorKnockOverCubes::HandleObjectUpAxisChanged(const ObjectUpAxisChanged& msg, Robot& robot)
{
  _objectsFlipped.insert(msg.objectID);
}
  
void BehaviorKnockOverCubes::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch (event.GetData().GetTag()) {
    case ExternalInterface::MessageEngineToGameTag::ObjectUpAxisChanged:
      HandleObjectUpAxisChanged(event.GetData().Get_ObjectUpAxisChanged(), robot);
      break;
      
    default:
      PRINT_NAMED_ERROR("BehaviorKnockOverCubes.HandleWhileRunning.InvalidEvent", "");
      break;
  }
}
  
}
}
