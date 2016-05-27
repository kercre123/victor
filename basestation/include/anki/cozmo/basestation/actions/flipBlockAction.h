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
      
      // Use DriveToFlipBlockPoseAction's completion info
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        if(_actions.size() > 0) {
          _actions.back()->GetCompletionUnion(completionUnion);
        } else {
          completionUnion = _completedActionInfoStack.front().first;
        }
      }
    };
    
    // Drive to the preDockPose that puts in position to flip a block
    class DriveToFlipBlockPoseAction : public CompoundActionSequential
    {
    public:
      DriveToFlipBlockPoseAction(Robot& robot, ObjectID objectID);
      
      virtual RobotActionType GetType() const override { return RobotActionType::FLIP_BLOCK; }
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
    private:
      ObjectID _blockToFlipID;
    };
    
    // Drives straight and move lift to flip block
    class FlipBlockAction : public CompoundActionSequential
    {
    public:
      FlipBlockAction(Robot& robot, ObjectID objectID);
      
      virtual RobotActionType GetType() const override { return RobotActionType::FLIP_BLOCK; }
    
    private:
      const f32 kDrivingSpeed_mmps = 150;
      const f32 kDrivingDist_mm = 100;
      const f32 kExtraDrivingDist_mm = 20;
      const f32 kTimeToWaitToFlip_sec = 0.7;
      const f32 kInitialLiftHeight_mm = 45;
    };
  }
}