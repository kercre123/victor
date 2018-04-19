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

#include "engine/namedColors/namedColors.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "coretech/planning/engine/xythetaEnvironment.h"
#include "coretech/planning/engine/xythetaPlanner.h"
#include "coretech/planning/engine/xythetaPlannerContext.h"

#include "latticePlanner.h"
#include "latticePlannerInternal.h"

// whether this planner should consider multiple goals
// todo: probably have this decided in the calling functions based on the world state/goal types/etc
#define LATTICE_PLANNER_MULTIPLE_GOALS 1

namespace Anki {
namespace Cozmo {

LatticePlanner::LatticePlanner(Robot* robot, Util::Data::DataPlatform* dataPlatform)
  : IPathPlanner("LatticePlanner")
{
  _impl = new LatticePlannerInternal(robot, dataPlatform, this);
}


LatticePlanner::~LatticePlanner()
{
  Util::SafeDelete(_impl);
}


EPlannerStatus LatticePlanner::CheckPlanningStatus() const
{
  return _impl->CheckPlanningStatus();
}

void LatticePlanner::StopPlanning()
{
  _impl->StopPlanning();
}

EComputePathStatus LatticePlanner::ComputePathHelper(const Pose3d& startPose,
                                                     const std::vector<Pose3d>& targetPoses)
{
  // This has to live in LatticePlanner instead of impl because it calls LatticePlanner::GetPlan at the end

  if( ! _impl->_contextMutex.try_lock() ) {

    // if the thread is actually computing, than thats an error (trying to compute while we are already
    // computing). But, it might just be the case that someone else has the lock for a bit, in which case we
    // should do a blocking lock. Note that there is no guarantee here that we don't block for an entire plan,
    // we just try to avoid it, if possible

    if( _impl->_timeToPlan || _impl->_plannerRunning ) {

      // thread is already running.
      PRINT_NAMED_WARNING("LatticePlanner.ComputePathHelper.AlreadyRunning",
                          "Tried to compute a new path, but the planner is already running! (timeToPlan %d, running %d)",
                          _impl->_timeToPlan,
                          _impl->_plannerRunning);
      return EComputePathStatus::Error;
    }
    else {
      if( LATTICE_PLANNER_THREAD_DEBUG ) {
        PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "ComputePathHelper: try lock failed, blocking");
      }

      _impl->_contextMutex.lock();
    }
  }

  std::lock_guard<std::recursive_mutex> lg(_impl->_contextMutex, std::adopt_lock);

  if( _impl->_timeToPlan ) {
    PRINT_NAMED_WARNING("LatticePlanner.ComputePathHelper.PreviousPlanRequested",
                        "timeToPlan already set, so another compute path call is already waiting to signal the planner");
    return EComputePathStatus::Error;
  }    

  
  if( LATTICE_PLANNER_THREAD_DEBUG ) {
    PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "ComputePathHelper: got lock");
  }


  /*
  const f32 Z_HEIGHT_DIFF_TOLERANCE = ROBOT_BOUNDING_Z * .2f;
  if(!NEAR(startPose.GetTranslation().z(), targetPose.GetTranslation().z(),  Z_HEIGHT_DIFF_TOLERANCE)) {
    PRINT_NAMED_ERROR("LatticePlannerInternal.GetPlan.DifferentHeights",
                      "Can't producea  plan for start and target on different levels (%f vs. %f)\n",
                      startPose.GetTranslation().z(), targetPose.GetTranslation().z());
    return PLAN_NEEDED_BUT_PLAN_FAILURE;
  }
   */
  
  // Save original target pose
  _impl->_targetPoses_orig = targetPoses;
  
  _impl->ImportBlockworldObstaclesIfNeeded(false);

  // Clear plan whenever we attempt to set goal
  _impl->_totalPlan.Clear();

  _impl->_context.goals_c.clear();
  _impl->_context.goals_c.reserve(targetPoses.size());
  
  for(GoalID goalID = 0; goalID<targetPoses.size(); ++goalID) {
    std::pair<GoalID, State_c> goal_cPair(std::piecewise_construct,
                                          std::forward_as_tuple(goalID),
                                          std::forward_as_tuple(targetPoses[goalID].GetTranslation().x(),
                                                                targetPoses[goalID].GetTranslation().y(),
                                                                targetPoses[goalID].GetRotationAngle<'Z'>().ToFloat()));
    if (_impl->_planner.GoalIsValid(goal_cPair)) {
      _impl->_context.goals_c.emplace_back(std::move(goal_cPair));
    }
  }
  

  if( _impl->_context.goals_c.empty() ) {

    if( LATTICE_PLANNER_THREAD_DEBUG ) {
      PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug",
                    "ComputePathHelper: no valid goals, returning (release lock)");
    }

    return EComputePathStatus::Error;
  }

  EComputePathStatus res = ComputeNewPathIfNeeded(startPose, true);

  if( LATTICE_PLANNER_THREAD_DEBUG ) {
    PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug",
                  "ComputePathHelper: release lock");
  }

  return res;
}

EComputePathStatus LatticePlanner::ComputePath(const Pose3d& startPose,
                                               const Pose3d& targetPose)
{
  std::vector<Pose3d> targetPoses = {targetPose};
  EComputePathStatus res = ComputePathHelper(startPose, targetPoses);
  return res;
}

EComputePathStatus LatticePlanner::ComputePath(const Pose3d& startPose,
                                               const std::vector<Pose3d>& targetPoses)
{
  // This has to live in LatticePlanner instead of impl because it calls LatticePlanner::GetPlan internally

  if( ! _impl->_contextMutex.try_lock() ) {

    // if the thread is actually computing, than thats an error (trying to compute while we are already
    // computing). But, it might just be the case that someone else has the lock for a bit, in which case we
    // should do a blocking lock. Note that there is no guarantee here that we don't block for an entire plan,
    // we just try to avoid it, if possible

    if( _impl->_timeToPlan || _impl->_plannerRunning ) {

      // thread is already running.
      PRINT_NAMED_WARNING("LatticePlanner.ComputePath.AlreadyRunning",
                          "Tried to compute a new path, but the planner is already running! (timeToPlan %d, running %d)",
                          _impl->_timeToPlan,
                          _impl->_plannerRunning);
      return EComputePathStatus::Error;
    }
    else {
      _impl->_contextMutex.lock();
    }
  }

  std::lock_guard<std::recursive_mutex> lg( _impl->_contextMutex, std::adopt_lock);

  if( _impl->_timeToPlan ) {
    PRINT_NAMED_WARNING("LatticePlanner.ComputePath.PreviousPlanRequested",
                        "timeToPlan already set, so another compute path call is already waiting to signal the planner");
    return EComputePathStatus::Error;
  }    

  _impl->ImportBlockworldObstaclesIfNeeded(false, &NamedColors::BLOCK_BOUNDING_QUAD);

  // either search among all goals or just one
  
  if( LATTICE_PLANNER_MULTIPLE_GOALS ) {
  
    // ComputePathHelper will only copy a goal from targetPoses to the LatticePlannerInternal context
    // if it's not in collision (wrt MAX_OBSTACLE_COST), so just pass all targetPoses to ComputePathHelper
    EComputePathStatus res = ComputePathHelper(startPose, targetPoses);
    return res;
    
  } else {
  
    // Select the closest goal without a soft penalty. If that doesnt exist, select the closest non-colliding goal

    size_t numTargetPoses = targetPoses.size();
    GoalID bestTargetID = 0;
    float closestDist2 = 0;
    bool found = false;
    
    for(auto maxPenalty : (float[]){0.01f, MAX_OBSTACLE_COST}) {
      
      bestTargetID = 0;
      closestDist2 = 0;
      for(GoalID i=0; i<numTargetPoses; ++i) {
        float dist2 = (targetPoses[i].GetTranslation() - startPose.GetTranslation()).LengthSq();
        
        if(!found || dist2 < closestDist2) {
          State_c target_c(targetPoses[i].GetTranslation().x(),
                           targetPoses[i].GetTranslation().y(),
                           targetPoses[i].GetRotationAngle<'Z'>().ToFloat());
          GraphState target(target_c);
          
          if(_impl->_context.env.GetCollisionPenalty(target) < maxPenalty) {
            closestDist2 = dist2;
            bestTargetID = i;
            found = true;
          }
        }
      }
      
      if(found) {
        // defer to the single-goal version of this method using the closest non-colliding goal
        EComputePathStatus res = ComputePath(startPose, targetPoses[bestTargetID]);
        _selectedTargetIdx = bestTargetID;
        return res;
      }
    }
    
    PRINT_CH_INFO("Planner", "LatticePlanner.ComputePath.NoValidTarget",
                  "could not find valid target out of %lu possible targets",
                  (unsigned long)numTargetPoses);
    return EComputePathStatus::Error;
    
  }
}

EComputePathStatus LatticePlanner::ComputeNewPathIfNeeded(const Pose3d& startPose,
                                                          bool forceReplanFromScratch,
                                                          bool allowGoalChange)
{
  if( !  _impl->_contextMutex.try_lock() ) {
    // thread is already running, in this case just say no plan needed
    return EComputePathStatus::NoPlanNeeded;
  }

  std::lock_guard<std::recursive_mutex> lg( _impl->_contextMutex, std::adopt_lock);
  
  // change planner goals if needed
  if ( !allowGoalChange && (_impl->_context.goals_c.size() > 1) ) {
    // assuming move-clear-emplace is faster than than removing individual elements, but this has not been benchmarked
    std::pair<GoalID, State_c> goal_cPair = std::move(_impl->_context.goals_c[_impl->_selectedGoalID]);
    _impl->_context.goals_c.clear();
    _impl->_context.goals_c.emplace_back(std::move(goal_cPair));
  } else if ( allowGoalChange && (_impl->_context.goals_c.size() != _impl->_targetPoses_orig.size()) ) {
    // only check if the sizes are different. This does assume that the only method to change the goals is 
    // through ComputePath, which should invalidate paths, and properly sync context's and impl's goals
    _impl->_context.goals_c.clear();
    _impl->_context.goals_c.reserve(_impl->_targetPoses_orig.size());
    
    for(GoalID goalID = 0; goalID < _impl->_targetPoses_orig.size(); ++goalID) {
      std::pair<GoalID, State_c> goal_cPair(std::piecewise_construct,
                                            std::forward_as_tuple(goalID),
                                            std::forward_as_tuple(_impl->_targetPoses_orig[goalID].GetTranslation().x(),
                                                                  _impl->_targetPoses_orig[goalID].GetTranslation().y(),
                                                                  _impl->_targetPoses_orig[goalID].GetRotationAngle<'Z'>().ToFloat()));
      if (_impl->_planner.GoalIsValid(goal_cPair)) {
        _impl->_context.goals_c.emplace_back(std::move(goal_cPair));
      }
    }
  }
  
  EComputePathStatus ret = _impl->StartPlanning(startPose, forceReplanFromScratch);

  return ret;
}

bool LatticePlanner::PreloadObstacles()
{
  _impl->ImportBlockworldObstaclesIfNeeded(false);
  return true;
}
  
bool LatticePlanner::CheckIsPathSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const
{
  return _impl->CheckIsPathSafe(path, startAngle, validPath);
}

void LatticePlanner::GetTestPath(const Pose3d& startPose, Planning::Path &path, const PathMotionProfile* motionProfile)
{
  State_c currentRobotState(startPose.GetTranslation().x(),
                            startPose.GetTranslation().y(),
                            startPose.GetRotationAngle<'Z'>().ToFloat());

  _impl->_context.start = currentRobotState;

  xythetaPlan plan;
  _impl->_planner.GetTestPlan(plan);
  PRINT_CH_INFO("Planner", "GetTestPath.Plan", "Plan from xytheta planner:");
  _impl->_context.env.GetActionSpace().PrintPlan(plan);

  Planning::Path outputPath;
  _impl->_context.env.GetActionSpace().AppendToPath(plan, outputPath);

  if( motionProfile != nullptr ) {
    ApplyMotionProfile(outputPath, *motionProfile, path);
  }
  else {
    path = outputPath;
  }

  PRINT_CH_INFO("Planner", "GetTestPath.Path", "Planning::Path:");
  path.PrintPath();
}


bool LatticePlanner::GetCompletePath_Internal(const Pose3d& currentRobotPose, Planning::Path &path)
{
  GoalID waste;
  return _impl->GetCompletePath(currentRobotPose, path, waste);
}

bool LatticePlanner::GetCompletePath_Internal(const Pose3d& currentRobotPose,
                                              Planning::Path &path,
                                              Planning::GoalID& selectedTargetIndex)
{
  auto ret = _impl->GetCompletePath(currentRobotPose, path, selectedTargetIndex);
  _selectedTargetIdx = selectedTargetIndex;
  return ret;
}

void LatticePlanner::SetIsSynchronous(bool val)
{
  _impl->SetIsSynchronous(val);
}

void LatticePlanner::SetArtificialPlannerDelay_ms(int ms)
{
  _impl->_msToBlock = ms;
  PRINT_CH_INFO("Planner", "LatticePlanner.SetDelay", "Adding %dms of artificial delay", ms);
}



}
}
