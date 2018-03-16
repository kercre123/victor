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

#include "engine/behaviorSystem/behaviors/basicWorldInteractions/behaviorKnockOverCubes.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/flipBlockAction.h"
#include "engine/actions/retryWrapperAction.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/blockWorld/blockConfigurationManager.h"
#include "engine/blockWorld/blockConfigurationStack.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/events/animationTriggerHelpers.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/common/basestation/jsonTools.h"
#include "engine/robot.h"
#include "util/console/consoleInterface.h"



namespace Anki {
namespace Cozmo {
  
namespace {

static const char* const kReachForBlockTrigger = "reachForBlockTrigger";
static const char* const kKnockOverEyesTrigger = "knockOverEyesTrigger";
static const char* const kKnockOverSuccessTrigger = "knockOverSuccessTrigger";
static const char* const kKnockOverFailureTrigger = "knockOverFailureTrigger";
static const char* const kPutDownTrigger = "knockOverPutDownTrigger";
static const char* const kMinimumStackHeight = "minimumStackHeight";
static const char* const kPreparingToKnockOverStackLock = "preparingToKnockOverDisable";


const int kMaxNumRetries = 2;
const float kMinThresholdRealign = 20.f;
const int kMinBlocksForSuccess = 1;
const float kWaitForBlockUpAxisChangeSecs = 0.5f;
const f32 kBSB_MaxTurnTowardsFaceBeforeKnockStack_rad = DEG_TO_RAD(90.f);

const float kScoreIncreaseSoNoRoll = 10.f;



CONSOLE_VAR(f32, kBKS_distanceToTryToGrabFrom_mm, "Behavior.AdmireStack", 85.0f);
CONSOLE_VAR(f32, kBKS_searchSpeed_mmps, "Behavior.AdmireStack", 60.0f);
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr ReactionTriggerHelpers::FullReactionArray kKnockOverCubesAffectedArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           false},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kKnockOverCubesAffectedArray),
              "Reaction triggers duplicate or non-sequential");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersPreparingKnockOverArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    false},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           false},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersPreparingKnockOverArray),
              "Reaction triggers duplicate or non-sequential");

} // end namespace

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKnockOverCubes::BehaviorKnockOverCubes(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _numRetries(0)
{
  LoadConfig(config);
  
  SubscribeToTags({
    EngineToGameTag::ObjectUpAxisChanged,
  });
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorKnockOverCubes::IsRunnableInternal(const Robot& robot) const
{
  UpdateTargetStack(robot);
  if(auto tallestStack = _currentTallestStack.lock()){
    return tallestStack->GetStackHeight() >= _minStackHeight;
  }
  
  return false;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorKnockOverCubes::InitInternal(Robot& robot)
{
  if(InitializeMemberVars()){
    if(!ShouldStreamline()){
      TransitionToReachingForBlock(robot);
    }else{
      TransitionToKnockingOverStack(robot);
    }
    return Result::RESULT_OK;
  }else{
    return Result::RESULT_FAIL;
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorKnockOverCubes::ResumeInternal(Robot& robot)
{
  if(InitializeMemberVars()){
    TransitionToKnockingOverStack(robot);
    return Result::RESULT_OK;
  }else{
    return Result::RESULT_FAIL;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::StopInternal(Robot& robot)
{
  ClearStack();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::TransitionToReachingForBlock(Robot& robot)
{
  DEBUG_SET_STATE(ReachingForBlock);

  const ObservableObject* topBlock = robot.GetBlockWorld().GetLocatedObjectByID(_topBlockID);
  
  if(topBlock == nullptr){
    ClearStack();
    return;
  }
  
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  
  action->AddAction(new TurnTowardsObjectAction(robot, _bottomBlockID));
  
  Pose3d poseWrtRobot;
  if(topBlock->GetPose().GetWithRespectTo(robot.GetPose(), poseWrtRobot) ) {
    const float fudgeFactor = 10.0f;
    if( poseWrtRobot.GetTranslation().x() + fudgeFactor > kBKS_distanceToTryToGrabFrom_mm) {
      float distToDrive = poseWrtRobot.GetTranslation().x() - kBKS_distanceToTryToGrabFrom_mm;
      action->AddAction(new DriveStraightAction(robot, distToDrive, kBKS_searchSpeed_mmps));
    }
  }
  
  action->AddAction(new TriggerLiftSafeAnimationAction(robot, _reachForBlockTrigger));
  StartActing(action, &BehaviorKnockOverCubes::TransitionToKnockingOverStack);
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::TransitionToKnockingOverStack(Robot& robot)
{
  DEBUG_SET_STATE(KnockingOverStack);
  
  auto flipCallback = [this, &robot](const ActionResult& result){
    // Mark the object as having no preaction poses - there's nothing else we can do
    if(result == ActionResult::NO_PREACTION_POSES){
      robot.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_bottomBlockID);
      return;
    }
    
    const ActionResultCategory resCat = IActionRunner::GetActionResultCategory(result);
    if(resCat == ActionResultCategory::SUCCESS){
      //Knocked over stack successfully
      TransitionToPlayingReaction(robot);
    }else if(resCat == ActionResultCategory::RETRY){
      //Assume we had an alignment issue
      if(_numRetries < kMaxNumRetries){
        TransitionToKnockingOverStack(robot);
      }else{
        // We've aligned a bunch of times - just go for it
        TransitionToBlindlyFlipping(robot);
      }
      _numRetries++;
    }
  };
  
  // Setup the flip action
  
  //skips turning towards face if this action is streamlined
  const f32 angleTurnTowardsFace_rad = (ShouldStreamline() || _numRetries > 0) ? 0 : kBSB_MaxTurnTowardsFaceBeforeKnockStack_rad;
  
  DriveAndFlipBlockAction* flipAction = new DriveAndFlipBlockAction(robot, _bottomBlockID, false, 0, false, angleTurnTowardsFace_rad, false, kMinThresholdRealign);
  
  flipAction->SetSayNameAnimationTrigger(AnimationTrigger::KnockOverPreActionNamedFace);
  flipAction->SetNoNameAnimationTrigger(AnimationTrigger::KnockOverPreActionUnnamedFace);
  
  
  // Set the action sequence
  CompoundActionSequential* flipAndWaitAction = new CompoundActionSequential(robot);
  flipAndWaitAction->AddAction(new TurnTowardsObjectAction(robot, _bottomBlockID));
  // emit completion signal so that the mood manager can react
  const bool shouldEmitCompletion = true;
  flipAndWaitAction->AddAction(flipAction, false, shouldEmitCompletion);
  flipAndWaitAction->AddAction(new WaitAction(robot, kWaitForBlockUpAxisChangeSecs));
  
  // make sure we only account for blocks flipped during the actual knock over action
  PrepareForKnockOverAttempt();
  StartActing(flipAndWaitAction, flipCallback);
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::TransitionToBlindlyFlipping(Robot& robot)
{
  CompoundActionSequential* flipAndWaitAction = new CompoundActionSequential(robot);
  {
    FlipBlockAction* flipAction = new FlipBlockAction(robot, _bottomBlockID);
    flipAction->SetShouldCheckPreActionPose(false);
  
    flipAndWaitAction->AddAction(flipAction);
    flipAndWaitAction->AddAction(new WaitAction(robot, kWaitForBlockUpAxisChangeSecs));
  }
  
  PrepareForKnockOverAttempt();
  StartActing(flipAndWaitAction, &BehaviorKnockOverCubes::TransitionToPlayingReaction);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::TransitionToPlayingReaction(Robot& robot)
{
  DEBUG_SET_STATE(PlayingReaction);
  
  // notify configuration manager that the tower was knocked over
  robot.GetBlockWorld().GetBlockConfigurationManager().FlagForRebuild();
  
  // determine if the robot successfully knocked over the min number of cubes
  auto animationTrigger = _knockOverFailureTrigger;
  if(_objectsFlipped.size() >= kMinBlocksForSuccess){
    BehaviorObjectiveAchieved(BehaviorObjective::KnockedOverBlocks);
    NeedActionCompleted();
    animationTrigger = _knockOverSuccessTrigger;
  }
  
  // play a reaction if not streamlined
  if(!ShouldStreamline()){
    StartActing(new TriggerLiftSafeAnimationAction(robot, animationTrigger));
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorKnockOverCubes::InitializeMemberVars()
{
  if(auto tallestStack = _currentTallestStack.lock()){
  // clear for success state check
    SmartDisableReactionsWithLock(GetIDStr(), kKnockOverCubesAffectedArray);
    _objectsFlipped.clear();
    _numRetries = 0;
    _bottomBlockID = tallestStack->GetBottomBlockID();
    _middleBlockID = tallestStack->GetMiddleBlockID();
    _topBlockID = tallestStack->GetTopBlockID();
    return true;
  }else{
    return false;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::ClearStack()
{
  _currentTallestStack = {};
  _bottomBlockID.SetToUnknown();
  _middleBlockID.SetToUnknown();
  _topBlockID.SetToUnknown();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::UpdateTargetStack(const Robot& robot) const
{
   _currentTallestStack = robot.GetBlockWorld().GetBlockConfigurationManager().GetStackCache().GetTallestStack();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::HandleObjectUpAxisChanged(const ObjectUpAxisChanged& msg, Robot& robot)
{
  const auto& objectID = msg.objectID;
  if(objectID == _bottomBlockID ||
     objectID == _topBlockID ||
     (_middleBlockID.IsSet() && objectID == _middleBlockID)){
    _objectsFlipped.insert(objectID);
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  switch (event.GetData().GetTag()) {
    case ExternalInterface::MessageEngineToGameTag::ObjectUpAxisChanged:
      // handled only while running
      break;
      

      
    default:
      PRINT_NAMED_ERROR("BehaviorKnockOverCubes.AlwaysHandleInternal.InvalidEvent", "");
      break;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::PrepareForKnockOverAttempt()
{
  _objectsFlipped.clear();
  IncreaseScoreWhileActing(kScoreIncreaseSoNoRoll);
  
  // Unlock the reactions b/c we're about to re-lock them
  // It's possible they might not have been locked - it's safe to remove an invalid lock
  // but locking twice due to a retry will cause a crash
  SmartRemoveDisableReactionsLock(kPreparingToKnockOverStackLock);
  SmartDisableReactionsWithLock(kPreparingToKnockOverStackLock,
                                kAffectTriggersPreparingKnockOverArray);
}
  
}
}
