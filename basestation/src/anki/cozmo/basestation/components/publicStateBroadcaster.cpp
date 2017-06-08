/**
 * File: publicStateBroadcaster.cpp
 *
 * Author: Kevin M. Karol
 * Created: 4/5/2017
 *
 * Description: Tracks state information about the robot/current behavior/game
 * that engine wants to expose to other parts of the system (music, UI, etc).
 * Full state information is broadcast every time any piece of tracked state changes.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/components/publicStateBroadcaster.h"

#include "anki/cozmo/basestation/behaviorSystem/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/robot.h"


namespace Anki {
namespace Cozmo {

namespace{
static const char* const kChannelName = "PublicStateBroadcast";
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PublicStateBroadcaster::PublicStateBroadcaster()
{
  BehaviorStageStruct empty;
  empty.behaviorStageTag = BehaviorStageTag::Count;
  const bool isCubeInLift = false;
  _currentState.reset(new RobotPublicState(UnlockId::Count,
                                           isCubeInLift,
                                           ActivityID::Invalid,
                                           ReactionTrigger::NoneTrigger,
                                           empty));
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PublicStateBroadcaster::Update(Robot& robot)
{
  bool currentStateUpdated = false;
  
  // Check if current spark has changed - when a user cancels a spark we leave the
  // active spark the same until the action ends, but this is an implementation detail
  // of the way the activity system works - to the rest of the system the spark should
  // be considered ended
  const auto& bm = robot.GetBehaviorManager();
  
  const bool hasActiveSparkChanged = bm.GetActiveSpark() != _currentState->currentSpark;
  const bool didUserCancelSpark = (bm.GetRequestedSpark() == UnlockId::Count) &&
                                  (bm.GetActiveSpark() != bm.GetRequestedSpark());
  
  if(hasActiveSparkChanged && !didUserCancelSpark){
    _currentState->currentSpark = robot.GetBehaviorManager().GetActiveSpark();
    currentStateUpdated = true;
    PRINT_CH_INFO(kChannelName,
                  "PublicStateBroadcaster.Update.SparkStateHasChanged",
                  "Spark state has changed to %s",
                  EnumToString(_currentState->currentSpark)
                  );
  }
  
  if(didUserCancelSpark && _currentState->currentSpark != UnlockId::Count){
    PRINT_CH_INFO(kChannelName,
                  "PublicStateBroadcaster.Update.UserCanceldSpark",
                  "Spark state has changed from %s",
                  EnumToString(_currentState->currentSpark)
                  );
    _currentState->currentSpark = UnlockId::Count;
    currentStateUpdated = true;
  }
  
  
  // Check if a cube has been added to/removed from the lift
  if(robot.IsCarryingObject() != _currentState->isCubeInLift){
    _currentState->isCubeInLift = robot.IsCarryingObject();
    currentStateUpdated = true;
    PRINT_CH_INFO(kChannelName,
                  "PublicStateBroadcaster.Update.RobotCarryingObjectChanged",
                  "Robot %s carrying an object",
                  robot.IsCarryingObject() ? "is now" : "is no longer"
                  );
  }
  
  // Check if current reaction trigger has changed
  if(robot.GetBehaviorManager().GetCurrentReactionTrigger() != _currentState->currentReactionTrigger){
    _currentState->currentReactionTrigger = robot.GetBehaviorManager().GetCurrentReactionTrigger();
    currentStateUpdated = true;
    PRINT_CH_INFO(kChannelName,
                  "PublicStateBroadcaster.Update.ReactionTriggered",
                  "Robot is now reacting to trigger %s",
                  EnumToString(_currentState->currentReactionTrigger)
                  );
  }

  
  // After all update checks, if any changed a state property, send the updated state
  if(currentStateUpdated){
    SendUpdatedState();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PublicStateBroadcaster::UpdateActivity(ActivityID activityID)
{
  // update internal struct with new AI Goal name and send it out.
  _currentState->currentActivity = activityID;
  SendUpdatedState();
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PublicStateBroadcaster::UpdateBroadcastBehaviorStage(BehaviorStageTag stageType, uint8_t stage)
{
  // Print information about the transition that's occurring, and verify that
  // at least one of the pieces of stage information is changing
  if(stageType == _currentState->userFacingBehaviorStageStruct.behaviorStageTag ){
    int oldStage = GetStageForBehaviorStageType(stageType,
                                                _currentState->userFacingBehaviorStageStruct);
    if(stage != oldStage){
      PRINT_CH_INFO(kChannelName,
                    "PublicStateBroadcaster.UpdateUserFacingBehaviorStage.UpdatingStage",
                    "Updating stage type %s to stage %d",
                    EnumToString(stageType), stage);
    }else{
      PRINT_NAMED_WARNING("PublicStateBroadcaster.UpdateUserFacingBehaviorStage",
                          "Attempting to update a stage that has already been set");
      return;
    }
    
  }else{
    PRINT_CH_INFO(kChannelName,
                  "PublicStateBroadcaster.UpdateUserFacingBehaviorStage.SwitchingStageType",
                  "Moving from behavior stage type %s to stage type %s",
                  EnumToString(_currentState->userFacingBehaviorStageStruct.behaviorStageTag ),
                  EnumToString(stageType));
  }
  
  BehaviorStageStruct newStruct;
  switch(stageType){
    case BehaviorStageTag::PyramidConstruction:
    {
      newStruct.currentPyramidConstruction = static_cast<PyramidConstructionStage>(stage);
      break;
    }
    case BehaviorStageTag::GuardDog:
    {
      newStruct.currentGuardDogStage = static_cast<GuardDogStage>(stage);
      break;
    }
    case BehaviorStageTag::Workout:
    {
      newStruct.currentWorkoutStage = static_cast<WorkoutStage>(stage);
      break;
    }
    case BehaviorStageTag::Dance: // Dancing doesn't have any stages
    case BehaviorStageTag::Count:
    {
      break;
    }
  }
  
  newStruct.behaviorStageTag = stageType;
  _currentState->userFacingBehaviorStageStruct = newStruct;
  SendUpdatedState();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PublicStateBroadcaster::GetBehaviorRoundFromMessage(const RobotPublicState& stateEvent)
{
  return GetStageForBehaviorStageType(stateEvent.userFacingBehaviorStageStruct.behaviorStageTag,
                                      stateEvent.userFacingBehaviorStageStruct);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PublicStateBroadcaster::GetStageForBehaviorStageType(BehaviorStageTag stageType,
                                                         const BehaviorStageStruct& stageStruct){
  switch(stageType){
    case BehaviorStageTag::PyramidConstruction:
    {
      return Util::EnumToUnderlying(stageStruct.currentPyramidConstruction);
    }
    case BehaviorStageTag::GuardDog:
    {
      return Util::EnumToUnderlying(stageStruct.currentGuardDogStage);
    }
    case BehaviorStageTag::Workout:
    {
      return Util::EnumToUnderlying(stageStruct.currentWorkoutStage);
    }
    case BehaviorStageTag::Dance: // Dancing doesn't have any stages
    case BehaviorStageTag::Count:
    {
      return 0;
    }
  }
  
  return 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Signal::SmartHandle PublicStateBroadcaster::Subscribe(SubscribeFunc messageHandler )
{
  return _eventMgr.Subscribe(0, messageHandler );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PublicStateBroadcaster::SendUpdatedState()
{
  _eventMgr.Broadcast(AnkiEvent<RobotPublicState>(static_cast<uint32_t>(0), *_currentState));
}

  
} // namespace Cozmo
} // namespace Anki
