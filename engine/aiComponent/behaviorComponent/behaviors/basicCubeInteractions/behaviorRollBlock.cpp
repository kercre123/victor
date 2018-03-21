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

#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorRollBlock.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/actions/animActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorEventAnimResponseDirector.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/moodSystem/moodManager.h"
#include "coretech/vision/engine/observableObject.h"
#include "util/console/consoleInterface.h"



namespace Anki {
namespace Cozmo {

namespace{
#define SET_STATE(s) SetState_internal(State::s, #s)

  
static const char* const kRetryCountKey = "rollRetryCount";
static const char* const kIsBlockRotationImportant = "isBlockRotationImportant";
static const float kMaxDistCozmoIsRollingCube_mm = 120;

CONSOLE_VAR(f32, kBRB_ScoreIncreaseForAction, "Behavior.RollBlock", 0.8f);
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRollBlock::InstanceConfig::InstanceConfig()
{
  isBlockRotationImportant = false;
  rollRetryCount           = 1;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRollBlock::DynamicVariables::DynamicVariables()
{
  didAttemptDock        = false;
  upAxisOnBehaviorStart = AxisName::X_POS;
  behaviorState         = State::RollingBlock;
  rollRetryCount        = 0;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRollBlock::BehaviorRollBlock(const Json::Value& config)
: ICozmoBehavior(config)
{  
  _iConfig.isBlockRotationImportant = config.get(kIsBlockRotationImportant, true).asBool();
  if(config.isMember(kRetryCountKey)){
    const std::string debugStr = "BehaviorRollBlock.Constructor.RetryCountParseIssue";
    _iConfig.rollRetryCount = JsonTools::ParseUint8(config, kRetryCountKey,debugStr);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kIsBlockRotationImportant,
    kRetryCountKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRollBlock::WantsToBeActivatedBehavior() const
{
  ObjectID targetID;
  CalculateTargetID(targetID);
  
  return targetID.IsSet() || IsControlDelegated();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  CalculateTargetID(_dVars.targetID);

  const ObservableObject* object = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);
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

  const ObservableObject* object = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);
  if(object != nullptr && _dVars.behaviorState == State::RollingBlock){
    const AxisName currentUpAxis = object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();
    if(currentUpAxis != _dVars.upAxisOnBehaviorStart){
      // Cozmo can see the cube at the end of a roll
      // so check distance between Cozmo and the cube to see whether he is still
      // rolling the cube himself or it was put upright by the user
      f32 distBetween = 0;
      if(ComputeDistanceSQBetween(GetBEI().GetRobotInfo().GetPose(), 
                                  object->GetPose(), distBetween) &&
         (distBetween > (kMaxDistCozmoIsRollingCube_mm * kMaxDistCozmoIsRollingCube_mm))){
        
        UpdateTargetsUpAxis();
        CancelDelegates(false);
        
        if(_dVars.didAttemptDock){
          TransitionToRollSuccess();
        }else{
          CalculateTargetID(_dVars.targetID);
          if(_dVars.targetID.IsSet()){
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
void BehaviorRollBlock::CalculateTargetID(ObjectID& targetID) const
{
  using Intent = ObjectInteractionIntention;
  const Intent intent = (_iConfig.isBlockRotationImportant ?
                         Intent::RollObjectWithDelegateAxisCheck :
                         Intent::RollObjectWithDelegateNoAxisCheck);
  auto& objInfoCache = GetAIComp<ObjectInteractionInfoCache>();
  targetID = objInfoCache.GetBestObjectForIntention(intent);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::TransitionToPerformingAction()
{
  SET_STATE(RollingBlock);
  
  if( ! _dVars.targetID.IsSet() ) {
    PRINT_NAMED_WARNING("BehaviorRollBlock.NoBlockID",
                        "%s: Transitioning to action state, but we don't have a valid block ID",
                        GetDebugLabel().c_str());
    return;
  }
  
  DriveToRollObjectAction* rollAction = new DriveToRollObjectAction(_dVars.targetID);
  rollAction->SetPreDockCallback([this](Robot& robot){
    _dVars.didAttemptDock = true;
  });

  DelegateIfInControl(rollAction, [this](ActionResult result){
    if(result == ActionResult::SUCCESS){
      TransitionToRollSuccess();
    }else if((IActionRunner::GetActionResultCategory(result) == ActionResultCategory::RETRY) &&
             (_dVars.rollRetryCount < _iConfig.rollRetryCount)){
      _dVars.rollRetryCount++;
      TransitionToPerformingAction();
    }else{
      auto& blockWorld = GetBEI().GetBlockWorld();
      const ObservableObject* pickupObj = blockWorld.GetLocatedObjectByID(_dVars.targetID);
      if(pickupObj != nullptr){
        auto& whiteboard = GetAIComp<AIWhiteboard>();	
        whiteboard.SetFailedToUse(*pickupObj,	
                                  AIWhiteboard::ObjectActionFailure::RollOrPopAWheelie);
      }
    }
  });
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
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::UpdateTargetsUpAxis()
{
  const ObservableObject* object = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.targetID);
  if(object != nullptr){
    _dVars.upAxisOnBehaviorStart = object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRollBlock::SetState_internal(State state, const std::string& stateName)
{
  _dVars.behaviorState = state;
  SetDebugStateName(stateName);
}



}
}
