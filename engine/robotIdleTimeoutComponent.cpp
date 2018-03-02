/**
* File: robotIdleTimeoutComponent
*
* Author: Lee Crippen
* Created: 8/9/2016
*
* Description: Handles the idle timeout functionality for the Robot. Listens for messages from Game
* for setting a timers that will trigger Cozmo going to sleep & turning off his screen, then cause Cozmo
* to disconnect from the engine entirely.
*
* Copyright: Anki, inc. 2016
*
*/

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/ankiEventUtil.h"
#include "engine/robot.h"
#include "engine/robotIdleTimeoutComponent.h"
#include "engine/robotInterface/messageHandler.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

RobotIdleTimeoutComponent::RobotIdleTimeoutComponent()
: IDependencyManagedComponent(this, RobotComponentID::RobotIdleTimeout)
{

}

void RobotIdleTimeoutComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  if (_robot->HasExternalInterface())
  {
    IExternalInterface* externalInterface = _robot->GetExternalInterface();
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(*externalInterface, *this, GetSignalHandles());
    
    helper.SubscribeGameToEngine<MessageGameToEngineTag::CancelIdleTimeout>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::StartIdleTimeout>();
  }
}


void RobotIdleTimeoutComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // If it's time to do sleep and face off
  if(_faceOffTimeout_s > 0.0f && _faceOffTimeout_s <= currentTime_s)
  {
    _faceOffTimeout_s = 0.0f;
    _robot->GetActionList().QueueAction(QueueActionPosition::NOW,
                                       CreateGoToSleepAnimSequence());
  }
  
  // If it's time to disconnect
  if(_disconnectTimeout_s > 0.0f && _disconnectTimeout_s <= currentTime_s)
  {
    _disconnectTimeout_s = 0.0f;
    _robot->GetRobotMessageHandler()->Disconnect();
  }
}
  
static bool ShouldUpdateTimeoutHelper(float currentTime, float oldTimeout, float newTimeoutDuration)
{
  bool oldTimeoutNotSet = (oldTimeout == -1.0f);
  // If we have a new timeout duration to use and (no existing timeout or a later existing timeout)
  if (newTimeoutDuration >= 0.0f &&
      (oldTimeoutNotSet || (currentTime + newTimeoutDuration) < oldTimeout) )
  {
    return true;
  }
  return false;
}
  
template<>
void RobotIdleTimeoutComponent::HandleMessage(const ExternalInterface::StartIdleTimeout& msg)
{
  const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // We need to have received our first state message from the robot (indicating successful communication) to send animations
  if (_robot->HasReceivedFirstStateMessage() && ShouldUpdateTimeoutHelper(currentTime, _faceOffTimeout_s, msg.faceOffTime_s))
  {
    _faceOffTimeout_s = currentTime + msg.faceOffTime_s;
  }
  
  if (ShouldUpdateTimeoutHelper(currentTime, _disconnectTimeout_s, msg.disconnectTime_s))
  {
    _disconnectTimeout_s = currentTime + msg.disconnectTime_s;
  }
}

template<>
void RobotIdleTimeoutComponent::HandleMessage(const ExternalInterface::CancelIdleTimeout& msg)
{
  _faceOffTimeout_s = -1.0f;
  _disconnectTimeout_s = -1.0f;
}
  
IActionRunner* RobotIdleTimeoutComponent::CreateGoToSleepAnimSequence()
{
  CompoundActionSequential* goToSleepAnims = new CompoundActionSequential();
  goToSleepAnims->AddAction(new TriggerAnimationAction(AnimationTrigger::GoToSleepGetIn));
  goToSleepAnims->AddAction(new TriggerAnimationAction(AnimationTrigger::GoToSleepSleeping));
  goToSleepAnims->AddAction(new TriggerAnimationAction(AnimationTrigger::GoToSleepOff));
  
  CompoundActionParallel* parallelContainer = new CompoundActionParallel();
  parallelContainer->AddAction(goToSleepAnims);
  parallelContainer->AddAction(new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK));
  
  return parallelContainer;
}

} // end namespace Cozmo
} // end namespace Anki
