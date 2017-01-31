 /**
 * File: behaviorReactToDoubleTap.cpp
 *
 * Author: Al Chaussee
 * Created: 11/9/2016
 *
 * Description: Behavior to react when a unknown or dirty cube is double tapped
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToDoubleTap.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

  static const int kTappedObjectPoseZThreshold_mm = 10;
  
BehaviorReactToDoubleTap::BehaviorReactToDoubleTap(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("ReactToDoubleTap");
}

bool BehaviorReactToDoubleTap::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  // Is runnable while we have a double tapped object that has a not known pose (dirty or unknown),
  // the robot is not picking or placing, and we have not already reacted to the current tapped object
  // This prevents this behavior from running should the current tapped object become dirty from random moved
  // messages
  const Robot& robot = preReqData.GetRobot();
  return (!robot.IsPickingOrPlacing() &&
          IsTappedObjectValid(robot) &&
          (_lastObjectReactedTo != robot.GetBehaviorManager().GetCurrTappedObject() ||
           robot.GetAIComponent().GetWhiteboard().CanReactToDoubleTapReactAgain()) &&
          !robot.GetAIComponent().GetWhiteboard().IsSuppressingReactToDoubleTap());
}

Result BehaviorReactToDoubleTap::InitInternal(Robot& robot)
{
  _objectInCurrentFrame = true;
  _turningTowardsGhostObject = false;
  _leaveTapInteractionOnStop = false;
  
  if(IsTappedObjectValid(robot))
  {
    const ObjectID& objectID = robot.GetBehaviorManager().GetCurrTappedObject();

    BlockWorldFilter filter;
    filter.SetAllowedIDs({objectID});
    filter.SetFilterFcn(&BlockWorldFilter::ActiveObjectsFilter);
    filter.SetOriginMode(BlockWorldFilter::OriginMode::InRobotFrame);
    ObservableObject* object = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
    
    IAction* action = nullptr;
    
    // Tapped object does not exist in the current frame
    if(object == nullptr)
    {
      filter.SetOriginMode(BlockWorldFilter::OriginMode::NotInRobotFrame);
      object = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
      if(object == nullptr)
      {
        // This can happen if the object has never been seen. Use 0,0,0 pose, which is what the old
        // code used to do for Unknown
        Pose3d zeroPose;
        zeroPose.SetParent(robot.GetWorldOrigin());
        
        
        PRINT_NAMED_ERROR("BehaviorReactToDoubleTap.UnknownPose",
                          "ObjectID %d not found in any frame. Using zero pose.",
                          objectID.GetValue());
        action = new TurnTowardsPoseAction(robot, zeroPose, DEG_TO_RAD(180.f));
      }
      else
      {
        PRINT_CH_INFO("Behaviors", "ReactToDoubleTap.ObjectInOtherFrame",
                      "Turning towards pose in old frame");
      
        action = new TurnTowardsPoseAction(robot, object->GetPose(), DEG_TO_RAD(180.f));
      }
      _objectInCurrentFrame = false;
      
      // Treat this as the same as turning towards a ghost object
      _turningTowardsGhostObject = true;
    }
    // If the tapped object's pose is in the ground (probably at 0,0) then we should artificially move it
    // up as if it was level with the robot so our head angle when we turn towards it will be in a position to
    // see markers
    else if(object->GetPose().GetWithRespectToOrigin().GetTranslation().z() < kTappedObjectPoseZThreshold_mm)
    {
      _ghostObject.reset(object->CloneType());
      Pose3d p = object->GetPose().GetWithRespectToOrigin();
      p.SetTranslation({
        object->GetPose().GetTranslation().x(),
        object->GetPose().GetTranslation().y(),
        object->GetSize().z()*0.5f
      });
      
      _ghostObject->InitPose(p, object->GetPoseState());
      auto* turnAction = new TurnTowardsObjectAction(robot, ObjectID(), DEG_TO_RAD(180.f));
      turnAction->UseCustomObject(_ghostObject.get());
      
      // Don't do a refined turn because in many cases when the object is in the ground it is because
      // we just delocalized and both the object and the robot are now at the origin. Turning around
      // the object's origin causes our initial turn angle and the refined turn angle to vary greatly, due to
      // the robot not turning exactly around its own origin.
      turnAction->ShouldDoRefinedTurn(false);
      action = turnAction;
      _turningTowardsGhostObject = true;
    }
    // Otherwise just turn towards the object
    else
    {
      action = new TurnTowardsObjectAction(robot, objectID, Radians(DEG_TO_RAD(180.f)), true, false);
    }
    
    StartActing(action,
                [this, &robot](const ExternalInterface::RobotCompletedAction& msg) {
                  // Who knows what pose the object has that is in another frame so don't try to
                  // drive to it but still do a little search
                  if(!_objectInCurrentFrame)
                  {
                    TransitionToSearchForCube(robot);
                    return;
                  }
                
                  // If we can't see the object after turning towards it then try to drive to it
                  if(msg.result == ActionResult::VISUAL_OBSERVATION_FAILED)
                  {
                    TransitionToDriveToCube(robot);
                  }
                  else if(msg.result == ActionResult::SUCCESS)
                  {
                    const ObjectID& objectID = robot.GetBehaviorManager().GetCurrTappedObject();
                    const ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(objectID);
                    
                    // If we turned towards the object and are seeing it but it's pose is still dirty then
                    // we should drive closer
                    if(object != nullptr &&
                       object->GetPoseState() == PoseState::Dirty)
                    {
                      TransitionToDriveToCube(robot);
                    }
                    else
                    {
                      if(_turningTowardsGhostObject)
                      {
                        TransitionToSearchForCube(robot);
                      }
                      else
                      {
                        _lastObjectReactedTo = robot.GetBehaviorManager().GetCurrTappedObject();
                      }
                    }
                  }
                  else
                  {
                    TransitionToSearchForCube(robot);
                  }
                });
    
    robot.GetBehaviorManager().RequestEnableReactionTrigger("ReactToDoubleTap", ReactionTrigger::ObjectPositionUpdated, false);
    
    return RESULT_OK;
  }
  return RESULT_FAIL;
}

void BehaviorReactToDoubleTap::StopInternal(Robot& robot)
{
  if(_leaveTapInteractionOnStop)
  {
    // Update the lights before leaving object tap interaction
    UpdateTappedObjectLights(false);
    
    robot.GetBehaviorManager().LeaveObjectTapInteraction();
  }
  else
  {
    // Update the tapped interaction since the tapped object may now be dirty/known and usable by the
    // object tap interaction behaviors
    const ObjectID& objectID = robot.GetBehaviorManager().GetCurrTappedObject();
    robot.GetAIComponent().GetWhiteboard().SetObjectTapInteraction(objectID);
    
    UpdateTappedObjectLights(true);
  }

  robot.GetBehaviorManager().RequestEnableReactionTrigger("ReactToDoubleTap", ReactionTrigger::ObjectPositionUpdated, true);
  
  // Let whiteboard know that this behavior can't react to the same object again
  robot.GetAIComponent().GetWhiteboard().SetReactToDoubleTapCanReactAgain(false);
}

bool BehaviorReactToDoubleTap::IsTappedObjectValid(const Robot& robot) const
{
  if(robot.GetAIComponent().GetWhiteboard().HasTapIntent())
  {
    const ObjectID& objectID = robot.GetBehaviorManager().GetCurrTappedObject();
    const ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(objectID);
    
    // The double tapped cube is valid for this reaction as long as it is not known and is not being
    // carried
    if(robot.GetCarryingObject() != objectID)
    {
      // If the object doesn't exist in the current frame we still want to react to the double tap
      if(object == nullptr)
      {
        return true;
      }
      
      return !object->IsPoseStateKnown();
    }
  }
  
  return false;
}

void BehaviorReactToDoubleTap::TransitionToDriveToCube(Robot& robot)
{
  const ObjectID& objectID = robot.GetBehaviorManager().GetCurrTappedObject();
  
  DriveToObjectAction* action = new DriveToObjectAction(robot, objectID, PreActionPose::ActionType::DOCKING);
  
  // It is ok to drive to an object with an unknown pose
  action->SetObjectCanBeUnknown(true);
  
  // Don't care if we don't get exactly where we need to
  action->DoPositionCheckOnPathCompletion(false);
  
  StartActing(action,
              [this, &robot](const ExternalInterface::RobotCompletedAction& msg) {
                // If the action failed (failed to drive or failed to visually verify) then
                // do a little search
                if(msg.result != ActionResult::SUCCESS)
                {
                  TransitionToSearchForCube(robot);
                }
                else
                {
                  _lastObjectReactedTo = robot.GetBehaviorManager().GetCurrTappedObject();
                }
              });
}

void BehaviorReactToDoubleTap::TransitionToSearchForCube(Robot& robot)
{
  const ObjectID& objectID = robot.GetBehaviorManager().GetCurrTappedObject();
  
  SearchForNearbyObjectAction* action = new SearchForNearbyObjectAction(robot, objectID);
  
  const f32 minWait_sec = 0.3f;
  const f32 maxWait_sec = 0.5f;
  action->SetSearchWaitTime(minWait_sec, maxWait_sec);
  
  StartActing(action,
              [this, &robot](const ExternalInterface::RobotCompletedAction& msg) {
                if(msg.result != ActionResult::SUCCESS)
                {
                  PRINT_CH_INFO("Behaviors", "BehaviorReactToDoubleTap.CantFindCube",
                                "Searching for cube failed, leaving object tap interaction");
                  _leaveTapInteractionOnStop = true;
                }
                else
                {
                  _lastObjectReactedTo = robot.GetBehaviorManager().GetCurrTappedObject();
                }
              });
}


}
}
