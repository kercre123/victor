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

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorKnockOverCubes.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/flipBlockAction.h"
#include "engine/actions/retryWrapperAction.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockConfigurationManager.h"
#include "engine/blockWorld/blockConfigurationStack.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/events/animationTriggerHelpers.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/common/basestation/jsonTools.h"
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

const int kMaxNumRetries = 2;
const float kMinThresholdRealign = 20.f;
const int kMinBlocksForSuccess = 1;
const float kWaitForBlockUpAxisChangeSecs = 0.5f;
const f32 kBSB_MaxTurnTowardsFaceBeforeKnockStack_rad = DEG_TO_RAD(90.f);

CONSOLE_VAR(f32, kBKS_distanceToTryToGrabFrom_mm, "Behavior.AdmireStack", 85.0f);
CONSOLE_VAR(f32, kBKS_searchSpeed_mmps, "Behavior.AdmireStack", 60.0f);
} // end namespace

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorKnockOverCubes::BehaviorKnockOverCubes(const Json::Value& config)
: ICozmoBehavior(config)
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
bool BehaviorKnockOverCubes::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  UpdateTargetStack(behaviorExternalInterface);
  if(auto tallestStack = _currentTallestStack.lock()){
    return tallestStack->GetStackHeight() >= _minStackHeight;
  }
  
  return false;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(InitializeMemberVars()){
    if(!ShouldStreamline()){
      TransitionToReachingForBlock(behaviorExternalInterface);
    }else{
      TransitionToKnockingOverStack(behaviorExternalInterface);
    }
    
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  ClearStack();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::TransitionToReachingForBlock(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(ReachingForBlock);

  const ObservableObject* topBlock = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_topBlockID);
  
  if(topBlock == nullptr){
    ClearStack();
    return;
  }
  
  CompoundActionSequential* action = new CompoundActionSequential();
  
  action->AddAction(new TurnTowardsObjectAction(_bottomBlockID));
  
  Pose3d poseWrtRobot;
  if(topBlock->GetPose().GetWithRespectTo(behaviorExternalInterface.GetRobotInfo().GetPose(), poseWrtRobot) ) {
    const float fudgeFactor = 10.0f;
    if( poseWrtRobot.GetTranslation().x() + fudgeFactor > kBKS_distanceToTryToGrabFrom_mm) {
      float distToDrive = poseWrtRobot.GetTranslation().x() - kBKS_distanceToTryToGrabFrom_mm;
      action->AddAction(new DriveStraightAction(distToDrive, kBKS_searchSpeed_mmps));
    }
  }
  
  action->AddAction(new TriggerLiftSafeAnimationAction(_reachForBlockTrigger));
  DelegateIfInControl(action, &BehaviorKnockOverCubes::TransitionToKnockingOverStack);
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::TransitionToKnockingOverStack(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(KnockingOverStack);
  
  auto flipCallback = [this, &behaviorExternalInterface](const ActionResult& result){
    // Mark the object as having no preaction poses - there's nothing else we can do
    if(result == ActionResult::NO_PREACTION_POSES){
      behaviorExternalInterface.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_bottomBlockID);
      return;
    }
    
    const ActionResultCategory resCat = IActionRunner::GetActionResultCategory(result);
    if(resCat == ActionResultCategory::SUCCESS){
      //Knocked over stack successfully
      TransitionToPlayingReaction(behaviorExternalInterface);
    }else if(resCat == ActionResultCategory::RETRY){
      //Assume we had an alignment issue
      if(_numRetries < kMaxNumRetries){
        TransitionToKnockingOverStack(behaviorExternalInterface);
      }else{
        // We've aligned a bunch of times - just go for it
        TransitionToBlindlyFlipping(behaviorExternalInterface);
      }
      _numRetries++;
    }
  };
  
  // Setup the flip action
  
  //skips turning towards face if this action is streamlined
  const f32 angleTurnTowardsFace_rad = (ShouldStreamline() || _numRetries > 0) ? 0 : kBSB_MaxTurnTowardsFaceBeforeKnockStack_rad;

  DriveAndFlipBlockAction* flipAction = new DriveAndFlipBlockAction(_bottomBlockID, 
                                                                    false, 
                                                                    0, 
                                                                    false, 
                                                                    angleTurnTowardsFace_rad, 
                                                                    false, 
                                                                    kMinThresholdRealign);
  
  flipAction->SetSayNameAnimationTrigger(AnimationTrigger::KnockOverPreActionNamedFace);
  flipAction->SetNoNameAnimationTrigger(AnimationTrigger::KnockOverPreActionUnnamedFace);
  
  
  // Set the action sequence
  CompoundActionSequential* flipAndWaitAction = new CompoundActionSequential();
  flipAndWaitAction->AddAction(new TurnTowardsObjectAction( _bottomBlockID));
  // emit completion signal so that the mood manager can react
  const bool shouldEmitCompletion = true;
  flipAndWaitAction->AddAction(flipAction, false, shouldEmitCompletion);
  flipAndWaitAction->AddAction(new WaitAction(kWaitForBlockUpAxisChangeSecs));
  
  // make sure we only account for blocks flipped during the actual knock over action
  PrepareForKnockOverAttempt();
  DelegateIfInControl(flipAndWaitAction, flipCallback);
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::TransitionToBlindlyFlipping(BehaviorExternalInterface& behaviorExternalInterface)
{
  CompoundActionSequential* flipAndWaitAction = new CompoundActionSequential();
  {
    FlipBlockAction* flipAction = new FlipBlockAction( _bottomBlockID);
    flipAction->SetShouldCheckPreActionPose(false);
  
    flipAndWaitAction->AddAction(flipAction);
    flipAndWaitAction->AddAction(new WaitAction(kWaitForBlockUpAxisChangeSecs));
  }
  
  PrepareForKnockOverAttempt();
  DelegateIfInControl(flipAndWaitAction, &BehaviorKnockOverCubes::TransitionToPlayingReaction);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::TransitionToPlayingReaction(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(PlayingReaction);

  // notify configuration manager that the tower was knocked over
  behaviorExternalInterface.GetBlockWorld().GetBlockConfigurationManager().FlagForRebuild();
  
  // determine if the robot successfully knocked over the min number of cubes
  auto animationTrigger = _knockOverFailureTrigger;
  if(_objectsFlipped.size() >= kMinBlocksForSuccess){
    BehaviorObjectiveAchieved(BehaviorObjective::KnockedOverBlocks);
    NeedActionCompleted();
    animationTrigger = _knockOverSuccessTrigger;
  }
  
  // play a reaction if not streamlined
  if(!ShouldStreamline()){
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(animationTrigger));
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorKnockOverCubes::InitializeMemberVars()
{
  if(auto tallestStack = _currentTallestStack.lock()){
  // clear for success state check
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
void BehaviorKnockOverCubes::UpdateTargetStack(BehaviorExternalInterface& behaviorExternalInterface) const
{
   _currentTallestStack = behaviorExternalInterface.GetBlockWorld().GetBlockConfigurationManager().GetStackCache().GetTallestStack();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::HandleObjectUpAxisChanged(const ObjectUpAxisChanged& msg, BehaviorExternalInterface& behaviorExternalInterface)
{
  const auto& objectID = msg.objectID;
  if(objectID == _bottomBlockID ||
     objectID == _topBlockID ||
     (_middleBlockID.IsSet() && objectID == _middleBlockID)){
    _objectsFlipped.insert(objectID);
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorKnockOverCubes::HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  switch (event.GetData().GetTag()) {
    case ExternalInterface::MessageEngineToGameTag::ObjectUpAxisChanged:
      HandleObjectUpAxisChanged(event.GetData().Get_ObjectUpAxisChanged(), behaviorExternalInterface);
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
void BehaviorKnockOverCubes::AlwaysHandleInScope(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
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
}
  
}
}
