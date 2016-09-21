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
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
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
static const char* const kIsReactionaryConfigFlag = "isReactionary";

const int kMaxNumRetries = 1;
const int kMinThresholdRealign = 10;
const int kMinBlocksForSuccess = 1;
const float kWaitForBlockUpAxisChangeSecs = 0.5f;
const f32 kBSB_MaxTurnTowardsFaceBeforeKnockStack_rad = RAD_TO_DEG_F32(90.f);

CONSOLE_VAR(f32, kBKS_headAngleForKnockOver_deg, "Behavior.AdmireStack", -14.0f);
CONSOLE_VAR(f32, kBKS_distanceToTryToGrabFrom_mm, "Behavior.AdmireStack", 85.0f);
CONSOLE_VAR(f32, kBKS_searchSpeed_mmps, "Behavior.AdmireStack", 60.0f);

namespace Anki {
namespace Cozmo {
  
  
BehaviorKnockOverCubes::BehaviorKnockOverCubes(Robot& robot, const Json::Value& config)
  : IReactionaryBehavior(robot, config)
  , _objectObservedChanged(false)
  , _lastObservedObject(-1)
  , _numRetries(0)
  , _isReactionary(true)
{
  SetDefaultName("KnockOverCubes");
  LoadConfig(config);
  
  SubscribeToTags({{
    EngineToGameTag::ObjectUpAxisChanged,
    EngineToGameTag::RobotObservedObject
  }});

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
  _isReactionary = config.get(kIsReactionaryConfigFlag, true).asBool();
  
}

bool BehaviorKnockOverCubes::IsRunnableInternalReactionary(const Robot& robot) const
{
  if(robot.GetBehaviorManager().GetActiveSpark() == UnlockId::KnockOverThreeCubeStack
     && IsReactionary()){
    return false;
  }
  
  if(_objectObservedChanged){
    _objectObservedChanged = false;
    UpdateTargetStack(robot);
    return CheckIfRunnable();
  }
  
  return false;
}

bool BehaviorKnockOverCubes::CheckIfRunnable() const
{
  return _baseBlockID.IsSet() && (_stackHeight >= _minStackHeight);
}
  
bool BehaviorKnockOverCubes::IsReactionary() const
{
  return _isReactionary;
}

  
Result BehaviorKnockOverCubes::InitInternalReactionary(Robot& robot)
{
  // determine the cubes actually in the stack
  _objectsInStack.clear();
  BlockWorldFilter blocksOnlyFilter;
  blocksOnlyFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});

  ASSERT_NAMED_EVENT(_baseBlockID.IsSet(), "BehaviorKnockOverCubes.InitInternalReactionary", "BaseBlockIDNotSet");
  _objectsInStack.insert(_baseBlockID);
  auto currentBlock = robot.GetBlockWorld().GetObjectByID(_baseBlockID);
  if(currentBlock){
    auto nextBlock = currentBlock;
    BOUNDED_WHILE(10,(nextBlock = robot.GetBlockWorld().FindObjectOnTopOf(*currentBlock, BlockWorld::kOnCubeStackHeightTolerence, blocksOnlyFilter))){
      _objectsInStack.insert(nextBlock->GetID());
      currentBlock = nextBlock;
    }
  }else{
    return Result::RESULT_FAIL;
  }
  
  SmartDisableReactionaryBehavior(BehaviorType::ReactToCubeMoved);

  if(!_shouldStreamline){
    TransitionToReachingForBlock(robot);
  }else{
    TransitionToKnockingOverStack(robot);
  }
  
  InitializeMemberVars();

  return Result::RESULT_OK;
}

  
void BehaviorKnockOverCubes::StopInternalReactionary(Robot& robot)
{
  ResetBehavior(robot);
}
  
bool BehaviorKnockOverCubes::ShouldComputationallySwitch(const Robot& robot)
{
  // Variables set in IsRunnable
  return CheckIfRunnable();
}
  
void BehaviorKnockOverCubes::TransitionToReachingForBlock(Robot& robot)
{
  DEBUG_SET_STATE(ReachingForBlock);
  
  ObservableObject* lastObj = robot.GetBlockWorld().GetObjectByID(_baseBlockID);
  if(nullptr == lastObj)
  {
    PRINT_NAMED_WARNING("BehaviorKnockOverCubes.TransitionToReachingForBlock.BaseBlockNull",
                        "BaseBlockID=%d", _baseBlockID.GetValue());
    _baseBlockID.UnSet();
    return;
  }
  
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
  
  auto flipCallback = [this, &robot](const ActionResult& result){
    if(result == ActionResult::SUCCESS){
      //Knocked over stack successfully
      TransitionToPlayingReaction(robot);
    }
  };
  
  //skips turning towards face if this action is streamlined
  const f32 angleTurnTowardsFace_rad = !_shouldStreamline ? kBSB_MaxTurnTowardsFaceBeforeKnockStack_rad : 0;
  
  DriveAndFlipBlockAction* flipAction = new DriveAndFlipBlockAction(robot, _baseBlockID, false, 0, false, angleTurnTowardsFace_rad, false, kMinThresholdRealign);
  
  flipAction->SetSayNameAnimationTrigger(AnimationTrigger::KnockOverPreActionNamedFace);
  flipAction->SetNoNameAnimationTrigger(AnimationTrigger::KnockOverPreActionUnnamedFace);
  
  RetryWrapperAction::RetryCallback retryCallback = [this, flipAction](const ExternalInterface::RobotCompletedAction& completion,
                                                                       const u8 retryCount,
                                                                       AnimationTrigger& retryAnimTrigger)
  {
    retryAnimTrigger = _knockOverFailureTrigger;
    
    flipAction->SetMaxTurnTowardsFaceAngle(0);

    // Use a different preAction pose if we are retrying
    flipAction->GetDriveToObjectAction()->SetGetPossiblePosesFunc([this, flipAction](ActionableObject* object,
                                                                                     std::vector<Pose3d>& possiblePoses,
                                                                                     bool& alreadyInPosition)
    {
      return IBehavior::UseSecondClosestPreActionPose(flipAction->GetDriveToObjectAction(),object, possiblePoses, alreadyInPosition);
    });
    
    return true;
    };

  CompoundActionSequential* flipAndWaitAction = new CompoundActionSequential(robot);

  {
    RetryWrapperAction* action = new RetryWrapperAction(robot, flipAction, retryCallback, kMaxNumRetries);
    // emit completion signal so that the mood manager can react
    const bool shouldEmitCompletion = true;
    flipAndWaitAction->AddAction(action, false, shouldEmitCompletion);
  }

  flipAndWaitAction->AddAction(new WaitAction(robot, kWaitForBlockUpAxisChangeSecs));

  // make sure we only account for blocks flipped during the actual knock over action
  _objectsFlipped.clear();
  StartActing(flipAndWaitAction, flipCallback);
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
  
  // play a reaction if not streamlined
  if(!_shouldStreamline){
    StartActing(new TriggerLiftSafeAnimationAction(robot, animationTrigger));
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
  _baseBlockID.UnSet();
}

void BehaviorKnockOverCubes::UpdateTargetStack(const Robot& robot) const
{
   _stackHeight = robot.GetBlockWorld().GetTallestStack(_baseBlockID);
}

void BehaviorKnockOverCubes::HandleObjectUpAxisChanged(const ObjectUpAxisChanged& msg, Robot& robot)
{
  if(_objectsInStack.find(msg.objectID) != _objectsInStack.end()){
    _objectsFlipped.insert(msg.objectID);
  }
}
  
void BehaviorKnockOverCubes::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch (event.GetData().GetTag()) {
    case ExternalInterface::MessageEngineToGameTag::ObjectUpAxisChanged:
      HandleObjectUpAxisChanged(event.GetData().Get_ObjectUpAxisChanged(), robot);
      break;
      
    case ExternalInterface::MessageEngineToGameTag::RobotObservedObject:
      // Handled in always handle
      break;
      
    default:
      PRINT_NAMED_ERROR("BehaviorKnockOverCubes.HandleWhileRunning.InvalidEvent", "");
      break;
  }
}
  
void BehaviorKnockOverCubes::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{
  switch (event.GetData().GetTag()) {
    case ExternalInterface::MessageEngineToGameTag::ObjectUpAxisChanged:
      // handled only while running
      break;
      
    case ExternalInterface::MessageEngineToGameTag::RobotObservedObject:
      if(event.GetData().Get_RobotObservedObject().objectID != _lastObservedObject){
        _lastObservedObject = event.GetData().Get_RobotObservedObject().objectID;
        _objectObservedChanged = true;
      }
      break;
      
    default:
      PRINT_NAMED_ERROR("BehaviorKnockOverCubes.AlwaysHandleInternal.InvalidEvent", "");
      break;
  }
}

  
}
}
