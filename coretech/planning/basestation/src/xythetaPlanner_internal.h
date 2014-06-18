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
#include "stateTable.h"

namespace Anki
{
namespace Planning
{

struct xythetaPlannerImpl
{
  xythetaPlannerImpl(const xythetaEnvironment& env);

  bool SetGoal(const State_c& goal);
  bool SetStart(const State_c& start);

  bool GoalIsValid() const;

  bool ComputePath(unsigned int maxExpansions);

  // helper functions

  void Reset();

  void ExpandState(StateID sid);

  Cost heur(StateID sid);

  void BuildPlan();

  // checks if we need to replan from scratch
  bool NeedsReplan() const;

  StateID goalID_;
  State_c goal_c_;
  State start_;
  StateID startID_;
  const xythetaEnvironment& env_;
  OpenList open_;
  StateTable table_;

  bool goalChanged_;
  bool fromScratch_;
  
  bool freeTurnInPlaceAtGoal_;

  xythetaPlan plan_;

  Cost finalCost_;

  unsigned int expansions_;
  unsigned int considerations_;
  unsigned int collisionChecks_;

  unsigned int searchNum_;

  // for debugging only
  FILE* debugExpPlotFile_;
};


}
}

#endif

