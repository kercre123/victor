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
#include "xythetaPlanner_definitions.h"

#define DEFUALT_MAX_EXPANSIONS 5000000

namespace Anki
{
namespace Planning
{

// TODO:(bn) env_fwd.h file?

class State_c;
class xythetaPlan;
struct xythetaPlannerContext;
struct xythetaPlannerImpl;

class xythetaPlanner
{
public:

  // NOTE: you can't change context while the planner is running or undefined behavior will result!
  xythetaPlanner(const xythetaPlannerContext& context);
  ~xythetaPlanner();

  // Check if the goal (from context) is valid
  bool GoalIsValid() const;

  // Check if the start (from context) is valid
  bool StartIsValid() const;

  // Computes a path from start to goal. Returns true if path found, false otherwise. Note that replanning may
  // or may not actually trigger the planner. E.g. if the environment hasn't changed (much), it may just use
  // the same path. Everything in context is allowed to change between replan calls (but it won't necessarily
  // be efficient if too many things change)
  bool Replan(unsigned int maxExpansions = DEFUALT_MAX_EXPANSIONS);  

  // must call compute path before getting the plan
  xythetaPlan& GetPlan();
  const xythetaPlan& GetPlan() const;

  // Return one of a set of hardcoded plans, useful for testing other components of the system
  void GetTestPlan(xythetaPlan& plan);

  Cost GetFinalCost() const;

private:
  xythetaPlannerImpl* _impl;

};

}
}

#endif
