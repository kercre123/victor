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
#include "anki/cozmo/robot/cozmoConfig.h"
#include "pathPlanner.h"
#include "vizManager.h"
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
  VizManager::getInstance()->EraseAllQuads(); // TODO: only erase bounding box quads

  assert(impl_->blockWorld_);

  printf("setting up environment...\n");
  unsigned int numAdded = 0;

  std::vector<Quad2f> boundingBoxes;
  impl_->blockWorld_->GetBlockBoundingBoxesXY(0.f, ROBOT_BOUNDING_Z,
                                              ROBOT_BOUNDING_RADIUS,
                                              boundingBoxes,
                                              _ignoreTypes, _ignoreIDs);
  
  for(auto boundingQuad : boundingBoxes) {
    
    // TODO: manage the quadID better so we don't conflict
    VizManager::getInstance()->DrawQuad(500+numAdded, boundingQuad, 0.5f, VIZ_COLOR_BLOCK_BOUNDING_QUAD);
    
    impl_->env_.AddObstacle(boundingQuad);
    ++numAdded;
  }

  printf("Added %u blocks\n", numAdded);

  State_c start(startPose.get_translation().x(),
                startPose.get_translation().y(),
                startPose.get_rotationAngle<'Z'>().ToFloat());

  if(!impl_->planner_.SetStart(start))
    return RESULT_FAIL_INVALID_PARAMETER;

  State_c target(targetPose.get_translation().x(),
                    targetPose.get_translation().y(),
                    targetPose.get_rotationAngle<'Z'>().ToFloat());

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
  VizManager::getInstance()->EraseAllQuads(); // TODO: only erase bounding box quads

  assert(impl_->blockWorld_);

  std::vector<Quad2f> boundingBoxes;
  impl_->blockWorld_->GetBlockBoundingBoxesXY(0.f, ROBOT_BOUNDING_Z,
                                              ROBOT_BOUNDING_RADIUS,
                                              boundingBoxes,
                                              _ignoreTypes, _ignoreIDs);
  unsigned int numAdded = 0;
  for(auto boundingQuad : boundingBoxes) {
    
    // TODO: manage the quadID better so we don't conflict
    VizManager::getInstance()->DrawQuad(500 + numAdded++, boundingQuad, 0.5f, VIZ_COLOR_BLOCK_BOUNDING_QUAD);
   
    impl_->env_.AddObstacle(boundingQuad);
  }

  if(!impl_->planner_.PlanIsSafe()) {
    printf("old plan unsafe! Will replan.\n");

    State_c start(startPose.get_translation().x(),
                  startPose.get_translation().y(),
                  startPose.get_rotationAngle<'Z'>().ToFloat());

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
