/**
* File: behaviorReactToVoiceCommand.cpp
*
* Author: Lee Crippen
* Created: 2/16/2017
*
* Description: Simple behavior to immediately respond to the voice command keyphrase, while waiting for further commands.
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToVoiceCommand.h"
#include "engine/components/movementComponent.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/voiceCommands/voiceCommandComponent.h"
#include "coretech/common/engine/math/pose.h"
#include "clad/types/animationTrigger.h"


namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToVoiceCommand::BehaviorReactToVoiceCommand(const Json::Value& config)
: ICozmoBehavior(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToVoiceCommand::WantsToBeActivatedBehavior() const
{
  if (_desiredFace.IsValid())
  {
    // If we don't know where this face is right now, switch it to Invalid so we just look toward the last face pose
    const auto* face = GetBEI().GetFaceWorld().GetFace(_desiredFace);
    Pose3d pose;

    const auto& robotInfo = GetBEI().GetRobotInfo();
    if(nullptr == face || !face->GetHeadPose().HasSameRootAs(robotInfo.GetPose()))
    {
      _desiredFace.Reset();
    }
  }
  
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnBehaviorActivated()
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  
  // Stop all movement so we can listen for a command
  robotInfo.GetMoveComponent().StopAllMotors();
  
  auto* actionSeries = new CompoundActionSequential();
  
  // Tilt the head up slightly, like we're listening, and wait a bit
  {
    actionSeries->AddAction(
      new TriggerLiftSafeAnimationAction(AnimationTrigger::VC_Listening));
  }
  
  // After waiting let's turn toward the face we know about, if we have one
  if(_desiredFace.IsValid()){
    const bool sayName = true;
    TurnTowardsFaceAction* turnAction = new TurnTowardsFaceAction(_desiredFace,
                                                                  M_PI_F,
                                                                  sayName);
    
    turnAction->SetSayNameAnimationTrigger(AnimationTrigger::AcknowledgeFaceNamed);
    turnAction->SetNoNameAnimationTrigger(AnimationTrigger::AcknowledgeFaceUnnamed);
    turnAction->SetMaxFramesToWait(5);
    actionSeries->AddAction(turnAction, false);
    
    // Play animation to indicate "You wanted something" since he's looking at the user
    actionSeries->AddAction(
       new TriggerLiftSafeAnimationAction(AnimationTrigger::VC_NoFollowupCommand_WithFace));
  }else{
    // Play animation to indicate "What was that" since no face to tun towards
    actionSeries->AddAction(
       new TriggerLiftSafeAnimationAction(AnimationTrigger::VC_NoFollowupCommand_NoFace));
  }
  
  using namespace ::Anki::Cozmo::VoiceCommand;
  
  auto completionCallback = [] (const ExternalInterface::RobotCompletedAction& completedActionInfo) {
    // If we got to the end of this series of actions without a command (or anything else) interrupting
    if (Anki::Cozmo::ActionResult::SUCCESS == completedActionInfo.result)
    {
      Anki::Util::sEvent("voice_command.no_observed_command_for_context", {},
                         EnumToString(VoiceCommandListenContext::CommandList));
    }
  };
  
  DelegateIfInControl(actionSeries, completionCallback);
  
  Anki::Util::sEvent("voice_command.responding_to_command", {}, EnumToString(VoiceCommandType::HeyCozmo));
  robotInfo.GetContext()->GetVoiceCommandComponent()->BroadcastVoiceEvent(RespondingToCommand(VoiceCommandType::HeyCozmo));
  robotInfo.GetContext()->GetVoiceCommandComponent()->BroadcastVoiceEvent(RespondingToCommandStart(VoiceCommandType::HeyCozmo));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToVoiceCommand::OnBehaviorDeactivated()
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  robotInfo.GetContext()->GetVoiceCommandComponent()->ForceListenContext(VoiceCommand::VoiceCommandListenContext::TriggerPhrase);
  
  using namespace ::Anki::Cozmo::VoiceCommand;
  robotInfo.GetContext()->GetVoiceCommandComponent()->BroadcastVoiceEvent(RespondingToCommandEnd(VoiceCommandType::HeyCozmo));
}

} // namespace Cozmo
} // namespace Anki
