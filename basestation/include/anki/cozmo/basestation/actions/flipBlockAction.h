/**
 * File: flipBlockAction.h
 *
 * Author: Al Chaussee
 * Date:   5/18/16
 *
 * Description: Action which flips blocks
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
    class DriveAndFlipBlockAction : public CompoundActionSequential
    {
    public:
      DriveAndFlipBlockAction(Robot& robot, ObjectID objectID);
      
      void SetMotionProfile(const PathMotionProfile& motionProfile);
      
      virtual RobotActionType GetType() const override { return RobotActionType::FLIP_BLOCK; }
      
    };
    
    // Drive to the preDockPose that puts in position to flip a block
    class DriveToFlipBlockPoseAction : public CompoundActionSequential
    {
    public:
      DriveToFlipBlockPoseAction(Robot& robot, ObjectID objectID);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::FLIP_BLOCK; }
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
    private:
      ObjectID _blockToFlipID;
    };
    
    // Drives straight and move lift to flip block
    class FlipBlockAction : public IAction
    {
    public:
      FlipBlockAction(Robot& robot, ObjectID objectID);
      virtual ~FlipBlockAction();
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::FLIP_BLOCK; }
      
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
      const f32 kPreDockPoseAngleTolerance = DEG_TO_RAD_F32(5);
      u32 _flipTag = -1;
    };
  }
}