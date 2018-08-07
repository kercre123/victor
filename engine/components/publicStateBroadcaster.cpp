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

#include "engine/components/publicStateBroadcaster.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"


namespace Anki {
namespace Vector {

namespace{
static const char* const kChannelName = "PublicStateBroadcast";
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PublicStateBroadcaster::PublicStateBroadcaster()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::PublicStateBroadcaster)
{
  BehaviorStageStruct empty;
  empty.behaviorStageTag = BehaviorStageTag::Count;
  const bool isCubeInLift = false;
  const bool isRequestingGame = false;
  const int  tallestStackHeight = 0;
  const NeedsLevels needsLevels(0,0,0);
  
  _currentState.reset(new RobotPublicState(isCubeInLift,
                                           isRequestingGame,
                                           tallestStackHeight,
                                           needsLevels,
                                           empty));
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PublicStateBroadcaster::UpdateDependent(const RobotCompMap& dependentComps)
{
  bool currentStateUpdated = false;
  
  const bool isCarryingObject = _robot->GetCarryingComponent().IsCarryingObject();
  // Check if a cube has been added to/removed from the lift
  if(isCarryingObject != _currentState->isCubeInLift){
    _currentState->isCubeInLift = isCarryingObject;
    currentStateUpdated = true;
    PRINT_CH_INFO(kChannelName,
                  "PublicStateBroadcaster.Update.RobotCarryingObjectChanged",
                  "Robot %s carrying an object",
                  isCarryingObject ? "is now" : "is no longer"
                  );
  }

  // After all update checks, if any changed a state property, send the updated state
  if(currentStateUpdated){
    SendUpdatedState();
  }
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
    case BehaviorStageTag::Feeding:
    {
      newStruct.currentFeedingStage = static_cast<FeedingStage>(stage);
      break;
    }
    case BehaviorStageTag::GuardDog:
    {
      newStruct.currentGuardDogStage = static_cast<GuardDogStage>(stage);
      break;
    }
    case BehaviorStageTag::PyramidConstruction:
    {
      newStruct.currentPyramidConstructionStage = static_cast<PyramidConstructionStage>(stage);
      break;
    }
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
void PublicStateBroadcaster::UpdateRequestingGame(bool isRequesting)
{
  _currentState->isRequestingGame = isRequesting;
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
    case BehaviorStageTag::Feeding:
    {
      return Util::EnumToUnderlying(stageStruct.currentFeedingStage);
    }
    case BehaviorStageTag::GuardDog:
    {
      return Util::EnumToUnderlying(stageStruct.currentGuardDogStage);
    }
    case BehaviorStageTag::PyramidConstruction:
    {
      return Util::EnumToUnderlying(stageStruct.currentPyramidConstructionStage);
    }
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

  
} // namespace Vector
} // namespace Anki
