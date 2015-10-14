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
#include <unordered_map>

namespace Anki
{
namespace Planning
{

struct xythetaPlannerImpl
{
  xythetaPlannerImpl(const xythetaPlannerContext& context);

  bool GoalIsValid() const;
  bool StartIsValid() const;

  // This function starts by making a copy of the context, then computes a plan
  bool ComputePath(unsigned int maxExpansions, bool* runPlan);

  // helper functions
  void Reset();

  void ExpandState(StateID sid);

  // this one takes the map an penalties into account
  Cost heur(StateID sid);

  // this one computes jsut based on distance
  Cost heur_internal(StateID sid);

  // must be called after start and goal are set. Returns true if everything is OK, false if we should fail
  bool InitializeHeuristic();

  void BuildPlan();

  // checks if we need to replan from scratch
  bool NeedsReplan() const;

  void GetTestPlan(xythetaPlan& plan);


  //////////////////////////////////////////////////////////////////////////////// 
  // internal helpers
  //////////////////////////////////////////////////////////////////////////////// 

  // returns true if goal is valid, false otherwise. Sets values of arguments. If false is returned, may have
  // set or not set any of the return arguments
  bool CheckContextGoal(StateID& goalID, State_c& roundedGoal_c) const;

  // same as above, but for the start
  bool CheckContextStart(State& start) const;

  // Context holds everything that can change outside of planning, and should represent all of the config,
  // including start, goal, motion primitives, etc. and holds the environment.
  const xythetaPlannerContext& _context;

  StateID _goalID;

  // This is different from the context continuous state because it was rounded down to goalID_, then
  // converted back to a continuous state
  State_c _goal_c;

  State _start;
  StateID _startID;

  OpenList _open;
  StateTable _table;
  
  bool _goalChanged;
  
  xythetaPlan _plan;

  Cost _finalCost;

  unsigned int _expansions;
  unsigned int _considerations;
  unsigned int _collisionChecks;

  unsigned int _searchNum;

  bool* _runPlan;

  // assuming that `sid` is in soft collision, this function will do a breadth-first expansion until we
  // "escape" from the soft penalty. It will add these penalties to a heuristic map, so the heuristic can take
  // these soft penalties into account
  Cost ExpandStatesForHeur(StateID sid);

  // heuristic pre-computation stuff
  Cost _costOutsideHeurMap;
  std::unordered_map<u32, Cost> _heurMap; // key is a StateID converted to u32
  typedef std::unordered_map<u32, Cost>::iterator HeurMapIter;
  

  // for debugging only
  FILE* _debugExpPlotFile;
};


}
}

#endif

