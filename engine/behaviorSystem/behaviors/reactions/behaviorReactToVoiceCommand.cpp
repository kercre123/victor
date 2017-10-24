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
#include "engine/behaviorSystem/behaviors/reactions/behaviorReactToVoiceCommand.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "engine/components/movementComponent.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"
#include "engine/voiceCommands/voiceCommandComponent.h"
#include "anki/common/basestation/math/pose.h"
#include "clad/types/animationTrigger.h"


namespace Anki {
namespace Cozmo {

namespace {
constexpr ReactionTriggerHelpers::FullReactionArray kDisableReactionTriggersArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             true},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           true},
  {ReactionTrigger::VC,                           false}
};
} // namespace

static_assert(ReactionTriggerHelpers::IsSequentialArray(kDisableReactionTriggersArray),
              "Reaction triggers duplicate or non-sequential");
  
BehaviorReactToVoiceCommand::BehaviorReactToVoiceCommand(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
}


bool BehaviorReactToVoiceCommand::IsRunnableInternal(const Robot& robot) const
{
  if (Vision::UnknownFaceID != _desiredFace)
  {
    // If we don't know where this face is right now, switch it to Invalid so we just look toward the last face pose
    const auto* face = robot.GetFaceWorld().GetFace(_desiredFace);
    if(nullptr == face || !face->GetHeadPose().HasSameRootAs(robot.GetPose()))
    {
      _desiredFace = Vision::UnknownFaceID;
    }
  }
  
  return true;
}

Result BehaviorReactToVoiceCommand::InitInternal(Robot& robot)
{
  SmartDisableReactionsWithLock(GetIDStr(), kDisableReactionTriggersArray);
  
  // Stop all movement so we can listen for a command
  robot.GetMoveComponent().StopAllMotors();
  
  auto* actionSeries = new CompoundActionSequential(robot);
  
  // Tilt the head up slightly, like we're listening, and wait a bit
  {
    actionSeries->AddAction(
      new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::VC_Listening));
  }
  
  // After waiting let's turn toward the face we know about, if we have one
  if(_desiredFace != Vision::UnknownFaceID){
    const bool sayName = true;
    TurnTowardsFaceAction* turnAction = new TurnTowardsFaceAction(robot,
                                                                  _desiredFace,
                                                                  M_PI_F,
                                                                  sayName);
    
    turnAction->SetSayNameAnimationTrigger(AnimationTrigger::AcknowledgeFaceNamed);
    turnAction->SetNoNameAnimationTrigger(AnimationTrigger::AcknowledgeFaceUnnamed);
    turnAction->SetMaxFramesToWait(5);
    actionSeries->AddAction(turnAction, false);
    
    // Play animation to indicate "You wanted something" since he's looking at the user
    actionSeries->AddAction(
       new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::VC_NoFollowupCommand_WithFace));
  }else{
    // Play animation to indicate "What was that" since no face to tun towards
    actionSeries->AddAction(
       new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::VC_NoFollowupCommand_NoFace));
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
  
  StartActing(actionSeries, completionCallback);
  
  Anki::Util::sEvent("voice_command.responding_to_command", {}, EnumToString(VoiceCommandType::HeyCozmo));
  robot.GetContext()->GetVoiceCommandComponent()->BroadcastVoiceEvent(RespondingToCommand(VoiceCommandType::HeyCozmo));
  robot.GetContext()->GetVoiceCommandComponent()->BroadcastVoiceEvent(RespondingToCommandStart(VoiceCommandType::HeyCozmo));
  
  return RESULT_OK;
}
  
void BehaviorReactToVoiceCommand::StopInternal(Robot& robot)
{
  robot.GetContext()->GetVoiceCommandComponent()->ForceListenContext(VoiceCommand::VoiceCommandListenContext::TriggerPhrase);
  
  using namespace ::Anki::Cozmo::VoiceCommand;
  robot.GetContext()->GetVoiceCommandComponent()->BroadcastVoiceEvent(RespondingToCommandEnd(VoiceCommandType::HeyCozmo));
}

} // namespace Cozmo
} // namespace Anki
