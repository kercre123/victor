/**
 * File: xythetaPlanner_internal.h
 *
 * Author: Brad Neuman
 * Created: 2014-04-30
 *
 * Description: internal definition for x,y,theta lattice planner
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKICORETECH_PLANNING_XYTHETA_PLANNER_INTERNAL_H_
#define _ANKICORETECH_PLANNING_XYTHETA_PLANNER_INTERNAL_H_

#include "openList.h"

namespace Anki
{
namespace Planning
{

struct xythetaPlannerImpl
{
  xythetaPlannerImpl(const xythetaEnvironment& env);

  void SetGoal(const State_c& goal);

  void ComputePath();

  State goal_;
  State start_;
  const xythetaEnvironment& env_;
  OpenList open_;

  xythetaPlan plan_;
};


}
}

#endif

