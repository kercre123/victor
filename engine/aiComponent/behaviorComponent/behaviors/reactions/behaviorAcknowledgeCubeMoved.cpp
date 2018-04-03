/**
 * File: BehaviorAcknowledgeCubeMoved.cpp
 *
 * Author: Kevin M. Karol
 * Created: 7/26/16
 *
 * Description: Behavior to acknowledge when a localized cube has been moved
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorAcknowledgeCubeMoved.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/visionComponent.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  

namespace{
  
#define SET_STATE(s) SetState_internal(State::s, #s)

const float kDelayForUserPresentBlock_s = 1.0f;
const float kDelayToRecognizeBlock_s = 0.5f;

}
  
  
  
using namespace ExternalInterface;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAcknowledgeCubeMoved::BehaviorAcknowledgeCubeMoved(const Json::Value& config)
: ICozmoBehavior(config)
, _state(State::PlayingSenseReaction)
, _activeObjectSeen(false)
{  
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject,
  });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAcknowledgeCubeMoved::WantsToBeActivatedBehavior() const
{
  return _activeObjectID.IsSet();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::OnBehaviorActivated()
{
  _activeObjectSeen = false;
  switch(_state){
    case State::TurningToLastLocationOfBlock:
      TransitionToTurningToLastLocationOfBlock();
      break;
      
    default:
      TransitionToPlayingSenseReaction();
      break;
  }
  
  
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  // object seen - cancel turn and play response
  if(_state == State::TurningToLastLocationOfBlock
     && _activeObjectSeen)
  {
    CancelDelegates(false);
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::DEPRECATED_AcknowledgeObject));
    SET_STATE(ReactingToBlockPresence);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::OnBehaviorDeactivated()
{  
  _activeObjectID.UnSet();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::TransitionToPlayingSenseReaction()
{
  SET_STATE(PlayingSenseReaction);

  DelegateIfInControl(new CompoundActionParallel({
    new TriggerLiftSafeAnimationAction(AnimationTrigger::DEPRECATED_CubeMovedSense),
    new WaitAction(kDelayForUserPresentBlock_s) }),
              &BehaviorAcknowledgeCubeMoved::TransitionToTurningToLastLocationOfBlock);
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::TransitionToTurningToLastLocationOfBlock()
{
  SET_STATE(TurningToLastLocationOfBlock);
  
  const ObservableObject* obj = GetBEI().GetBlockWorld().GetLocatedObjectByID(_activeObjectID );
  if(obj == nullptr)
  {
    PRINT_NAMED_WARNING("BehaviorAcknowledgeCubeMoved.TransitionToTurningToLastLocationOfBlock.NullObject",
                        "The robot's context has changed and the block's location is no longer valid. (ObjectID=%d)",
                        _activeObjectID.GetValue());
    return;
  }
  const Pose3d& blockPose = obj->GetPose();

  DelegateIfInControl(new CompoundActionParallel({
    new TurnTowardsPoseAction(blockPose),
    new WaitAction(kDelayToRecognizeBlock_s) }),
              &BehaviorAcknowledgeCubeMoved::TransitionToReactingToBlockAbsence);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::TransitionToReactingToBlockAbsence()
{
  SET_STATE(ReactingToBlockAbsence);
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::DEPRECATED_CubeMovedUpset));
  BehaviorObjectiveAchieved(BehaviorObjective::ReactedAcknowledgedCubeMoved);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  SetDebugStateName(stateName);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::HandleWhileActivated(const EngineToGameEvent& event)
{
    switch(event.GetData().GetTag()){
      case EngineToGameTag::RobotObservedObject:
      {
        HandleObservedObject(event.GetData().Get_RobotObservedObject());
        break;
      }
      default:
      break;
    }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::HandleObservedObject(const ExternalInterface::RobotObservedObject& msg)
{
  if(_activeObjectID.IsSet() && msg.objectID == _activeObjectID){
    _activeObjectSeen = true;
  }
}

  
} // namespace Cozmo
} // namespace Anki
