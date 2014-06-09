/**
 * File: latticePlanner.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-06-04
 *
 * Description: Cozmo wrapper for the xytheta lattice planner in
 * coretech/planning
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "anki/common/basestation/math/rotatedRect.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "pathPlanner.h"
#include "anki/planning/basestation/xythetaPlanner.h"
#include "anki/planning/basestation/xythetaEnvironment.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

using namespace Planning;

class LatticePlannerImpl
{
public:

  LatticePlannerImpl(const BlockWorld* blockWorld, const Json::Value& mprims)
    : blockWorld_(blockWorld)
    , planner_(env_)
    {
      env_.Init(mprims);
    }

  const BlockWorld* blockWorld_;
  xythetaEnvironment env_;
  xythetaPlanner planner_;
};

LatticePlanner::LatticePlanner(const BlockWorld* blockWorld, const Json::Value& mprims)
{
  impl_ = new LatticePlannerImpl(blockWorld, mprims);
}

LatticePlanner::~LatticePlanner()
{
  delete impl_;
  impl_ = nullptr;
}
      
Result LatticePlanner::GetPlan(Planning::Path &path, const Pose3d &startPose, const Pose3d &targetPose)
{
  impl_->env_.ClearObstacles();

  assert(impl_->blockWorld_);

  printf("setting up environment...\n");
  unsigned int numAdded = 0;

  for(auto blocksByType : impl_->blockWorld_->GetAllExistingBlocks()) {
    for(auto blocksByID : blocksByType.second) {
          
      const Block* block = dynamic_cast<Block*>(blocksByID.second);

      // TODO:(bn) ask andrew why its like this
      // TODO:(bn) real parameters
      float paddingRadius = 65.0;
      float blockRadius = 44.0;
      float paddingFactor = (paddingRadius + blockRadius) / blockRadius;
      Quad2f quadOnGround2d = block->GetBoundingQuadXY(paddingFactor);

      // TODO:(bn) who frees this??
      RotatedRectangle *boundingRect = new RotatedRectangle;
      boundingRect->ImportQuad(quadOnGround2d);

      impl_->env_.AddObstacle(boundingRect);
      numAdded++;
    }
  }

  printf("Added %u blocks\n", numAdded);

  State_c start(startPose.get_translation().x(),
                    startPose.get_translation().y(),
                    startPose.get_rotationAngle().ToFloat());

  if(!impl_->planner_.SetStart(start))
    return RESULT_FAIL_INVALID_PARAMETER;

  State_c target(targetPose.get_translation().x(),
                    targetPose.get_translation().y(),
                    targetPose.get_rotationAngle().ToFloat());

  if(!impl_->planner_.SetGoal(target))
    return RESULT_FAIL_INVALID_PARAMETER;

  printf("planning from (%f, %f, %f) to (%f %f %f)\n",
             start.x_mm, start.y_mm, start.theta,
             target.x_mm, target.y_mm, target.theta);


  // TODO:(bn) param!
  impl_->planner_.AllowFreeTurnInPlaceAtGoal(false);

  printf("planning....\n");
  if(!impl_->planner_.ComputePath()) {
    return RESULT_FAIL;
  }

  impl_->env_.ConvertToPath(impl_->planner_.GetPlan(), path);

  path.PrintPath();

  // impl_->env_.WriteEnvironment("/Users/bneuman/blockWorld.env");

  // TODO:(bn) return value!
  return RESULT_OK;
}

bool LatticePlanner::ReplanIfNeeded(Planning::Path &path, const Pose3d& startPose) 
{

  // TODO:(bn) don't do this every time! Get an update from BlockWorld
  // if a new block shows up or one moves significantly
  impl_->env_.ClearObstacles();

  assert(impl_->blockWorld_);

  for(auto blocksByType : impl_->blockWorld_->GetAllExistingBlocks()) {
    for(auto blocksByID : blocksByType.second) {
          
      const Block* block = dynamic_cast<Block*>(blocksByID.second);

      // TODO:(bn) ask andrew why its like this
      // TODO:(bn) real parameters
      float paddingRadius = 65.0;
      float blockRadius = 44.0;
      float paddingFactor = (paddingRadius + blockRadius) / blockRadius;
      Quad2f quadOnGround2d = block->GetBoundingQuadXY(paddingFactor);

      // TODO:(bn) who frees this??
      RotatedRectangle *boundingRect = new RotatedRectangle;
      boundingRect->ImportQuad(quadOnGround2d);

      impl_->env_.AddObstacle(boundingRect);
    }
  }

  if(!impl_->planner_.PlanIsSafe()) {
    printf("old plan unsafe! Will replan.\n");

    State_c start(startPose.get_translation().x(),
                  startPose.get_translation().y(),
                  startPose.get_rotationAngle().ToFloat());

    path.Clear();

    if(!impl_->planner_.SetStart(start)) {
      printf("ERROR: ReplanIfNeeded, invalid start!\n");      
    }
    else if(!impl_->planner_.GoalIsValid()) {
      printf("ERROR: ReplanIfNeeded, invalid goal!\n");
    }
    else {
      impl_->planner_.SetReplanFromScratch();

      printf("(re-)planning from (%f, %f, %f)\n",
             start.x_mm, start.y_mm, start.theta);

      if(!impl_->planner_.ComputePath()) {
        printf("plan failed during replanning!\n");
      }
      else {
        impl_->env_.ConvertToPath(impl_->planner_.GetPlan(), path);
        path.PrintPath();
      }
    }

    return true;
  }

  return false;
}

}
}
