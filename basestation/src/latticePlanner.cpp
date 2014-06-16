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

  // imports and pads obstacles
  void ImportBlockworldObstacles(float paddingRadius);

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

void LatticePlannerImpl::ImportBlockworldObstacles(float paddingRadius)
{
  env_.ClearObstacles();

  assert(blockWorld_);

  for(auto blocksByType : blockWorld_->GetAllExistingBlocks()) {
    for(auto blocksByID : blocksByType.second) {
          
      const Block* block = dynamic_cast<Block*>(blocksByID.second);

      // TODO:(bn) real parameters
      float blockRadius = 44.0;
      float paddingFactor = (paddingRadius + blockRadius) / blockRadius;
      Quad2f quadOnGround2d = block->GetBoundingQuadXY(paddingFactor);

      RotatedRectangle *boundingRect = new RotatedRectangle;
      boundingRect->ImportQuad(quadOnGround2d);

      // printf("adding obstacle (%f padding): (%f, %f) width: %f height: %f\n",
      //        paddingRadius,
      //        boundingRect->GetX(),
      //        boundingRect->GetY(),
      //        boundingRect->GetWidth(),
      //        boundingRect->GetHeight());

      env_.AddObstacle(boundingRect);
    }
  }
}

      
Result LatticePlanner::GetPlan(Planning::Path &path, const Pose3d &startPose, const Pose3d &targetPose)
{
  impl_->ImportBlockworldObstacles(65.0);

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

  path.Clear();
  impl_->env_.AppendToPath(impl_->planner_.GetPlan(), path);

  path.PrintPath();

  // impl_->env_.WriteEnvironment("/Users/bneuman/blockWorld.env");

  // TODO:(bn) return value!
  return RESULT_OK;
}

bool LatticePlanner::ReplanIfNeeded(Planning::Path &path, const Pose3d& startPose, const u8 currentPathSegment)
{

  // TODO:(bn) don't do this every time! Get an update from BlockWorld
  // if a new block shows up or one moves significantly

  // TODO:(bn) parameters!

  // pad slightly less than when doing planning so we don't trigger a
  // replan if the blocks move very slightly
  impl_->ImportBlockworldObstacles(60.0);

  State_c lastSafeState;
  xythetaPlan validOldPlan;

  State_c currentRobotState(startPose.get_translation().x(),
                            startPose.get_translation().y(),
                            startPose.get_rotationAngle().ToFloat());
                    

  // plan Idx is the number of plan actions to execute before getting
  // to the starting point closest to start
  size_t planIdx = impl_->planner_.FindClosestPlanSegmentToPose(currentRobotState);

  // TODO:(bn) param
  const float maxDistancetoFollowOldPlan_mm = 60.0;

  if(!impl_->planner_.PlanIsSafe(maxDistancetoFollowOldPlan_mm, planIdx, lastSafeState, validOldPlan)) {
    // at this point, we know the plan isn't completely
    // safe. lastSafeState will be set to the furthest state along the
    // plan (after planIdx) which is safe. validOldPlan will contain a
    // partial plan starting at planIdx and ending at lastSafeState

    printf("old plan unsafe! Will replan, starting from %zu, keeping %zu actions from oldPlan.\n",
           planIdx, validOldPlan.Size());

    if(validOldPlan.Size() == 0) {
      // if we can't safely complete the action we are currently
      // executing, then valid old plan will be empty and we will
      // replan from the current state of the robot
      lastSafeState = currentRobotState;
    }

    path.Clear();

    if(!impl_->planner_.SetStart(lastSafeState)) {
      printf("ERROR: ReplanIfNeeded, invalid start!\n");      
    }
    else if(!impl_->planner_.GoalIsValid()) {
      printf("ReplanIfNeeded, invalid goal! Goal may have moved into collision.\n");
    }
    else {
      impl_->planner_.SetReplanFromScratch();

      // use real padding for re-plab
      impl_->ImportBlockworldObstacles(65.0);

      printf("(re-)planning from (%f, %f, %f)\n",
             lastSafeState.x_mm, lastSafeState.y_mm, lastSafeState.theta);

      if(!impl_->planner_.ComputePath()) {
        printf("plan failed during replanning!\n");
      }
      else {

        assert(impl_->planner_.GetPlan().Size() == 0 ||
               impl_->planner_.GetPlan().start_ == impl_->env_.State_c2State(lastSafeState));

        path.Clear();

        impl_->env_.AppendToPath(validOldPlan, path);
        printf("from old plan:\n");
        path.PrintPath();

        impl_->env_.AppendToPath(impl_->planner_.GetPlan(), path);
        printf("total plan:\n");
        path.PrintPath();
      }
    }

    return true;
  }

  return false;
}

}
}
