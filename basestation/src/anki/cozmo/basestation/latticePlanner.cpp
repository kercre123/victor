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

#include "anki/common/basestation/math/fastPolygon2d.h"
#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/rotatedRect.h"
#include "anki/util/logging/logging.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/planning/basestation/xythetaEnvironment.h"
#include "anki/planning/basestation/xythetaPlanner.h"
#include "json/json.h"
#include "pathPlanner.h"
#include "anki/cozmo/basestation/viz/vizManager.h"


// base padding on the robot footprint
#define LATTICE_PLANNER_ROBOT_PADDING 7.0

// base padding on the obstacles
#define LATTICE_PLANNER_OBSTACLE_PADDING 6.0

// amount of padding to subtract for replan-checks. MUST be less than the above values
#define LATTICE_PLANNER_RPLAN_PADDING_SUBTRACT 5.0

#define DEBUG_REPLAN_CHECKS 0

// a global max so planning doesn't run forever
#define LATTICE_PLANNER_MAX_EXPANSIONS 300000000

// this is in units of seconds per mm, meaning the robot would drive X
// seconds out of the way to avoid having to drive 1 mm through the
// penalty
#define DEFAULT_OBSTACLE_PENALTY 0.1

// how far (in mm) away from the path the robot needs to be before it gives up and plans a new path
#define PLAN_ERROR_FOR_REPLAN 20.0


// The min absolute difference between the commanded end pose angle
// and the end pose angle of the generated plan that is required
// in order for a point turn to be appended to the path such that
// the end pose angle of the path is the commanded one.
const f32 TERMINAL_POINT_TURN_CORRECTION_THRESH_RAD = DEG_TO_RAD_F32(2.f);

const f32 TERMINAL_POINT_TURN_SPEED = 2; //rad/s
const f32 TERMINAL_POINT_TURN_ACCEL = 100.f;
const f32 TERMINAL_POINT_TURN_DECEL = 100.f;


namespace Anki {
namespace Cozmo {

using namespace Planning;

class LatticePlannerImpl
{
public:

  LatticePlannerImpl(const Robot* robot, const Json::Value& mprims, const LatticePlanner* parent)
    : robot_(robot)
    , planner_(env_)
    , _parent(parent)
    {
      env_.Init(mprims);
    }

  // imports and pads obstacles
  void ImportBlockworldObstacles(const bool isReplanning, const ColorRGBA* vizColor = nullptr);

  const Robot* robot_;
  xythetaEnvironment env_;
  xythetaPlanner planner_;
  xythetaPlan totalPlan_;
  Pose3d targetPose_orig_;

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
  float obstaclePadding = LATTICE_PLANNER_OBSTACLE_PADDING;
  if(isReplanning) {
    obstaclePadding -= LATTICE_PLANNER_RPLAN_PADDING_SUBTRACT;
  }

  float robotPadding = LATTICE_PLANNER_ROBOT_PADDING;
  if(isReplanning) {
    robotPadding -= LATTICE_PLANNER_RPLAN_PADDING_SUBTRACT;
  }

  const bool didObjecsChange = robot_->GetBlockWorld().DidObjectsChange();
  
  if(!isReplanning ||  // if not replanning, this must be a fresh, new plan, so we always should get obstacles
     didObjecsChange)
  {
    std::vector<std::pair<Quad2f,ObjectID> > boundingBoxes;

    robot_->GetBlockWorld().GetObstacles(boundingBoxes, obstaclePadding);
    
    env_.ClearObstacles();
    
    if(vizColor != nullptr) {
      VizManager::getInstance()->EraseAllPlannerObstacles(isReplanning);
    }

    unsigned int numAdded = 0;
    unsigned int vizID = 0;

    Planning::StateTheta numAngles = (StateTheta)env_.GetNumAngles();
    for(StateTheta theta=0; theta < numAngles; ++theta) {

      float thetaRads = env_.GetTheta_c(theta);

      // Since the planner is given the driveCenter poses to be the robot's origin,
      // compute the actual robot origin given a driveCenter pose at (0,0,0) in order
      // to get the actual bounding box of the robot.
      Pose3d robotDriveCenterPose = Pose3d(thetaRads, Z_AXIS_3D(), {0.0f, 0.0f, 0.0f});
      Pose3d robotOriginPose;
      robot_->ComputeOriginPose(robotDriveCenterPose, robotOriginPose);
      
      // Get the robot polygon, and inflate it by a bit to handle error
      Poly2f robotPoly;
      robotPoly.ImportQuad2d(robot_->GetBoundingQuadXY(robotOriginPose, robotPadding) );

      for(auto boundingQuad : boundingBoxes) {

        Poly2f boundingPoly;
        boundingPoly.ImportQuad2d(boundingQuad.first);

        /*const FastPolygon& expandedPoly =*/
          env_.AddObstacleWithExpansion(boundingPoly, robotPoly, theta, DEFAULT_OBSTACLE_PENALTY);

        // only draw the angle we are currently at
        if(vizColor != nullptr && env_.GetTheta(robot_->GetPose().GetRotationAngle<'Z'>().ToFloat()) == theta
           // && !isReplanning
          ) {

          // TODO: manage the quadID better so we don't conflict
          // TODO:(bn) handle isReplanning with color??
          // VizManager::getInstance()->DrawPlannerObstacle(isReplanning, vizID++ , expandedPoly, *vizColor);

          // TODO:(bn) figure out a good way to visualize the
          // multi-angle stuff. For now just draw the quads with
          // padding
          VizManager::getInstance()->DrawQuad (
            isReplanning ? VIZ_QUAD_PLANNER_OBSTACLE_REPLAN : VIZ_QUAD_PLANNER_OBSTACLE,
            vizID++, boundingQuad.first, 0.1f, *vizColor );
        }
        numAdded++;
      }

      // if(theta == 0) {
      //   PRINT_NAMED_INFO("LatticePlannerImpl.ImportBlockworldObstacles.ImportedObstacles",
      //                    "imported %d obstacles form blockworld",
      //                    numAdded);
      // }
    }

    // PRINT_NAMED_INFO("LatticePlannerImpl.ImportBlockworldObstacles.ImportedAngles",
    //                  "imported %d total obstacles for %d angles",
    //                  numAdded,
    //                  numAngles);
  }
  else {
    PRINT_NAMED_INFO("LatticePlanner.ImportBlockworldObstacles.NoUpdateNeeded",
                     "robot padding %f, obstacle padding %f , didBlocksChange %d",
                     robotPadding,
                     obstaclePadding,
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
  
  // Save original target pose
  impl_->targetPose_orig_ = targetPose;
  
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
     !impl_->env_.PlanIsSafe(impl_->totalPlan_, maxDistancetoFollowOldPlan_mm, static_cast<int>(planIdx), lastSafeState, validOldPlan)) {
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

      impl_->env_.PrepareForPlanning();

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
    

    // Do final check on how close the plan's goal angle is to the originally requested goal angle
    // and either add a point turn at the end or modify the last point turn action.
    u8 numSegments = path.GetNumSegments();
    Radians desiredGoalAngle = impl_->targetPose_orig_.GetRotationAngle<'Z'>();
    
    if (numSegments > 0) {
      // TODO: Should we expect to see empty paths here? Ask Brad.
    
      // Get last non-point turn segment of path
      PathSegment& lastSeg = path[numSegments-1];

      f32 end_x, end_y, end_angle;
      lastSeg.GetEndPose(end_x, end_y, end_angle);
      
      while(lastSeg.GetType() == Planning::PST_POINT_TURN) {
        path.PopBack(1);
        --numSegments;
        
        if (numSegments > 0) {
          lastSeg = path[numSegments-1];
          lastSeg.GetEndPose(end_x, end_y, end_angle);
        } else {
          break;
        }
      }
      
      Radians plannedGoalAngle(end_angle);
      f32 angDiff = (desiredGoalAngle - plannedGoalAngle).ToFloat();
      
      if (abs(angDiff) > TERMINAL_POINT_TURN_CORRECTION_THRESH_RAD) {
        
        f32 turnDir = angDiff > 0 ? 1.f : -1.f;
        f32 rotSpeed = TERMINAL_POINT_TURN_SPEED * turnDir;
        
        PRINT_NAMED_INFO("LatticePlanner.ReplanIfNeeded.", "LatticePlanner: Final angle off by %f rad. DesiredAng = %f, endAngle = %f, rotSpeed = %f. Adding point turn.", angDiff, desiredGoalAngle.ToFloat(), end_angle, rotSpeed);
        
        path.AppendPointTurn(0, end_x, end_y, desiredGoalAngle.ToFloat(),
                             rotSpeed,
                             TERMINAL_POINT_TURN_ACCEL,
                             TERMINAL_POINT_TURN_DECEL,
                             true);
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
