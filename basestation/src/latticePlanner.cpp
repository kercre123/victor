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


#define LATTICE_PLANNER_BOUNDING_DISTANCE_REPLAN_CHECK ROBOT_BOUNDING_RADIUS
#define LATTICE_PLANNER_BOUNDING_DISTANCE ROBOT_BOUNDING_RADIUS + 5.0

#define DEBUG_REPLAN_CHECKS 0

#define LATTICE_PLANNER_MAX_EXPANSIONS 300000

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
  xythetaPlan totalPlan_;
};

LatticePlanner::LatticePlanner(const BlockWorld* blockWorld, const Json::Value& mprims)
{
  impl_ = new LatticePlannerImpl(blockWorld, mprims);

  // TODO:(bn) param!
  impl_->planner_.AllowFreeTurnInPlaceAtGoal(false);
}

LatticePlanner::~LatticePlanner()
{
  delete impl_;
  impl_ = nullptr;
}
      
IPathPlanner::EPlanStatus LatticePlanner::GetPlan(Planning::Path &path,
                                                  const Pose3d &startPose,
                                                  const Pose3d &targetPose)
{

  State_c target(targetPose.get_translation().x(),
                    targetPose.get_translation().y(),
                    targetPose.get_rotationAngle<'Z'>().ToFloat());

  if(!impl_->planner_.SetGoal(target))
    return PLAN_NEEDED_BUT_GOAL_FAILURE;

  // impl_->env_.WriteEnvironment("/Users/bneuman/blockWorld.env");

  return GetPlan(path, startPose, true);
}

LatticePlanner::EPlanStatus LatticePlanner::GetPlan(Planning::Path &path,
                                                    const Pose3d& startPose,
                                                    bool forceReplanFromScratch)
{
  using namespace std;

  State_c lastSafeState;
  xythetaPlan validOldPlan;
  size_t planIdx = 0;

  State_c currentRobotState(startPose.get_translation().x(),
                            startPose.get_translation().y(),
                            startPose.get_rotationAngle().ToFloat());

  VizManager::getInstance()->EraseAllQuads();
  
  std::vector<Quad2f> boundingBoxes;

  if(!forceReplanFromScratch) {
    // first check plan with slightly smaller radius to see if we need to replan
    impl_->blockWorld_->GetBlockBoundingBoxesXY(0.f, ROBOT_BOUNDING_Z,
                                                LATTICE_PLANNER_BOUNDING_DISTANCE_REPLAN_CHECK,
                                                boundingBoxes,
                                                _ignoreTypes, _ignoreIDs);
    impl_->env_.ClearObstacles();
    unsigned int numAdded = 0;
    for(auto boundingQuad : boundingBoxes) {
      impl_->env_.AddObstacle(boundingQuad);

      // TODO: manage the quadID better so we don't conflict
      // TODO:(bn) custom color for this
      VizManager::getInstance()->DrawQuad(700 + numAdded++, boundingQuad, 0.5f, VIZ_COLOR_REPLAN_BLOCK_BOUNDING_QUAD);
    }

    // plan Idx is the number of plan actions to execute before getting
    // to the starting point closest to start
    planIdx = impl_->env_.FindClosestPlanSegmentToPose(impl_->totalPlan_, currentRobotState);
  }
  else {
    impl_->env_.ClearObstacles();
    impl_->totalPlan_.Clear();
  }

  // TODO:(bn) param
  const float maxDistancetoFollowOldPlan_mm = 40.0;

  if(forceReplanFromScratch ||
     !impl_->env_.PlanIsSafe(impl_->totalPlan_, maxDistancetoFollowOldPlan_mm, planIdx, lastSafeState, validOldPlan)) {
    // at this point, we know the plan isn't completely
    // safe. lastSafeState will be set to the furthest state along the
    // plan (after planIdx) which is safe. validOldPlan will contain a
    // partial plan starting at planIdx and ending at lastSafeState

    printf("old plan unsafe! Will replan, starting from %zu, keeping %zu actions from oldPlan.\n",
           planIdx, validOldPlan.Size());
    cout<<"currentRobotState: "<<currentRobotState<<endl;

    // uncomment to print debugging info
    // impl_->env_.FindClosestPlanSegmentToPose(impl_->totalPlan_, currentRobotState, true);

    if(validOldPlan.Size() == 0) {
      // if we can't safely complete the action we are currently
      // executing, then valid old plan will be empty and we will
      // replan from the current state of the robot
      lastSafeState = currentRobotState;
    }

    impl_->totalPlan_ = validOldPlan;

    path.Clear();

    if(!impl_->planner_.SetStart(lastSafeState)) {
      printf("ERROR: ReplanIfNeeded, invalid start!\n");
      return PLAN_NEEDED_BUT_START_FAILURE;
    }
    else if(!impl_->planner_.GoalIsValid()) {
      printf("ReplanIfNeeded, invalid goal! Goal may have moved into collision.\n");
      return PLAN_NEEDED_BUT_GOAL_FAILURE;
    }
    else {
      if(forceReplanFromScratch) {
        impl_->planner_.SetReplanFromScratch();
      }

      // use real padding for re-plan
      boundingBoxes.clear();
      impl_->blockWorld_->GetBlockBoundingBoxesXY(0.f, ROBOT_BOUNDING_Z,
                                                  LATTICE_PLANNER_BOUNDING_DISTANCE,
                                                  boundingBoxes,
                                                  _ignoreTypes, _ignoreIDs);
      unsigned int numAdded = 0;
      impl_->env_.ClearObstacles();
      for(auto boundingQuad : boundingBoxes) {

        // TODO: manage the quadID better so we don't conflict
        VizManager::getInstance()->DrawQuad(500 + numAdded++, boundingQuad, 0.5f, VIZ_COLOR_BLOCK_BOUNDING_QUAD);

        impl_->env_.AddObstacle(boundingQuad);
      }

      printf("(re)-planning from (%f, %f, %f) to (%f %f %f)\n",
             lastSafeState.x_mm, lastSafeState.y_mm, lastSafeState.theta,
             impl_->planner_.GetGoal().x_mm, impl_->planner_.GetGoal().y_mm, impl_->planner_.GetGoal().theta);

      if(!impl_->planner_.Replan(LATTICE_PLANNER_MAX_EXPANSIONS)) {
        printf("plan failed during replanning!\n");
        return PLAN_NEEDED_BUT_PLAN_FAILURE; 
      }
      else {
        if(impl_->planner_.GetPlan().Size() == 0) {
          impl_->totalPlan_.Clear();
        }
        else {

          assert(impl_->planner_.GetPlan().start_ == impl_->env_.State_c2State(lastSafeState));

          path.Clear();

          // TODO:(bn) hide in #if DEBUG or something like that
          // verify that the append will be correct
          if(impl_->totalPlan_.Size() > 0) {
            State endState = impl_->env_.GetPlanFinalState(impl_->totalPlan_);
            if(endState != impl_->planner_.GetPlan().start_) {
              using namespace std;
              cout<<"ERROR: (LatticePlanner::ReplanIfNeeded) trying to append a plan with a mismatching state!\n";
              cout<<"endState = "<<endState<<endl;
              cout<<"next plan start = "<<impl_->planner_.GetPlan().start_<<endl;

              cout<<"\ntotalPlan_:\n";
              impl_->env_.PrintPlan(impl_->totalPlan_);

              cout<<"\nnew plan:\n";
              impl_->env_.PrintPlan(impl_->planner_.GetPlan());

              assert(false);
            }
          }

          impl_->totalPlan_.Append(impl_->planner_.GetPlan());

          printf("old plan:\n");
          impl_->env_.PrintPlan(validOldPlan);

          printf("new plan:\n");
          impl_->env_.PrintPlan(impl_->planner_.GetPlan());

          impl_->env_.AppendToPath(impl_->totalPlan_, path);
          printf("total path:\n");
          path.PrintPath();
        }
      }
    }

    return DID_PLAN;
  }
  else {
#if DEBUG_REPLAN_CHECKS
    using namespace std;
    if(validOldPlan.Size() > 0) {
      cout<<"LatticePlanner: safely checked plan starting at action "<<planIdx<<" from "<<validOldPlan.start_<<" to "
          <<impl_->env_.GetPlanFinalState(validOldPlan)<< " goal = "<<impl_->planner_.GetGoal()
          <<" ("<<validOldPlan.Size()
          <<" valid actions, totalPlan_.Size = "<<impl_->totalPlan_.Size()<<")\n";
      cout<<"There are "<<impl_->env_.GetNumObstacles()<<" obstacles\n";
    }
    else {
      printf("LatticePlanner: Plan safe, but validOldPlan is empty\n");
    }
#endif
  }

  return PLAN_NOT_NEEDED;
}

}
}
