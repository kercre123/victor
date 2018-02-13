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

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorRollBlock.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorEventAnimResponseDirector.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/behaviorHelperFactory.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/moodSystem/moodManager.h"
#include "coretech/vision/engine/observableObject.h"
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
: ICozmoBehavior(config)
, _isBlockRotationImportant(true)
, _didCozmoAttemptDock(false)
, _upAxisOnBehaviorStart(AxisName::X_POS)
, _behaviorState(State::RollingBlock)
{  
  _isBlockRotationImportant = config.get(kIsBlockRotationImportant, true).asBool();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRollBlock::WantsToBeActivatedBehavior() const
{
  UpdateTargetBlock();
  
  return _targetID.IsSet() || IsControlDelegated();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::OnBehaviorActivated()
{
  _didCozmoAttemptDock = false;
  const ObservableObject* object = GetBEI().GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(object != nullptr){
    UpdateTargetsUpAxis();
    TransitionToPerformingAction();
    
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  const ObservableObject* object = GetBEI().GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(object != nullptr && _behaviorState == State::RollingBlock){
    const AxisName currentUpAxis = object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();
    if(currentUpAxis != _upAxisOnBehaviorStart){
      // Cozmo can see the cube at the end of a roll
      // so check distance between Cozmo and the cube to see whether he is still
      // rolling the cube himself or it was put upright by the user
      f32 distBetween = 0;
      if(ComputeDistanceSQBetween(GetBEI().GetRobotInfo().GetPose(), 
                                  object->GetPose(), distBetween) &&
         (distBetween > (kMaxDistCozmoIsRollingCube_mm * kMaxDistCozmoIsRollingCube_mm))){
        
        UpdateTargetsUpAxis();
        CancelDelegates(false, false);
        
        if(_didCozmoAttemptDock){
          TransitionToRollSuccess();
        }else{
          UpdateTargetBlock();
          if(_targetID.IsSet()){
            TransitionToPerformingAction();
          }else{
            CancelSelf();
          }
        }
      }
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::OnBehaviorDeactivated()
{
  ResetBehavior();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::UpdateTargetBlock() const
{
  using Intent = ObjectInteractionIntention;
  const Intent intent = (_isBlockRotationImportant ?
                         Intent::RollObjectWithDelegateAxisCheck :
                         Intent::RollObjectWithDelegateNoAxisCheck);
  auto& objInfoCache = GetBEI().GetAIComponent().GetObjectInteractionInfoCache();
  _targetID = objInfoCache.GetBestObjectForIntention(intent);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::TransitionToPerformingAction(bool isRetry)
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
  
  auto delegateSuccess = [this]() {
    TransitionToRollSuccess();
  };
  
  auto delegateFailure = [this]() {
    PRINT_NAMED_INFO("BehaviorRollBlock.FailedAbort", "Failed to verify roll");
    
    const ObservableObject* failedObject = GetBEI().GetBlockWorld().GetLocatedObjectByID(_targetID);
    if(failedObject){
      GetBEI().GetAIComponent().GetWhiteboard().SetFailedToUse(*failedObject, AIWhiteboard::ObjectActionFailure::RollOrPopAWheelie);
    }
  };
  
  RollBlockParameters params;
  params.maxTurnToFaceAngle = maxTurnToFaceAngle;
  params.preDockCallback = preDockCallback;
  params.sayNameAnimationTrigger = AnimationTrigger::RollBlockPreActionNamedFace;
  params.noNameAnimationTrigger = AnimationTrigger::RollBlockPreActionUnnamedFace;
  auto& factory = GetBEI().GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  HelperHandle rollHandle = factory.CreateRollBlockHelper(*this,
                                                          _targetID,
                                                          upright,
                                                          params);
  
  SmartDelegateToHelper(rollHandle, delegateSuccess, delegateFailure);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::TransitionToRollSuccess()
{
  SET_STATE(CelebratingRoll);
  // The mood manager listens for actions to succeed to modify mood, but we may have just canceled the
  // action, so manually send the mood event here
  if(GetBEI().HasMoodManager()){
    auto& moodManager = GetBEI().GetMoodManager();
    moodManager.TriggerEmotionEvent("RollSucceeded",
                                    MoodManager::GetCurrentTimeInSeconds());
  }
  UpdateTargetsUpAxis();

  if(!ShouldStreamline()){
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::RollBlockSuccess));
  }
  BehaviorObjectiveAchieved(BehaviorObjective::BlockRolled);
  NeedActionCompleted();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::ResetBehavior()
{
  _targetID.UnSet();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::UpdateTargetsUpAxis()
{
  const ObservableObject* object = GetBEI().GetBlockWorld().GetLocatedObjectByID(_targetID);
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
