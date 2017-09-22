/**
 * File: behaviorRollBlock.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-04-05
 *
 * Description: This behavior rolls blocks when it see's them facing a direction specified in config
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorRollBlock.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorSystem/behaviorManager.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorEventAnimResponseDirector.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorSystem/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorSystem/behaviorHelpers/behaviorHelperFactory.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robot.h"
#include "anki/vision/basestation/observableObject.h"
#include "util/console/consoleInterface.h"



namespace Anki {
namespace Cozmo {

namespace{
#define SET_STATE(s) SetState_internal(State::s, #s)

  
static const char* const kIsBlockRotationImportant = "isBlockRotationImportant";
static const float kMaxDistCozmoIsRollingCube_mm = 120;

CONSOLE_VAR(f32, kBRB_ScoreIncreaseForAction, "Behavior.RollBlock", 0.8f);
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRollBlock::BehaviorRollBlock(const Json::Value& config)
: IBehavior(config)
, _isBlockRotationImportant(true)
, _didCozmoAttemptDock(false)
, _upAxisOnBehaviorStart(AxisName::X_POS)
, _behaviorState(State::RollingBlock)
{  
  _isBlockRotationImportant = config.get(kIsBlockRotationImportant, true).asBool();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRollBlock::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  UpdateTargetBlock(behaviorExternalInterface);
  
  return _targetID.IsSet() || IsActing();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorRollBlock::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _didCozmoAttemptDock = false;
  const ObservableObject* object = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(object != nullptr){
    UpdateTargetsUpAxis(behaviorExternalInterface);
    TransitionToPerformingAction(behaviorExternalInterface);
    return Result::RESULT_OK;
  }
  
  return Result::RESULT_FAIL;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorRollBlock::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  const ObservableObject* object = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(object != nullptr && _behaviorState == State::RollingBlock){
    const AxisName currentUpAxis = object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();
    if(currentUpAxis != _upAxisOnBehaviorStart){
      // Cozmo can see the cube at the end of a roll
      // so check distance between Cozmo and the cube to see whether he is still
      // rolling the cube himself or it was put upright by the user
      f32 distBetween = 0;
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should
      // be removed
      const Robot& robot = behaviorExternalInterface.GetRobot();
      if(ComputeDistanceSQBetween(robot.GetPose(), object->GetPose(), distBetween) &&
         (distBetween > (kMaxDistCozmoIsRollingCube_mm * kMaxDistCozmoIsRollingCube_mm))){
        
        UpdateTargetsUpAxis(behaviorExternalInterface);
        StopActing(false, false);
        
        if(_didCozmoAttemptDock){
          TransitionToRollSuccess(behaviorExternalInterface);
        }else{
          UpdateTargetBlock(behaviorExternalInterface);
          if(_targetID.IsSet()){
            TransitionToPerformingAction(behaviorExternalInterface);
          }else{
            return IBehavior::Status::Complete;
          }
        }
      }
    }
  }
  
  return base::UpdateInternal_WhileRunning(behaviorExternalInterface);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  ResetBehavior(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::UpdateTargetBlock(BehaviorExternalInterface& behaviorExternalInterface) const
{
  using Intent = ObjectInteractionIntention;
  const Intent intent = (_isBlockRotationImportant ?
                         Intent::RollObjectWithDelegateAxisCheck :
                         Intent::RollObjectWithDelegateNoAxisCheck);
  auto& objInfoCache = behaviorExternalInterface.GetAIComponent().GetObjectInteractionInfoCache();
  _targetID = objInfoCache.GetBestObjectForIntention(intent);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::TransitionToPerformingAction(BehaviorExternalInterface& behaviorExternalInterface, bool isRetry)
{
  SET_STATE(RollingBlock);
  
  if( ! _targetID.IsSet() ) {
    PRINT_NAMED_WARNING("BehaviorRollBlock.NoBlockID",
                        "%s: Transitioning to action state, but we don't have a valid block ID",
                        GetIDStr().c_str());
    return;
  }

  const Radians maxTurnToFaceAngle(ShouldStreamline() ? 0 : DEG_TO_RAD(90));
  // always roll to upright, even if orientation isn't important (always prefer it to end upright, even if we
  // will run without it being upright, e.g. Sparks)
  const bool upright = true;
  
  auto preDockCallback = [this](Robot& robot) {
    _didCozmoAttemptDock = true;
  };
  
  auto delegateSuccess = [this](BehaviorExternalInterface& behaviorExternalInterface) {
    TransitionToRollSuccess(behaviorExternalInterface);
  };
  
  auto delegateFailure = [this](BehaviorExternalInterface& behaviorExternalInterface) {
    PRINT_NAMED_INFO("BehaviorRollBlock.FailedAbort", "Failed to verify roll");
    
    const ObservableObject* failedObject = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
    if(failedObject){
      behaviorExternalInterface.GetAIComponent().GetWhiteboard().SetFailedToUse(*failedObject, AIWhiteboard::ObjectActionFailure::RollOrPopAWheelie);
    }
  };
  
  RollBlockParameters params;
  params.maxTurnToFaceAngle = maxTurnToFaceAngle;
  params.preDockCallback = preDockCallback;
  params.sayNameAnimationTrigger = AnimationTrigger::RollBlockPreActionNamedFace;
  params.noNameAnimationTrigger = AnimationTrigger::RollBlockPreActionUnnamedFace;
  auto& factory = behaviorExternalInterface.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  HelperHandle rollHandle = factory.CreateRollBlockHelper(behaviorExternalInterface,
                                                          *this,
                                                          _targetID,
                                                          upright,
                                                          params);
  
  SmartDelegateToHelper(behaviorExternalInterface, rollHandle, delegateSuccess, delegateFailure);
  IncreaseScoreWhileActing( kBRB_ScoreIncreaseForAction );
  
  // Set the cube lights to interacting for full behavior run time
  std::vector<BehaviorStateLightInfo> basePersistantLight;
  basePersistantLight.push_back(
    BehaviorStateLightInfo(_targetID, CubeAnimationTrigger::InteractingBehaviorLock)
  );
  SetBehaviorStateLights(basePersistantLight, false);
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::TransitionToRollSuccess(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(CelebratingRoll);
  // The mood manager listens for actions to succeed to modify mood, but we may have just canceled the
  // action, so manually send the mood event here
  auto moodManager = behaviorExternalInterface.GetMoodManager().lock();
  if(moodManager != nullptr){
    moodManager->TriggerEmotionEvent("RollSucceeded", MoodManager::GetCurrentTimeInSeconds());
  }
  UpdateTargetsUpAxis(behaviorExternalInterface);

  if(!ShouldStreamline()){
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::RollBlockSuccess));
  }
  IncreaseScoreWhileActing( kBRB_ScoreIncreaseForAction );
  BehaviorObjectiveAchieved(BehaviorObjective::BlockRolled);
  NeedActionCompleted();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::ResetBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  _targetID.UnSet();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::UpdateTargetsUpAxis(BehaviorExternalInterface& behaviorExternalInterface)
{
  const ObservableObject* object = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(object != nullptr){
    _upAxisOnBehaviorStart = object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::SetState_internal(State state, const std::string& stateName)
{
  _behaviorState = state;
  SetDebugStateName(stateName);
}



}
}
