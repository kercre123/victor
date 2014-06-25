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

  LatticePlannerImpl(const BlockWorld* blockWorld, const Json::Value& mprims, const LatticePlanner* parent)
    : blockWorld_(blockWorld)
    , planner_(env_)
    , lastPaddingRadius_(0.0)
    , _parent(parent)
    {
      env_.Init(mprims);
    }

  // imports and pads obstacles
  void ImportBlockworldObstacles(float paddingRadius, VIZ_COLOR_ID vizColor = VIZ_COLOR_NONE);
  float lastPaddingRadius_;

  const BlockWorld* blockWorld_;
  xythetaEnvironment env_;
  xythetaPlanner planner_;
  xythetaPlan totalPlan_;

  const LatticePlanner* _parent;
};

LatticePlanner::LatticePlanner(const BlockWorld* blockWorld, const Json::Value& mprims)
{
  impl_ = new LatticePlannerImpl(blockWorld, mprims, this);

  // TODO:(bn) param!
  impl_->planner_.AllowFreeTurnInPlaceAtGoal(false);
}

LatticePlanner::~LatticePlanner()
{
  delete impl_;
  impl_ = nullptr;
}

void LatticePlannerImpl::ImportBlockworldObstacles(float paddingRadius, VIZ_COLOR_ID vizColor)
{
  // TEMP: visualization doesn't work because we keep clearing all
  // quads. Once we fix vis and remove the EraseAllQuads() call, get
  // rid of the "true" so this only runs when it needs to
  if(!FLT_NEAR(paddingRadius, lastPaddingRadius_) ||
     blockWorld_->DidBlocksChange())
  {
    lastPaddingRadius_ = paddingRadius;
    std::vector<Quad2f> boundingBoxes;

    // first check plan with slightly smaller radius to see if we need to replan
    blockWorld_->GetBlockBoundingBoxesXY(0.f, ROBOT_BOUNDING_Z,
                                         paddingRadius,
                                         boundingBoxes,
                                         _parent->_ignoreTypes, _parent->_ignoreIDs);
    env_.ClearObstacles();
    
    // TODO: figure out whether we are in replan mode in some other way (pass in flag?)
    const bool isReplan = vizColor == VIZ_COLOR_REPLAN_BLOCK_BOUNDING_QUAD;
    
    if(vizColor != VIZ_COLOR_NONE) {
      VizManager::getInstance()->EraseAllPlannerObstacles(isReplan);
    }
    unsigned int numAdded = 0;
    for(auto boundingQuad : boundingBoxes) {
      env_.AddObstacle(boundingQuad);

      if(vizColor != VIZ_COLOR_NONE) {
        // TODO: manage the quadID better so we don't conflict
        // TODO:(bn) custom color for this
        //VizManager::getInstance()->DrawQuad(300 + ((int)vizColor) * 100 + numAdded++, boundingQuad, 0.5f, vizColor);
        VizManager::getInstance()->DrawPlannerObstacle(isReplan, numAdded++, boundingQuad, 0.5f, vizColor);
        //(300 + ((int)vizColor) * 100 + numAdded++, boundingQuad, 0.5f, vizColor);
      }
    }

  }
}

      
IPathPlanner::EPlanStatus LatticePlanner::GetPlan(Planning::Path &path,
                                                  const Pose3d &startPose,
                                                  const Pose3d &targetPose)
{

  State_c target(targetPose.get_translation().x(),
                    targetPose.get_translation().y(),
                    targetPose.get_rotationAngle<'Z'>().ToFloat());

  impl_->ImportBlockworldObstacles(LATTICE_PLANNER_BOUNDING_DISTANCE,
                                   VIZ_COLOR_NONE);

  // Clear plan whenever we attempt to set goal
  impl_->totalPlan_.Clear();
  
  if(!impl_->planner_.SetGoal(target))
    return PLAN_NEEDED_BUT_GOAL_FAILURE;

  // impl_->env_.WriteEnvironment("/Users/bneuman/blockWorld.env");

  return GetPlan(path, startPose, true);
}

IPathPlanner::EPlanStatus LatticePlanner::GetPlan(Planning::Path &path,
                                                  const Pose3d& startPose,
                                                  const std::vector<Pose3d>& targetPoses,
                                                  size_t& selectedIndex)
{
  // for now just select the closest non-colliding goal as the true
  // goal. Eventually I'll implement a real multi-goal planner that
  // decides on its own which goal it wants

  size_t bestTargetIdx = 0;
  bool found = false;
  size_t numTargetPoses = targetPoses.size();
  float closestDist2 = 0;

  for(size_t i=0; i<numTargetPoses; ++i) {
    float dist2 = (targetPoses[i].get_translation() - startPose.get_translation()).LengthSq();

    if(!found || dist2 < closestDist2) {
      State_c target(targetPoses[i].get_translation().x(),
                     targetPoses[i].get_translation().y(),
                     targetPoses[i].get_rotationAngle<'Z'>().ToFloat());

      if(!impl_->env_.IsInCollision(target)) {
        closestDist2 = dist2;
        bestTargetIdx = i;
        found = true;
      }
    }
  }

  if(found) {
    selectedIndex = bestTargetIdx;
    return GetPlan(path, startPose, targetPoses[bestTargetIdx]);
  }
  else {
    printf("LatticePlanner::GetPlan: could not find valid target out of %lu possible targets\n",
           numTargetPoses);
    return PLAN_NEEDED_BUT_GOAL_FAILURE;
  }
}


IPathPlanner::EPlanStatus LatticePlanner::GetPlan(Planning::Path &path,
                                                  const Pose3d& startPose,
                                                  bool forceReplanFromScratch)
{
  using namespace std;

  State_c lastSafeState;
  xythetaPlan validOldPlan;
  size_t planIdx = 0;

  State_c currentRobotState(startPose.get_translation().x(),
                            startPose.get_translation().y(),
                            startPose.get_rotationAngle<'Z'>().ToFloat());

  //VizManager::getInstance()->EraseAllQuads();
  
  if(!forceReplanFromScratch) {
    impl_->ImportBlockworldObstacles(LATTICE_PLANNER_BOUNDING_DISTANCE_REPLAN_CHECK,
                                     VIZ_COLOR_REPLAN_BLOCK_BOUNDING_QUAD);

    // plan Idx is the number of plan actions to execute before getting
    // to the starting point closest to start
    planIdx = impl_->env_.FindClosestPlanSegmentToPose(impl_->totalPlan_, currentRobotState);
  }
  else {
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
      impl_->ImportBlockworldObstacles(LATTICE_PLANNER_BOUNDING_DISTANCE,
                                       VIZ_COLOR_BLOCK_BOUNDING_QUAD);

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
