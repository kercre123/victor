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
#include "anki/cozmo/basestation/robot.h"


namespace Anki {
  namespace Cozmo {
    
    DriveAndFlipBlockAction::DriveAndFlipBlockAction(Robot& robot, ObjectID objectID)
    : CompoundActionSequential(robot)
    {
      AddAction(new DriveToFlipBlockPoseAction(robot, objectID));
      AddAction(new FlipBlockAction(robot));
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
    
    void DriveToFlipBlockPoseAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted info;
      info.numObjects = 1;
      info.objectIDs[0] = _blockToFlipID;
      completionUnion.Set_objectInteractionCompleted(std::move( info ));
    }
    
    
    FlipBlockAction::FlipBlockAction(Robot& robot)
    : CompoundActionSequential(robot)
    {
      // Need to suppress track locking so the two lift actions don't fail because the other locked the lift track
      // A little dangerous as animations playing in parallel to this action could move lift
      ShouldSuppressTrackLocking(true);
      
      // Drive through the block
      DriveStraightAction* drive = new DriveStraightAction(robot, kDrivingDist_mm, kDrivingSpeed_mmps);
      
      // Lift movement to actually flip
      MoveLiftToHeightAction* moveLift = new MoveLiftToHeightAction(robot, MoveLiftToHeightAction::Preset::CARRY);
      
      // We need to wait a bit until we start moving the lift and flip
      CompoundActionSequential* flip = new CompoundActionSequential(robot, {new WaitAction(robot, kTimeToWaitToFlip_sec), moveLift});
      
      // Need to set the initial lift height to fit lift base into block corner edge
      MoveLiftToHeightAction* initialLift = new MoveLiftToHeightAction(robot, kInitialLiftHeight_mm);
      
      // Drive and flip run in parallel (this is why flip needs to wait a bit to start running
      // Is parallel so lift can be moving while we are driving
      CompoundActionParallel* driveAndFlip = new CompoundActionParallel(robot, {drive, flip});
      
      CompoundActionSequential* action = new CompoundActionSequential(robot, {initialLift, driveAndFlip});
      AddAction(action);
    }
    
  }
}