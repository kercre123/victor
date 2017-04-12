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
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

namespace{
static const int kTappedObjectPoseZThreshold_mm = 10;

constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersDoubleTapArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    false},
  {ReactionTrigger::DoubleTapDetected,            false},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::PyramidInitialDetection,      false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::StackOfCubesInitialDetection, false},
  {ReactionTrigger::UnexpectedMovement,           false},
  {ReactionTrigger::VC,                           false}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersDoubleTapArray),
              "Reaction triggers duplicate or non-sequential");

}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToDoubleTap::BehaviorReactToDoubleTap(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("ReactToDoubleTap");
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToDoubleTap::InitInternal(Robot& robot)
{
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
      // al/andrew/raul: We think that we no longer need to rely on LastKnown pose, which is currently hard to
      // support (due to rejiggers), so simply search for the cube when we have no idea where it is
      TransitionToSearchForCube(robot);
      
      // note that by leaving action = nullptr we do not try to run other than the transition one
      action = nullptr;
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
      auto* turnAction = new TurnTowardsObjectAction(robot, ObjectID());
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
    
    // if an action was set, start it now
    if ( nullptr != action )
    {
      StartActing(action,
                [this, &robot](const ExternalInterface::RobotCompletedAction& msg) {

                  // If we can't see the object after turning towards it then try to drive to it
                  if(msg.result == ActionResult::VISUAL_OBSERVATION_FAILED)
                  {
                    // Note the object may have recently become Unknown/deleted (for example because we turned
                    // and then realized it's not there)
                    // If the object is still located, drive to it, if it's not, search for it
                    const ObjectID& objectID = robot.GetBehaviorManager().GetCurrTappedObject();
                    const ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(objectID);
                    if ( nullptr != object ) {
                      TransitionToDriveToCube(robot);
                    } else {
                      TransitionToSearchForCube(robot);
                    }
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
    } // nullptr != action
    
    SmartDisableReactionsWithLock(GetName(), kAffectTriggersDoubleTapArray);
    
    return RESULT_OK;
  }
  return RESULT_FAIL;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
    
  // Let whiteboard know that this behavior can't react to the same object again
  robot.GetAIComponent().GetWhiteboard().SetReactToDoubleTapCanReactAgain(false);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToDoubleTap::TransitionToDriveToCube(Robot& robot)
{
  const ObjectID& objectID = robot.GetBehaviorManager().GetCurrTappedObject();
  
  // al/andrew/rsam: SetObjectCanBeUnknown would need to know about LastPose. This is not currently supported,
  // and we think it's ok to not support at the moment. If it's still calling it here,
  
  DriveToObjectAction* action = new DriveToObjectAction(robot, objectID, PreActionPose::ActionType::DOCKING);
  
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

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToDoubleTap::TransitionToSearchForCube(Robot& robot)
{
  const ObjectID& objectID = robot.GetBehaviorManager().GetCurrTappedObject();
  
  SearchForNearbyObjectAction* action = new SearchForNearbyObjectAction(robot, objectID);
  
  const f32 minWait_sec = 0.3f;
  const f32 maxWait_sec = 0.5f;
  action->SetSearchWaitTime(minWait_sec, maxWait_sec);
  
  StartActing(action,
              [this, &robot](const ExternalInterface::RobotCompletedAction& msg) {
                if(IActionRunner::GetActionResultCategory(msg.result) == ActionResultCategory::CANCELLED)
                {
                  return;
                }
                
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

} // namespace Cozmo
} // namespace Anki
