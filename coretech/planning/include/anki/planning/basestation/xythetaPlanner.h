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

  // set the starting state. Will be rounded to the nearest continuous
  // state. Returns true if it is valid, false otherwise
  bool SetStart(const State_c& start);

  // Allow (or disallow) free turn-in-place at the goal
  void AllowFreeTurnInPlaceAtGoal(bool allow = true);

  // Returns true if path found, false otherwise
  bool ComputePath();

  // must call compute path before getting the plan
  const xythetaPlan& GetPlan() const;

private:
  xythetaPlannerImpl* _impl;

};

}
}

#endif
