/**
 * File: behaviorAcknowledgeObject.cpp
 *
 * Author:  Andrew Stein
 * Created: 2016-06-14
 *
 * Description:  Simple quick reaction to a "new" block, just to show Cozmo has noticed it.
 *               Cozmo just turns towards the object and then plays a reaction animation if
 *               it still sees the object after the turn.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorAcknowledgeObject.h"

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

  
BehaviorAcknowledgeObject::BehaviorAcknowledgeObject(Robot& robot, const Json::Value& config)
: IBehaviorPoseBasedAcknowledgement(robot, config)
{
  SetDefaultName("AcknowledgeObject");

  SubscribeToTags({{
    EngineToGameTag::RobotObservedObject,
    EngineToGameTag::RobotMarkedObjectPoseUnknown,
  }});
}

  
Result BehaviorAcknowledgeObject::InitInternal(Robot& robot)
{
  TurnTowardsObjectAction* turnAction = new TurnTowardsObjectAction(robot, _targetObject,
                                                                    _params.maxTurnAngle_rad);
  turnAction->ShouldDoRefinedTurn(false);
  turnAction->SetTiltTolerance(_params.tiltTolerance_rad);
  turnAction->SetPanTolerance(_params.panTolerance_rad);

  // If visual verification fails (e.g. because object has moved since we saw it),
  // then the compound action fails, and we don't play animation, since it's silly
  // to react to something that's no longer there.
  VisuallyVerifyObjectAction* verifyAction = new VisuallyVerifyObjectAction(robot, _targetObject);
  verifyAction->SetNumImagesToWaitFor(_params.numImagesToWaitFor);
  
  CompoundActionSequential* action = new CompoundActionSequential(robot, {
    turnAction,
    verifyAction,
    new TriggerAnimationAction(robot, _params.reactionAnimTrigger),
  });
  
  StartActing(action, _params.scoreIncreaseWhileReacting,
              [this](ActionResult res) {
                // Whether or not we succeeded, unset the target block
                // (We've already added it to the reacted set)
                _targetObject.UnSet();
              });

  
  return Result::RESULT_OK;
  
} // InitInternal()
  

float BehaviorAcknowledgeObject::EvaluateScoreInternal(const Robot& robot) const
{
  // TODO: compute a score based on how much we want to get distracted by the current target object
  
  return 1.0f;
}
  

bool BehaviorAcknowledgeObject::IsRunnableInternal(const Robot& robot) const
{
  return _targetObject.IsSet(); // TODO: Consider adding "&& !robot.IsCarryingObject();" ? 
}
  

void BehaviorAcknowledgeObject::HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedObject:
      HandleObjectObserved(robot, event.GetData().Get_RobotObservedObject());
      break;
      
    case EngineToGameTag::RobotMarkedObjectPoseUnknown:
      HandleObjectMarkedUnknown(robot, event.GetData().Get_RobotMarkedObjectPoseUnknown().objectID);
      break;

    default:
      PRINT_NAMED_ERROR("BehaviorAcknowledgeObject.HandleWhileNotRunning.InvalidTag",
                        "Received event with unhandled tag %hhu.",
                        event.GetData().GetTag());
      break;
  }
} // HandleWhileNotRunning()
  
  
void BehaviorAcknowledgeObject::HandleObjectObserved(const Robot& robot, const ExternalInterface::RobotObservedObject& msg)
{
  // Only objects whose marker we actually see are valid
  if( ! msg.markersVisible ) {
    return;
  }
  
  // Never get distracted by an object being carried or docked with
  if(msg.objectID == robot.GetCarryingObject() ||
     msg.objectID == robot.GetDockObject())
  {
    return;
  }
  
  // Object must be in one of the families this behavior cares about
  const bool hasValidFamily = _objectFamilies.count(msg.objectFamily) > 0;
  if(!hasValidFamily) {
    return;
  }
  
  Pose3d obsPose( msg.pose );
  
  ReactionData* data = nullptr;
  const bool alreadyReacted = GetReactionData(msg.objectID, data);
  if(alreadyReacted)
  {
    assert(nullptr != data);
    
    // We've already reacted to this object ID, but check if it's in a new location or cooldown has passed
    const bool isCoolDownOver = msg.timestamp - data->lastSeenTime_ms > _params.coolDownDuration_ms;
    const bool isPoseDifferent = !obsPose.IsSameAs(data->lastPose,
                                                   _params.samePoseDistThreshold_mm,
                                                   _params.samePoseAngleThreshold_rad);
    
    if(isCoolDownOver || isPoseDifferent)
    {
      // React again
      _targetObject = msg.objectID;
    }
    
    // Always keep the last observed pose updated, so we react when there's a quick big change,
    // not a slot incremental one. Also keep last observed time updated.
    data->lastPose = std::move(obsPose);
    data->lastSeenTime_ms = msg.timestamp;
  }
  else
  {
    // New object altogether, always react
    _targetObject = msg.objectID;
    
    ReactionData reactedObject{
      .lastPose        = std::move(obsPose),
      .lastSeenTime_ms = msg.timestamp,
    };
    
    AddReactionData(_targetObject.GetValue(), std::move(reactedObject));
  }
} // HandleObjectObserved()

  
void BehaviorAcknowledgeObject::HandleObjectMarkedUnknown(const Robot& robot, ObjectID objectID)
{
  const bool objectRemoved = RemoveReactionData(objectID);
  if(objectRemoved)
  {
    PRINT_NAMED_DEBUG("BehaviorAcknowledgeObject.HandleObjectMarkedUnknown",
                      "Removing Object %d from reacted set because it was marked unknown",
                      objectID.GetValue());
  }
} // HandleObjectMarkedUnknown()


} // namespace Cozmo
} // namespace Anki

