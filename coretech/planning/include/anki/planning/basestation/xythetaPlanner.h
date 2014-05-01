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

  // set a goal in meters and radians
  void SetGoal(const State_c& goal);

  void ComputePath();

  // must call compute path before getting the plan
  const xythetaPlan& GetPlan() const;

private:
  xythetaPlannerImpl* _impl;

};

}
}

#endif
