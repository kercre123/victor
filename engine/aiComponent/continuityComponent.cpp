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

#include "clad/externalInterface/messageEngineToGame.h"
#include "engine/actions/animActions.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ContinuityComponent::ContinuityComponent(Robot& robot)
: IDependencyManagedComponent<AIComponentID>(this, AIComponentID::ContinuityComponent)
, _robot(robot)
, _animTag(ActionConstants::INVALID_TAG)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ContinuityComponent::~ContinuityComponent()
{
  Util::SafeDelete(_nextActionToQueue);
}
  
void ContinuityComponent::InitDependent(Robot *robot, const AICompMap& dependentComps)
{
  if( robot->HasExternalInterface() ){
    auto onCompletedAction = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
    {
      if( event.GetData().GetTag() == ExternalInterface::MessageEngineToGameTag::RobotCompletedAction ) {
        const auto& msg = event.GetData().Get_RobotCompletedAction();
        if( msg.idTag == _animTag ) {
          _playingGetOut = false;
          _animTag = ActionConstants::INVALID_TAG;
        }
      }
    };
    _signalHandles.push_back(_robot.GetExternalInterface()->Subscribe( ExternalInterface::MessageEngineToGameTag::RobotCompletedAction,
                                                                       onCompletedAction ) );
  }
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
    if( _nextActionToQueue != nullptr ) {
      PRINT_NAMED_WARNING("ContinuityComponent.GetIntoAction.ReplacingAction", "Replacing delegated action");
      delete _nextActionToQueue;
    }
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
  if( _playingGetOut ) {
    PRINT_NAMED_WARNING( "ContinuityComponent.PlayEmergencyGetOut.MultipleGetOuts",
                         "Continuity component is trying to play multiple emergency getouts (%s)",
                         AnimationTriggerToString(anim) );
  }

  // Prevent emergency getouts from playing when we are displaying info screens
  // such as pairing or CC screens as the getout animations can draw over the info screens
  if(_displayingInfoFace)
  {
    PRINT_NAMED_INFO("ContinuityComponent.PlayEmergencyGetOut.DisplayingInfoFace",
                     "Not playing emergency get out %s due to info face being displayed",
                     EnumToString(anim));
    return;
  }
  
  // Queue now to cancel current action
  IActionRunner* animAction = new TriggerAnimationAction(anim);
  const auto animTag = animAction->GetTag();
  _playingGetOut = QueueAction(animAction);
  if( _playingGetOut ) {
    _animTag = animTag;
  }
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


} // namespace Vector
} // namespace Anki
