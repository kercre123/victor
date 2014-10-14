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

#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/rotatedRect.h"
#include "anki/common/basestation/utils/logging/logging.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/planning/basestation/xythetaEnvironment.h"
#include "anki/planning/basestation/xythetaPlanner.h"
#include "json/json.h"
#include "pathPlanner.h"
#include "vizManager.h"


#define LATTICE_PLANNER_BOUNDING_DISTANCE_REPLAN_CHECK ROBOT_BOUNDING_RADIUS
#define LATTICE_PLANNER_BOUNDING_DISTANCE ROBOT_BOUNDING_RADIUS + 5.0

#define DEBUG_REPLAN_CHECKS 0

#define LATTICE_PLANNER_MAX_EXPANSIONS 300000000  // TEMP: 

// this is in units of seconds per mm, meaning the robot would drive X
// seconds out of the way to avoid having to drive 1 mm through the
// penalty
#define DEFAULT_OBSTACLE_PENALTY 0.1

// how far (in mm) away from the path the robot needs to be before it gives up and plans a new path
#define PLAN_ERROR_FOR_REPLAN 20.0

namespace Anki {
namespace Cozmo {

using namespace Planning;

class LatticePlannerImpl
{
public:

  LatticePlannerImpl(const Robot* robot, const Json::Value& mprims, const LatticePlanner* parent)
    : lastPaddingRadius_(0.0)
    , robot_(robot)
    , planner_(env_)
    , _parent(parent)
    {
      env_.Init(mprims);
    }

  // imports and pads obstacles
  void ImportBlockworldObstacles(const bool isReplanning, const ColorRGBA* vizColor = nullptr);
  float lastPaddingRadius_;

  const Robot* robot_;
  xythetaEnvironment env_;
  xythetaPlanner planner_;
  xythetaPlan totalPlan_;

  const LatticePlanner* _parent;
};

LatticePlanner::LatticePlanner(const Robot* robot, const Json::Value& mprims)
{
  impl_ = new LatticePlannerImpl(robot, mprims, this);

  // TODO:(bn) param!
  impl_->planner_.AllowFreeTurnInPlaceAtGoal(false);
}

LatticePlanner::~LatticePlanner()
{
  delete impl_;
  impl_ = nullptr;
}

void LatticePlannerImpl::ImportBlockworldObstacles(const bool isReplanning, const ColorRGBA* vizColor)
{
  const float paddingRadius = (isReplanning ? LATTICE_PLANNER_BOUNDING_DISTANCE_REPLAN_CHECK :
                               LATTICE_PLANNER_BOUNDING_DISTANCE);

  // TEMP: visualization doesn't work because we keep clearing all
  // quads. Once we fix vis and remove the EraseAllQuads() call, get
  // rid of the "true" so this only runs when it needs to
  const bool didObjecsChange = robot_->GetBlockWorld().DidObjectsChange();
  
  if(!isReplanning ||  // if not replanning, this must be a fresh, new plan, so we always should get obstacles
     !FLT_NEAR(paddingRadius, lastPaddingRadius_) ||
     didObjecsChange)
  {
    lastPaddingRadius_ = paddingRadius;
    std::vector<Quad2f> boundingBoxes;

    // first check plan with slightly smaller radius to see if we need to replan
    // TODO:(bn) remove old padding radius!!!
    robot_->GetBlockWorld().GetObstacles(boundingBoxes, 0.0f);
    
    env_.ClearObstacles();
    
    if(vizColor != nullptr) {
      VizManager::getInstance()->EraseAllPlannerObstacles(isReplanning);
    }

    // TODO:(bn) config? use old paddingRadius one once it's converted  // TEMP: 
    const float padding = 5.0;

    unsigned int numAdded = 0;

    Planning::StateTheta numAngles = (StateTheta)env_.GetNumAngles();
    for(StateTheta theta=0; theta<numAngles; ++theta) {
      float thetaRads = env_.GetTheta_c(theta);

      Poly2f robotPoly( robot_->GetBoundingQuadXY( Pose3d{thetaRads, Z_AXIS_3D, {0.0f, 0.0f, 0.0f}}, padding ) );

      for(auto boundingQuad : boundingBoxes) {

        Poly2f boundingPoly(boundingQuad);

        const Poly2f& expandedPoly =
          env_.AddObstacleWithExpansion(boundingPoly, robotPoly, theta, DEFAULT_OBSTACLE_PENALTY);

        if(vizColor != nullptr) {
          // TODO: manage the quadID better so we don't conflict
          // TODO:(bn) handle isReplanning with color??
          VizManager::getInstance()->DrawPlannerObstacle(isReplanning, numAdded++, expandedPoly, *vizColor);
        }
        else {
          numAdded++;
        }
      }

      if(theta == 0) {
        PRINT_NAMED_INFO("LatticePlannerImpl.ImportBlockworldObstacles.ImportedObstacles",
                         "imported %d obstacles form blockworld",
                         numAdded);
      }
    }

    PRINT_NAMED_INFO("LatticePlannerImpl.ImportBlockworldObstacles.ImportedAngles",
                     "imported %d total obstacles for %d angles",
                     numAdded,
                     numAngles);
  }
  else {
    PRINT_NAMED_INFO("LatticePlanner.ImportBlockworldObstacles.NoUpdateNeeded",
                     "radius %f (last = %f), didBlocksChange %d",
                     paddingRadius,
                     lastPaddingRadius_,
                     didObjecsChange);
  }
}

      
IPathPlanner::EPlanStatus LatticePlanner::GetPlan(Planning::Path &path,
                                                  const Pose3d &startPose,
                                                  const Pose3d &targetPose)
{
  /*
  const f32 Z_HEIGHT_DIFF_TOLERANCE = ROBOT_BOUNDING_Z * .2f;
  if(!NEAR(startPose.GetTranslation().z(), targetPose.GetTranslation().z(),  Z_HEIGHT_DIFF_TOLERANCE)) {
    PRINT_NAMED_ERROR("LatticePlannerImpl.GetPlan.DifferentHeights",
                      "Can't producea  plan for start and target on different levels (%f vs. %f)\n",
                      startPose.GetTranslation().z(), targetPose.GetTranslation().z());
    return PLAN_NEEDED_BUT_PLAN_FAILURE;
  }
   */
  
  State_c target(targetPose.GetTranslation().x(),
                    targetPose.GetTranslation().y(),
                    targetPose.GetRotationAngle<'Z'>().ToFloat());

  impl_->ImportBlockworldObstacles(false);

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

  impl_->ImportBlockworldObstacles(false, &NamedColors::BLOCK_BOUNDING_QUAD);

  // for now just select the closest non-colliding goal as the true
  // goal. Eventually I'll implement a real multi-goal planner that
  // decides on its own which goal it wants

  bool found = false;
  size_t numTargetPoses = targetPoses.size();
  size_t bestTargetIdx = 0;
  float closestDist2 = 0;

  // first try to find a pose with no soft collisions, then try to find one with no fatal collisions
  for(auto maxPenalty : (float[]){0.01, MAX_OBSTACLE_COST}) {

    bestTargetIdx = 0;
    closestDist2 = 0;
    for(size_t i=0; i<numTargetPoses; ++i) {
      float dist2 = (targetPoses[i].GetTranslation() - startPose.GetTranslation()).LengthSq();

      if(!found || dist2 < closestDist2) {
        State_c target_c(targetPoses[i].GetTranslation().x(),
                         targetPoses[i].GetTranslation().y(),
                         targetPoses[i].GetRotationAngle<'Z'>().ToFloat());
        State target(impl_->env_.State_c2State(target_c));
        
        if(impl_->env_.GetCollisionPenalty(target) < maxPenalty) {
          closestDist2 = dist2;
          bestTargetIdx = i;
          found = true;
        }
      }
    }

    if(found)
      break;
  }

  if(found) {
    selectedIndex = bestTargetIdx;
    return GetPlan(path, startPose, targetPoses[bestTargetIdx]);
  }
  else {
    PRINT_NAMED_INFO("LatticePlanner.GetPlan.NoValidTarget", "could not find valid target out of %lu possible targets\n",
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

  State_c currentRobotState(startPose.GetTranslation().x(),
                            startPose.GetTranslation().y(),
                            startPose.GetRotationAngle<'Z'>().ToFloat());

  //VizManager::getInstance()->EraseAllQuads();
  
  if(!forceReplanFromScratch) {
    impl_->ImportBlockworldObstacles(true, &NamedColors::REPLAN_BLOCK_BOUNDING_QUAD);

    // plan Idx is the number of plan actions to execute before getting
    // to the starting point closest to start
    float offsetFromPlan = 0.0;
    planIdx = impl_->env_.FindClosestPlanSegmentToPose(impl_->totalPlan_, currentRobotState, offsetFromPlan);

    if(offsetFromPlan >= PLAN_ERROR_FOR_REPLAN) {
      PRINT_NAMED_INFO("LatticePlanner.GetPlanFailed",
                       "Current state is %f away from the plan, failing\n",
                       offsetFromPlan);
      impl_->totalPlan_.Clear();
      return PLAN_NEEDED_BUT_PLAN_FAILURE;
    }
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

    if(!forceReplanFromScratch) {
      PRINT_NAMED_INFO("LatticePlanner.GetPlan.OldPlanUnsafe",
                       "old plan unsafe! Will replan, starting from %zu, keeping %zu actions from oldPlan.\n",
                       planIdx, validOldPlan.Size());
    }

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
      PRINT_NAMED_INFO("LatticePlanner.ReplanIfNeeded.InvalidStart", "could not set start\n");
      return PLAN_NEEDED_BUT_START_FAILURE;
    }
    else if(!impl_->planner_.GoalIsValid()) {
      PRINT_NAMED_INFO("LatticePlanner.ReplanIfNeeded.InvalidGoal", "Goal may have moved into collision.\n");
      return PLAN_NEEDED_BUT_GOAL_FAILURE;
    }
    else {
      if(forceReplanFromScratch) {
        impl_->planner_.SetReplanFromScratch();
      }

      // use real padding for re-plan
      impl_->ImportBlockworldObstacles(false, &NamedColors::BLOCK_BOUNDING_QUAD);

      PRINT_NAMED_INFO("LatticePlanner.ReplanIfNeeded.Replanning",
                       "from (%f, %f, %f) to (%f %f %f)\n",
                       lastSafeState.x_mm, lastSafeState.y_mm, lastSafeState.theta,
                       impl_->planner_.GetGoal().x_mm, impl_->planner_.GetGoal().y_mm, impl_->planner_.GetGoal().theta);

      if(!impl_->planner_.Replan(LATTICE_PLANNER_MAX_EXPANSIONS)) {
        PRINT_NAMED_WARNING("LatticePlanner.ReplanIfNeeded.PlannerFailed", "plan failed during replanning!\n");
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

void LatticePlanner::GetTestPath(const Pose3d& startPose, Planning::Path &path)
{
  State_c currentRobotState(startPose.GetTranslation().x(),
                            startPose.GetTranslation().y(),
                            startPose.GetRotationAngle<'Z'>().ToFloat());

  impl_->planner_.SetStart(currentRobotState);

  xythetaPlan plan;
  impl_->planner_.GetTestPlan(plan);
  printf("test plan:\n");
  impl_->env_.PrintPlan(plan);

  path.Clear();
  impl_->env_.AppendToPath(plan, path);
  printf("test path:\n");
  path.PrintPath();
}

}
}
