/**
 * File: flipBlockAction.cpp
 *
 * Author: Al Chaussee
 * Date:   5/18/16
 *
 * Description: Action which flips blocks
 *              By default, when driving to the flipping preAction pose, we will drive to one of the two poses that is closest
 *              to Cozmo and farthest from the last known face. In order to maximize the chances of the person being able to see
 *              Cozmo's face and reactions while he is flipping the block
 *                - Should there not be a last known face, of the two closest preAction poses the left most will be chosen
 *
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
  
    static constexpr f32 kPreDockPoseAngleTolerance = DEG_TO_RAD_F32(5);
    
    DriveAndFlipBlockAction::DriveAndFlipBlockAction(Robot& robot,
                                                     const ObjectID objectID,
                                                     const bool shouldDriveToClosestPreActionPose,
                                                     const bool useApproachAngle,
                                                     const f32 approachAngle_rad,
                                                     const bool useManualSpeed,
                                                     Radians maxTurnTowardsFaceAngle_rad,
                                                     const bool sayName)
    : IDriveToInteractWithObject(robot,
                                 objectID,
                                 PreActionPose::FLIPPING,
                                 0,
                                 useApproachAngle,
                                 approachAngle_rad,
                                 useManualSpeed,
                                 maxTurnTowardsFaceAngle_rad,
                                 sayName)
    , _flipBlockAction(new FlipBlockAction(robot, objectID))
    , _shouldDriveToClosestPreActionPose(shouldDriveToClosestPreActionPose)
    {
      GetDriveToObjectAction()->SetGetPossiblePosesFunc(&DriveAndFlipBlockAction::GetPossiblePoses);

      AddAction(_flipBlockAction);
      SetProxyTag(_flipBlockAction->GetTag()); // Use flip action's completion info
    }
    
    ActionResult DriveAndFlipBlockAction::GetPossiblePoses(ActionableObject* object,
                                                           std::vector<Pose3d>& possiblePoses,
                                                           bool& alreadyInPosition)
    {
      IDockAction::PreActionPoseInfo preActionPoseInfo(object->GetID(),
                                                       PreActionPose::FLIPPING,
                                                       false,
                                                       0,
                                                       kPreDockPoseAngleTolerance);
      
      IDockAction::IsCloseEnoughToPreActionPose(_robot, preActionPoseInfo);
      
      if(preActionPoseInfo.actionResult == ActionResult::FAILURE_ABORT)
      {
        PRINT_NAMED_WARNING("DriveToFlipBlockPoseAction.Constructor", "Failed to find closest preAction pose");
        return ActionResult::FAILURE_ABORT;
      }
      
      Pose3d facePose;
      TimeStamp_t faceTime = _robot.GetFaceWorld().GetLastObservedFace(facePose);
      
      if(preActionPoseInfo.preActionPoses.size() == 0)
      {
        PRINT_NAMED_WARNING("DriveToFlipBlockPoseAction.Constructor", "No preAction poses");
        return ActionResult::FAILURE_ABORT;
      }
      
      if(_shouldDriveToClosestPreActionPose)
      {
        possiblePoses.push_back(preActionPoseInfo.preActionPoses[preActionPoseInfo.closestIndex].GetPose());
        return ActionResult::SUCCESS;
      }
      
      Pose3d firstClosestPose;
      Pose3d secondClosestPose;
      f32 firstClosestDist = std::numeric_limits<float>::max();
      f32 secondClosestDist = firstClosestDist;
      
      for(auto iter = preActionPoseInfo.preActionPoses.begin(); iter != preActionPoseInfo.preActionPoses.end(); ++iter)
      {
        Pose3d poseWrtRobot;
        if(!iter->GetPose().GetWithRespectTo(_robot.GetPose(), poseWrtRobot))
        {
          continue;
        }
        
        f32 dist = poseWrtRobot.GetTranslation().Length();
        
        
        if(dist < firstClosestDist)
        {
          secondClosestDist = firstClosestDist;
          secondClosestPose = firstClosestPose;
          
          firstClosestDist = dist;
          firstClosestPose = poseWrtRobot;
        }
        else if(dist < secondClosestDist)
        {
          secondClosestDist = dist;
          secondClosestPose = poseWrtRobot;
        }
      }
      
      Pose3d poseToDriveTo;
      
      // There is only one preaction pose so it will be the first closest
      if(preActionPoseInfo.preActionPoses.size() == 1)
      {
        poseToDriveTo = firstClosestPose;
      }
      // No last known face so pick the preaction pose that is left most relative to Cozmo
      else if(faceTime == 0)
      {
        if(firstClosestPose.GetTranslation().y() >= secondClosestPose.GetTranslation().y())
        {
          poseToDriveTo = firstClosestPose;
        }
        else
        {
          poseToDriveTo = secondClosestPose;
        }
      }
      // Otherwise pick one of the two preaction poses closest to the robot and farthest from the last known face
      else
      {
        if(!firstClosestPose.GetWithRespectTo(facePose, firstClosestPose))
        {
          PRINT_NAMED_WARNING("DriveToFlipBlockPoseAction", "Couldn't get firstClosestPose wrt facePose");
          return ActionResult::FAILURE_ABORT;
        }
        if(!secondClosestPose.GetWithRespectTo(facePose, secondClosestPose))
        {
          PRINT_NAMED_WARNING("DriveToFlipBlockPoseAction", "Couldn't get secondClosestPose wrt facePose");
          return ActionResult::FAILURE_ABORT;
        }
        
        if(firstClosestPose.GetTranslation().Length() > secondClosestPose.GetTranslation().Length())
        {
          firstClosestPose.GetWithRespectTo(_robot.GetPose(), poseToDriveTo);
        }
        else
        {
          secondClosestPose.GetWithRespectTo(_robot.GetPose(), poseToDriveTo);
        }
      }
      
      possiblePoses.push_back(poseToDriveTo);
      return ActionResult::SUCCESS;
    }
    
    
    DriveToFlipBlockPoseAction::DriveToFlipBlockPoseAction(Robot& robot, ObjectID objectID)
    : DriveToObjectAction(robot, objectID, PreActionPose::FLIPPING)
    {
      SetGetPossiblePosesFunc(&DriveAndFlipBlockAction::GetPossiblePoses);
    }
    
    const std::string& DriveToFlipBlockPoseAction::GetName() const
    {
      static const std::string name("DriveToFlipBlockPoseAction");
      return name;
    }
    
    
    FlipBlockAction::FlipBlockAction(Robot& robot, ObjectID objectID)
    : IAction(robot)
    , _objectID(objectID)
    , _compoundAction(robot)
    {
      
    }
    
    FlipBlockAction::~FlipBlockAction()
    {
      _compoundAction.PrepForCompletion();
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