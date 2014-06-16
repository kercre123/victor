/**
 * File: xythetaPlanner.h
 *
 * Author: Brad Neuman
 * Created: 2014-04-30
 *
 * Description: A* lattice planner
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKICORETECH_PLANNING_XYTHETA_PLANNER_H_
#define _ANKICORETECH_PLANNING_XYTHETA_PLANNER_H_

#include <stddef.h>

namespace Anki
{
namespace Planning
{

// TODO:(bn) env_fwd.h file?

struct xythetaPlannerImpl;
class xythetaEnvironment;
class xythetaPlan;
class State_c;

class xythetaPlanner
{
public:

  xythetaPlanner(const xythetaEnvironment& env);
  ~xythetaPlanner();

  // set a goal in meters and radians. Returns true if it is valid,
  // false otherwise
  bool SetGoal(const State_c& goal);

  // Re-checks the existing goal to see if it is valid
  bool GoalIsValid() const;

  // set the starting state. Will be rounded to the nearest continuous
  // state. Returns true if it is valid, false otherwise
  bool SetStart(const State_c& start);

  // Allow (or disallow) free turn-in-place at the goal
  void AllowFreeTurnInPlaceAtGoal(bool allow = true);

  // Tells the planner to replan from scratch next time
  void SetReplanFromScratch();

  // Returns true if the plan is safe and complete, false
  // otherwise. This should always return true immediately after
  // Replan returns true, but if the environment is updated it can be
  // useful to re-check the plan.
  // 
  // First argument is how much of the path has already been
  // executed. Note that this is different form the robot's
  // currentPathSegment because our plans are different from robot
  // paths
  // 
  // Second argument value will be set to last valid state along the
  // path before collision if unsafe (or the goal if safe)
  // 
  // Third argument is the valid portion of the plan, up to lastSafeState
  bool PlanIsSafe(const float maxDistancetoFollowOldPlan_mm, int currentPathIndex = 0) const;
  bool PlanIsSafe(const float maxDistancetoFollowOldPlan_mm,
                  int currentPathIndex,
                  State_c& lastSafeState,
                  xythetaPlan& validPlan) const;

  // This essentially projects the given pose onto the plan (held in
  // this member). The projection is just the closest euclidean
  // distance point on the plan, and the return value is the number of
  // complete plan actions that are finished by the time you get to
  // this point
  size_t FindClosestPlanSegmentToPose(const State_c& state) const;

  // Computes a path from start to goal. Returns true if path found,
  // false otherwise. Note that replanning may or may not actually
  // trigger the planner. E.g. if the environment hasn't changed
  // (much), it may just use the same path
  bool Replan();  

  // TEMP: for compatibility
  bool ComputePath() {Replan(); return true;}

  // must call compute path before getting the plan
  xythetaPlan& GetPlan();
  const xythetaPlan& GetPlan() const;

private:
  xythetaPlannerImpl* _impl;

};

}
}

#endif
