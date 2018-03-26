/**
 * File: xythetaPlanner.h
 *
 * Author: Brad Neuman
 * Created: 2014-04-30
 *
 * Description: A* lattice planner in (x,y,theta) space
 *
 * Copyright: Anki, Inc. 2014-2015
 *
 **/

#ifndef _ANKICORETECH_PLANNING_XYTHETA_PLANNER_H_
#define _ANKICORETECH_PLANNING_XYTHETA_PLANNER_H_

#include <stddef.h>
#include "coretech/planning/shared/goalDefs.h"
#include "coretech/planning/shared/path.h"
#include "xythetaActions.h"
#include <utility>

#define DEFUALT_MAX_EXPANSIONS 5000000

namespace Anki
{
namespace Planning
{

struct xythetaPlannerContext;
struct xythetaPlannerImpl;

class xythetaPlanner
{
public:

  // NOTE: you can't change context while the planner is running or undefined behavior will result!
  xythetaPlanner(const xythetaPlannerContext& context);
  ~xythetaPlanner();

  // Check if the goal (from context) is valid
  bool GoalIsValid(GoalID goalID) const;
  bool GoalsAreValid() const;
  
  // Check if a goal is valid. Compares against context obstacles, but not using its goals.
  bool GoalIsValid(const std::pair<GoalID, State_c>& goal_cPair) const;

  // Check if the start (from context) is valid
  bool StartIsValid() const;
  
  // Compares against context's env, returns true if segments in path comprise a safe and complete plan
  // Clears and fills in a list of path segments whose cumulative penalty doesnt exceed the max penalty
  bool PathIsSafe(const Path& path, float startAngle, Path& validPath) const;

  // Computes a path from start to goal. Returns true if path found, false otherwise. Note that replanning may
  // or may not actually trigger the planner. E.g. if the environment hasn't changed (much), it may just use
  // the same path. Everything in context is allowed to change between replan calls (but it won't necessarily
  // be efficient if too many things change). If the second argument is not null, then its value will be
  // checked somewhere within the planenr loop, and the planner will return quickly if it turns false
  bool Replan(unsigned int maxExpansions = DEFUALT_MAX_EXPANSIONS, volatile bool* runPlan = nullptr);

  // must Replan before getting the plan
  xythetaPlan& GetPlan();
  const xythetaPlan& GetPlan() const;

  // Return one of a set of hardcoded plans, useful for testing other components of the system
  void GetTestPlan(xythetaPlan& plan);

  Cost GetFinalCost() const;
  
  GoalID GetChosenGoalID() const;

  float GetLastPlanTime() const;
  int GetLastNumExpansions() const;
  int GetLastNumConsiderations() const;

private:
  xythetaPlannerImpl* _impl;

};

}
}

#endif
