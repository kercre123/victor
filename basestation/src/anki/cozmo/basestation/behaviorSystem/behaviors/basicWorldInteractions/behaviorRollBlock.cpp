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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorRollBlock.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorManager.h"
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/aiComponent/behaviorEventAnimResponseDirector.h"
#include "anki/cozmo/basestation/aiComponent/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/vision/basestation/observableObject.h"
#include "util/console/consoleInterface.h"



namespace Anki {
namespace Cozmo {

namespace{
#define SET_STATE(s) SetState_internal(State::s, #s)

  
static const char* const kIsBlockRotationImportant = "isBlockRotationImportant";
static const float kMaxDistCozmoIsRollingCube_mm = 120;

CONSOLE_VAR(f32, kBRB_ScoreIncreaseForAction, "Behavior.RollBlock", 0.8f);
CONSOLE_VAR(f32, kBRB_MaxTowardFaceAngle_deg, "Behavior.RollBlock", 90.f);
CONSOLE_VAR(s32, kBRB_MaxRollRetries,         "Behavior.RollBlock", 1);
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRollBlock::BehaviorRollBlock(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _robot(robot)
, _isBlockRotationImportant(true)
, _didCozmoAttemptDock(false)
, _upAxisOnBehaviorStart(AxisName::X_POS)
, _behaviorState(State::RollingBlock)
{  
  _isBlockRotationImportant = config.get(kIsBlockRotationImportant, true).asBool();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRollBlock::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  UpdateTargetBlock(preReqData.GetRobot());
  
  return _targetID.IsSet() || IsActing();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorRollBlock::InitInternal(Robot& robot)
{
  _didCozmoAttemptDock = false;
  ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(object != nullptr){
    UpdateTargetsUpAxis(robot);
    TransitionToPerformingAction(robot);
    return Result::RESULT_OK;
  }
  
  return Result::RESULT_FAIL;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorRollBlock::UpdateInternal(Robot& robot)
{
  ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(object != nullptr && _behaviorState == State::RollingBlock){
    const AxisName currentUpAxis = object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();
    if(currentUpAxis != _upAxisOnBehaviorStart){
      // Cozmo can see the cube at the end of a roll
      // so check distance between Cozmo and the cube to see whether he is still
      // rolling the cube himself or it was put upright by the user
      f32 distBetween = 0;
      if(ComputeDistanceSQBetween(robot.GetPose(), object->GetPose(), distBetween) &&
         (distBetween > (kMaxDistCozmoIsRollingCube_mm * kMaxDistCozmoIsRollingCube_mm))){
        
        UpdateTargetsUpAxis(robot);
        StopActing(false, false);
        
        if(_didCozmoAttemptDock){
          TransitionToRollSuccess(robot);
        }else{
          UpdateTargetBlock(robot);
          if(_targetID.IsSet()){
            TransitionToPerformingAction(robot);
          }else{
            return IBehavior::Status::Complete;
          }
        }
      }
    }
  }
  
  return base::UpdateInternal(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::StopInternal(Robot& robot)
{
  ResetBehavior(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::StopInternalFromDoubleTap(Robot& robot)
{
  ResetBehavior(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::UpdateTargetBlock(const Robot& robot) const
{
  using Intent = ObjectInteractionIntention;
  const Intent intent = (_isBlockRotationImportant ?
                         Intent::RollObjectWithDelegateAxisCheck :
                         Intent::RollObjectWithDelegateNoAxisCheck);
  auto& objInfoCache = robot.GetAIComponent().GetObjectInteractionInfoCache();
  _targetID = objInfoCache.GetBestObjectForIntention(intent);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::TransitionToPerformingAction(Robot& robot, bool isRetry)
{
  SET_STATE(RollingBlock);
  
  if( ! _targetID.IsSet() ) {
    PRINT_NAMED_WARNING("BehaviorRollBlock.NoBlockID",
                        "%s: Transitioning to action state, but we don't have a valid block ID",
                        GetIDStr().c_str());
    return;
  }

  const Radians maxTurnToFaceAngle(_shouldStreamline ? 0 : DEG_TO_RAD(90));
  // always roll to upright, even if orientation isn't important (always prefer it to end upright, even if we
  // will run without it being upright, e.g. Sparks)
  const bool upright = true;
  
  auto preDockCallback = [this](Robot& robot) {
    // If this behavior uses a tapped object then prevent ReactToDoubleTap from interrupting
    if(RequiresObjectTapped())
    {
      robot.GetAIComponent().GetWhiteboard().SetSuppressReactToDoubleTap(true);
    }
    _didCozmoAttemptDock = true;
  };
  
  auto delegateSuccess = [this](Robot& robot) {
    TransitionToRollSuccess(robot);
  };
  
  auto delegateFailure = [this](Robot& robot) {
    PRINT_NAMED_INFO("BehaviorRollBlock.FailedAbort", "Failed to verify roll");
    
    const ObservableObject* failedObject = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
    if(failedObject){
      robot.GetAIComponent().GetWhiteboard().SetFailedToUse(*failedObject, AIWhiteboard::ObjectActionFailure::RollOrPopAWheelie);
    }
  };
  
  RollBlockParameters params;
  params.maxTurnToFaceAngle = maxTurnToFaceAngle;
  params.preDockCallback = preDockCallback;
  params.sayNameAnimationTrigger = AnimationTrigger::RollBlockPreActionNamedFace;
  params.noNameAnimationTrigger = AnimationTrigger::RollBlockPreActionUnnamedFace;
  auto& factory = robot.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  HelperHandle rollHandle = factory.CreateRollBlockHelper(robot,
                                                          *this,
                                                          _targetID,
                                                          upright,
                                                          params);
  
  SmartDelegateToHelper(robot, rollHandle, delegateSuccess, delegateFailure);
  IncreaseScoreWhileActing( kBRB_ScoreIncreaseForAction );
  
  // Set the cube lights to interacting for full behavior run time
  std::vector<BehaviorStateLightInfo> basePersistantLight;
  basePersistantLight.push_back(
    BehaviorStateLightInfo(_targetID, CubeAnimationTrigger::InteractingBehaviorLock)
  );
  SetBehaviorStateLights(basePersistantLight, false);
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::TransitionToRollSuccess(Robot& robot)
{
  SET_STATE(CelebratingRoll);
  // The mood manager listens for actions to succeed to modify mood, but we may have just canceled the
  // action, so manually send the mood event here
  robot.GetMoodManager().TriggerEmotionEvent("RollSucceeded", MoodManager::GetCurrentTimeInSeconds());
  UpdateTargetsUpAxis(robot);

  if(!_shouldStreamline){
    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::RollBlockSuccess));
  }
  IncreaseScoreWhileActing( kBRB_ScoreIncreaseForAction );
  BehaviorObjectiveAchieved(BehaviorObjective::BlockRolled);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::ResetBehavior(Robot& robot)
{
  _targetID.UnSet();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::UpdateTargetsUpAxis(Robot& robot)
{
  ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
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
