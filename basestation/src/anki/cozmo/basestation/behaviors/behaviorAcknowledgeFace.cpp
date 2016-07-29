/**
 * File: behaviorAcknowledgeFace.cpp
 *
 * Author:  Andrew Stein
 * Created: 2016-06-16
 *
 * Description:  Simple quick reaction to a "new" face, just to show Cozmo has noticed you.
 *               Cozmo just turns towards the face and then plays a reaction animation.

 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorAcknowledgeFace.h"

#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageFromActiveObject.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
BehaviorAcknowledgeFace::BehaviorAcknowledgeFace(Robot& robot, const Json::Value& config)
: IBehaviorPoseBasedAcknowledgement(robot, config)
{
  SetDefaultName("AcknowledgeFace");

  SubscribeToTags({{
    EngineToGameTag::RobotObservedFace,
    EngineToGameTag::RobotDeletedFace,
  }});

}
  
  
Result BehaviorAcknowledgeFace::InitInternal(Robot& robot)
{
  TurnTowardsPoseAction* turnAction = new TurnTowardsFaceAction(robot, _targetFace, _params.maxTurnAngle_rad);
  turnAction->SetTiltTolerance(_params.tiltTolerance_rad);
  turnAction->SetPanTolerance(_params.panTolerance_rad);

  // Will fail the action if we don't see the face, so we don't play the reaction
  // not looking at a face
  VisuallyVerifyFaceAction* verifyAction = new VisuallyVerifyFaceAction(robot, _targetFace);
  verifyAction->SetNumImagesToWaitFor(_params.numImagesToWaitFor);
  
  CompoundActionSequential* action = new CompoundActionSequential(robot, {
    turnAction,
    verifyAction,
    new TriggerAnimationAction(robot, _params.reactionAnimTrigger),
  });
  
  StartActing(action, _params.scoreIncreaseWhileReacting,
              [this](ActionResult res) {
                // Whether or not we succeeded, unset the target face
                // (We've already added it to the reacted set)
                _targetFace = Vision::UnknownFaceID;
              });

  
  return Result::RESULT_OK;
  
} // InitInternal()
  

float BehaviorAcknowledgeFace::EvaluateScoreInternal(const Robot& robot) const
{
  // TODO: compute a score based on how much we want to get distracted by the current target object
  
  return 1.0f;
}
  
  
bool BehaviorAcknowledgeFace::IsRunnableInternal(const Robot& robot) const
{
  return _targetFace != Vision::UnknownFaceID;
}
  

void BehaviorAcknowledgeFace::HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedFace:
      HandleFaceObserved(robot, event.GetData().Get_RobotObservedFace());
      break;
      
    case EngineToGameTag::RobotDeletedFace:
      HandleFaceDeleted(robot, event.GetData().Get_RobotDeletedFace().faceID);
      break;

    default:
      PRINT_NAMED_ERROR("BehaviorAcknowledgeFace.HandleWhileNotRunning.InvalidTag",
                        "Received event with unhandled tag %hhu.",
                        event.GetData().GetTag());
      break;
  }
} // HandleWhileNotRunning()
  
  
void BehaviorAcknowledgeFace::HandleFaceObserved(const Robot& robot, const ExternalInterface::RobotObservedFace& msg)
{
  Pose3d facePose( msg.pose, robot.GetPoseOriginList() );
  
  ReactionData* data = nullptr;
  const bool alreadyReacted  = GetReactionData(msg.faceID, data);
  if(alreadyReacted)
  {
    assert(nullptr != data);
    
    // We've already reacted to this object ID, but check if it's in a new location or cooldown has passed
    const bool isCoolDownOver = msg.timestamp - data->lastSeenTime_ms > _params.coolDownDuration_ms;
    const bool isPoseDifferent = !facePose.IsSameAs(data->lastPose,
                                                    _params.samePoseDistThreshold_mm,
                                                    _params.samePoseAngleThreshold_rad);
    
    if(isCoolDownOver || isPoseDifferent)
    {
      // React again
      _targetFace = msg.faceID;
    }
    
    // Always keep the last observed pose updated, so we react when there's a quick big change,
    // not a slow incremental one. Also keep last observed time updated.
    data->lastPose = std::move(facePose);
    data->lastSeenTime_ms = msg.timestamp;
  }
  else
  {
    // New object altogether, always react
    _targetFace = msg.faceID;
    
    ReactionData reactedFace{
      .lastPose        = std::move(facePose),
      .lastSeenTime_ms = msg.timestamp,
    };
    
    AddReactionData(_targetFace, std::move(reactedFace));
  }
} // HandleObjectObserved()

  
void BehaviorAcknowledgeFace::HandleFaceDeleted(const Robot& robot, Vision::FaceID_t faceID)
{
  const bool faceRemoved = RemoveReactionData(faceID);
  if(faceRemoved) {
    PRINT_NAMED_DEBUG("BehaviorAcknowledgeFace.HandleFaceDeleted",
                      "Removing Face %d from reacted set because it was deleted",
                      faceID);
  }
} // HandleObjectMarkedUnknown()


} // namespace Cozmo
} // namespace Anki

