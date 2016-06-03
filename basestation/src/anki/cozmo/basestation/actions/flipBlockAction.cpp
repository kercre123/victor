/**
 * File: flipBlockAction.cpp
 *
 * Author: Al Chaussee
 * Date:   5/18/16
 *
 * Description: Action which flips blocks
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "flipBlockAction.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/robot.h"


namespace Anki {
  namespace Cozmo {
    
    DriveAndFlipBlockAction::DriveAndFlipBlockAction(Robot& robot, ObjectID objectID)
    : CompoundActionSequential(robot)
    {
      AddAction(new DriveToFlipBlockPoseAction(robot, objectID));
      FlipBlockAction* flipAction = new FlipBlockAction(robot, objectID);
      AddAction(flipAction);
      SetProxyTag(flipAction->GetTag()); // Use flip action's completion info
    }
    
    void DriveAndFlipBlockAction::SetMotionProfile(const PathMotionProfile& motionProfile)
    {
      if(GetActionList().size() == 2)
      {
        (static_cast<DriveToObjectAction*>(GetActionList().front()))->SetMotionProfile(motionProfile);
      }
    }
    
    
    DriveToFlipBlockPoseAction::DriveToFlipBlockPoseAction(Robot& robot, ObjectID objectID)
    : CompoundActionSequential(robot)
    , _blockToFlipID(objectID)
    {
      if(objectID == robot.GetCarryingObject())
      {
        PRINT_NAMED_WARNING("IDriveToInteractWithObject.Constructor",
                            "Robot is currently carrying action object with ID=%d",
                            objectID.GetValue());
        return;
      }
      
      DriveToObjectAction* driveToObjectAction = new DriveToObjectAction(robot,
                                                                         objectID,
                                                                         PreActionPose::FLIPPING,
                                                                         0,
                                                                         false,
                                                                         0,
                                                                         false);
      
      AddAction(driveToObjectAction);
    }
    
    const std::string& DriveToFlipBlockPoseAction::GetName() const
    {
      static const std::string name("DriveToFlipBlockPoseAction");
      return name;
    }
    
    void DriveToFlipBlockPoseAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted info;
      info.numObjects = 1;
      info.objectIDs[0] = _blockToFlipID;
      completionUnion.Set_objectInteractionCompleted(std::move( info ));
    }
    
    
    FlipBlockAction::FlipBlockAction(Robot& robot, ObjectID objectID)
    : IAction(robot)
    , _objectID(objectID)
    , _compoundAction(robot)
    {
      
    }
    
    FlipBlockAction::~FlipBlockAction()
    {
      if(_flipTag != -1)
      {
        _robot.GetActionList().Cancel(_flipTag);
      }
    }
    
    const std::string& FlipBlockAction::GetName() const
    {
      static const std::string name("FlipBlockAction");
      return name;
    }
    
    ActionResult FlipBlockAction::Init()
    {
      IDockAction::PreActionPoseInfo preActionPoseInfo(_objectID,
                                                       PreActionPose::FLIPPING,
                                                       true,
                                                       0,
                                                       kPreDockPoseAngleTolerance);
      IDockAction::IsCloseEnoughToPreActionPose(_robot, preActionPoseInfo);
      
      if(preActionPoseInfo.actionResult != ActionResult::SUCCESS)
      {
        return preActionPoseInfo.actionResult;
      }
      
      ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(_objectID);
      Pose3d p;
      object->GetPose().GetWithRespectTo(_robot.GetPose(), p);
    
      // Need to suppress track locking so the two lift actions don't fail because the other locked the lift track
      // A little dangerous as animations playing in parallel to this action could move lift
      _compoundAction.ShouldSuppressTrackLocking(true);
      
      // Drive through the block
      DriveStraightAction* drive = new DriveStraightAction(_robot, p.GetTranslation().Length() + kDrivingDist_mm, kDrivingSpeed_mmps);
      
      // Need to set the initial lift height to fit lift base into block corner edge
      MoveLiftToHeightAction* initialLift = new MoveLiftToHeightAction(_robot, kInitialLiftHeight_mm);
      
      _compoundAction.AddAction(initialLift);
      _compoundAction.AddAction(drive);
      _compoundAction.Update();
      return ActionResult::SUCCESS;
    }
    
    ActionResult FlipBlockAction::CheckIfDone()
    {
      ActionResult result = _compoundAction.Update();
      if(result != ActionResult::RUNNING)
      {
        // By clearing the bottom block the entire stack will get cleared
        _robot.GetBlockWorld().ClearObject(_objectID);
        return result;
      }
      
      ActionableObject* object = dynamic_cast<ActionableObject*>(_robot.GetBlockWorld().GetObjectByID(_objectID));
      Pose3d p;
      object->GetPose().GetWithRespectTo(_robot.GetPose(), p);
      if(p.GetTranslation().Length() < kDistToObjectToFlip_mm && _flipTag == -1)
      {
        IAction* action = new MoveLiftToHeightAction(_robot, MoveLiftToHeightAction::Preset::CARRY);
        action->ShouldEmitCompletionSignal(false);
        
        // FlipBlockAction is already locking all tracks so this lift action doesn't need to lock
        action->ShouldSuppressTrackLocking(true);
        _flipTag = action->GetTag();
        _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, action);
      }
    
      return ActionResult::RUNNING;
    }
    
  }
}