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

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToVoiceCommand.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeFace.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/voiceCommands/voiceCommandComponent.h"
#include "anki/common/basestation/math/pose.h"
#include "clad/types/animationTrigger.h"


namespace Anki {
namespace Cozmo {

namespace {
  const float kReactToTriggerDelayTime_s = 4.0f;
  
  constexpr ReactionTriggerHelpers::FullReactionArray kDisableReactionTriggersArray = {
    {ReactionTrigger::CliffDetected,                false},
    {ReactionTrigger::CubeMoved,                    true},
    {ReactionTrigger::DoubleTapDetected,            true},
    {ReactionTrigger::FacePositionUpdated,          true},
    {ReactionTrigger::FistBump,                     true},
    {ReactionTrigger::Frustration,                  true},
    {ReactionTrigger::Hiccup,                       false},
    {ReactionTrigger::MotorCalibration,             false},
    {ReactionTrigger::NoPreDockPoses,               false},
    {ReactionTrigger::ObjectPositionUpdated,        false},
    {ReactionTrigger::PlacedOnCharger,              false},
    {ReactionTrigger::PetInitialDetection,          true},
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
}

static_assert(ReactionTriggerHelpers::IsSequentialArray(kDisableReactionTriggersArray),
              "Reaction triggers duplicate or non-sequential");
  
BehaviorReactToVoiceCommand::BehaviorReactToVoiceCommand(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
}


bool BehaviorReactToVoiceCommand::IsRunnableInternal(const BehaviorPreReqAcknowledgeFace& preReqData) const
{
  const auto& desiredTargets = preReqData.GetDesiredTargets();
  if (!ANKI_VERIFY(desiredTargets.size() == 1, "BehaviorReactToVoiceCommand.IsRunnableInternal.PreReqDataInvalid", "PreReqData needs exactly 1 faceID"))
  {
    return false;
  }
  
  _desiredFace = *(desiredTargets.begin());
  
  if (Vision::UnknownFaceID != _desiredFace)
  {
    // If we don't know where this face is right now, switch it to Invalid so we just look toward the last face pose
    const auto& robot = preReqData.GetRobot();
    const auto* face = robot.GetFaceWorld().GetFace(_desiredFace);
    Pose3d pose;
    if(nullptr == face || !face->GetHeadPose().GetWithRespectTo(robot.GetPose(), pose))
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
    auto* parallelActions = new CompoundActionParallel(robot);
    parallelActions->AddAction(new MoveHeadToAngleAction(robot, DEG_TO_RAD(10.f)));
    parallelActions->AddAction(new WaitAction(robot, kReactToTriggerDelayTime_s));
    actionSeries->AddAction(parallelActions);
  }
  
  // After waiting let's turn toward the face we know about, if we have one
  {
    const bool sayName = true;
    TurnTowardsFaceAction* turnAction = new TurnTowardsFaceAction(robot,
                                                                  _desiredFace,
                                                                  M_PI_F,
                                                                  sayName);
    
    turnAction->SetSayNameAnimationTrigger(AnimationTrigger::AcknowledgeFaceNamed);
    turnAction->SetNoNameAnimationTrigger(AnimationTrigger::AcknowledgeFaceUnnamed);
    turnAction->SetMaxFramesToWait(5);
    actionSeries->AddAction(turnAction);
  }
  
  StartActing(actionSeries);
  
  using namespace ::Anki::Cozmo::VoiceCommand;
  robot.GetContext()->GetVoiceCommandComponent()->BroadcastVoiceEvent(RespondingToCommandStart(VoiceCommandType::HeyCozmo));
  
  return RESULT_OK;
}
  
void BehaviorReactToVoiceCommand::StopInternal(Robot& robot)
{
  robot.GetContext()->GetVoiceCommandComponent()->SetListenContext(VoiceCommand::VoiceCommandListenContext::Keyphrase);
  
  using namespace ::Anki::Cozmo::VoiceCommand;
  robot.GetContext()->GetVoiceCommandComponent()->BroadcastVoiceEvent(RespondingToCommandEnd(VoiceCommandType::HeyCozmo));
}

} // namespace Cozmo
} // namespace Anki
