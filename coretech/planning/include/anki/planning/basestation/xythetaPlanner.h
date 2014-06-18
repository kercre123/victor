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
#include "xythetaPlanner_definitions.h"

#define DEFUALT_MAX_EXPANSIONS 1000000

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
  State_c GetGoal() const;

  // Re-checks the existing goal to see if it is valid
  bool GoalIsValid() const;

  // set the starting state. Will be rounded to the nearest continuous
  // state. Returns true if it is valid, false otherwise
  bool SetStart(const State_c& start);

  // Allow (or disallow) free turn-in-place at the goal
  void AllowFreeTurnInPlaceAtGoal(bool allow = true);

  // Tells the planner to replan from scratch next time
  void SetReplanFromScratch();

  // Computes a path from start to goal. Returns true if path found,
  // false otherwise. Note that replanning may or may not actually
  // trigger the planner. E.g. if the environment hasn't changed
  // (much), it may just use the same path
  bool Replan(unsigned int maxExpansions = DEFUALT_MAX_EXPANSIONS);  

  // must call compute path before getting the plan
  xythetaPlan& GetPlan();
  const xythetaPlan& GetPlan() const;

  Cost GetFinalCost() const;

private:
  xythetaPlannerImpl* _impl;

};

}
}

#endif
