/**
 * File: flipBlockAction.h
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

#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "clad/types/actionTypes.h"

namespace Anki {
  namespace Cozmo {
    
    // Drives to and flips block
    class DriveAndFlipBlockAction : public IDriveToInteractWithObject
    {
    public:
      DriveAndFlipBlockAction(Robot& robot,
                              const ObjectID objectID,
                              const bool shouldDriveToClosestPreActionPose = false,
                              const bool useApproachAngle = false,
                              const f32 approachAngle_rad = 0,
                              const bool useManualSpeed = false,
                              Radians maxTurnTowardsFaceAngle_rad = 0.f,
                              const bool sayName = false);
      
      virtual RobotActionType GetType() const override { return RobotActionType::FLIP_BLOCK; }
      
      ActionResult GetPossiblePoses(ActionableObject* object,
                                    std::vector<Pose3d>& possiblePoses,
                                    bool& alreadyInPosition);
      
    private:
      IAction* _flipBlockAction = nullptr;
      bool _shouldDriveToClosestPreActionPose = false;
      
    };
    
    
    class DriveToFlipBlockPoseAction : public DriveToObjectAction
    {
    public:
      DriveToFlipBlockPoseAction(Robot& robot, ObjectID objectID);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::FLIP_BLOCK; }
    };
    
    // Drives straight and move lift to flip block
    class FlipBlockAction : public IAction
    {
    public:
      FlipBlockAction(Robot& robot, ObjectID objectID);
      virtual ~FlipBlockAction();
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::FLIP_BLOCK; }
      virtual u8 GetTracksToLock() const override { return (u8)AnimTrackFlag::LIFT_TRACK | (u8)AnimTrackFlag::BODY_TRACK; }
      
    protected:
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
    
    private:
      ObjectID _objectID;
      CompoundActionSequential _compoundAction;
      const f32 kDrivingSpeed_mmps = 150;
      const f32 kDrivingDist_mm = 20;
      const f32 kInitialLiftHeight_mm = 45;
      const f32 kDistToObjectToFlip_mm = 40;
      u32 _flipTag = -1;
    };
  }
}