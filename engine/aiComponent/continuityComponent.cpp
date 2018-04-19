/**
* File: continuityComponent.cpp
*
* Author: Kevin M. Karol
* Created: 2/1/18
*
* Description: Component responsible for ensuring decisions by the behavior system
* blend together well by the time they are sent to robot
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/continuityComponent.h"

#include "engine/actions/animActions.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ContinuityComponent::ContinuityComponent(Robot& robot)
: IDependencyManagedComponent<AIComponentID>(this, AIComponentID::ContinuityComponent)
, _robot(robot)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ContinuityComponent::~ContinuityComponent()
{
  Util::SafeDelete(_nextActionToQueue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContinuityComponent::UpdateDependent(const AICompMap& dependentComps)
{
  if(!_playingGetOut &&
     (_nextActionToQueue != nullptr)){
    QueueAction(_nextActionToQueue);
    _nextActionToQueue = nullptr;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ContinuityComponent::GetIntoAction(IActionRunner* action)
{
  if(_playingGetOut){
    delete _nextActionToQueue;
    _nextActionToQueue = action;
    return true;
  }else{
    return QueueAction(action);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ContinuityComponent::GetOutOfAction(u32 idTag)
{
  return _robot.GetActionList().Cancel(idTag);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContinuityComponent::PlayEmergencyGetOut(AnimationTrigger anim)
{
  // Queue now to cancel current action
  IActionRunner* animAction = new TriggerAnimationAction(anim);
  auto getOutCompleteCallback = [this](ActionResult res){
    _playingGetOut = false;
  };

  animAction->AddCompletionCallback(getOutCompleteCallback);
  _playingGetOut = QueueAction(animAction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ContinuityComponent::QueueAction(IActionRunner* action)
{
  Result result = _robot.GetActionList().QueueAction(QueueActionPosition::NOW, action);
  const bool success = (RESULT_OK == result);
  if (!success) {
    PRINT_NAMED_WARNING("ContinuityComponent.GetIntoAction.Failure.ActionNotSet",
                        "Unable to queue action '%s' (error %d)",
                        action->GetName().c_str(), result);
    delete action;
  }
  return success;
}


} // namespace Cozmo
} // namespace Anki
