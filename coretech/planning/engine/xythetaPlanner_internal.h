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

  bool GoalsAreValid() const;
  bool GoalIsValid(GoalID goalID) const;
  bool GoalIsValid(const std::pair<GoalID, State_c>& goal_cPair) const;
  bool StartIsValid() const;

  // This function starts by making a copy of the context, then computes a plan
  bool ComputePath(unsigned int maxExpansions, volatile bool* runPlan);

  // helper functions
  void Reset();

  void ExpandState(StateID sid);

  // this one takes the map an penalties into account
  Cost heur(StateID sid);

  // this one computes based on the distance to the closest goal (including _costOutsideHeurMapPerGoal)
  Cost heur_internal(StateID sid);

  // must be called after start and goal are set. Returns true if everything is OK, false if we should fail.
  // Also removes any goals (_goals_c and _goalStateIDs) that exceed MAX_COST_HEUR_EXPANSIONS
  bool InitializeHeuristic();

  void BuildPlan();

  // checks if we need to replan from scratch
  bool NeedsReplan() const;
  
  // Compares against context's env, returns true if segments in path comprise a safe and complete plan
  // Clears and fills in a list of path segments whose cumulative penalty doesnt exceed the max penalty
  bool PathIsSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const;

  void GetTestPlan(xythetaPlan& plan);


  //////////////////////////////////////////////////////////////////////////////// 
  // internal helpers
  //////////////////////////////////////////////////////////////////////////////// 

  using GoalState_cPair  = std::pair<GoalID,State_c>;
  using GoalState_cPairs = std::vector<GoalState_cPair>;
  using GoalStateIDPairs = std::vector<std::pair<GoalID,StateID>>;
  
  // returns true if a single goal is valid, false otherwise. Sets values of arguments. If false is returned, may have
  // set or not set any of the return arguments. The method that takes as input a goalID looks up in the context's goals
  // the goal pair used the by the second method. This second method still uses the context's env, so only use the second
  // if you know the goal pair is equiv to that in the context, or will soon be equiv (i.e., youre about to insert it)
  bool CheckContextGoal(const GoalID& goalID, StateID& goalStateID, State_c& roundedGoal_c) const;
  bool CheckGoal(const GoalState_cPair& goal, StateID& goalStateID, State_c& roundedGoal_c) const;
  
  // Checks all goals, returns true if any goals are valid. Adds a map element to the arguments if the goal is valid.
  // If false is returned, may have set or not set any of the return arguments. 
  bool CheckContextGoals(GoalStateIDPairs& goalStateIDs,
                         GoalState_cPairs& roundedGoals_c) const;

  // same as above, but for the start
  bool CheckContextStart(GraphState& start) const;

  // Context holds everything that can change outside of planning, and should represent all of the config,
  // including start, goal, motion primitives, etc. and holds the environment.
  const xythetaPlannerContext& _context;

  // goalID to stateID and its inverse
  GoalStateIDPairs _goalStateIDs; // These will NOT remain sorted by GoalID
  std::unordered_map<u32, GoalID> _goalState2GoalID;

  // These goalid/state_c pairs are different from the context continuous states because they were rounded
  // down to _goalStateIDs, then converted back to continuous states. These will NOT remain sorted by GoalID.
  GoalState_cPairs _goals_c;

  GraphState _start;
  StateID _startID;

  OpenList _open;
  StateTable _table;
  
  bool _goalsChanged; // any goal changed
  
  xythetaPlan _plan;

  Cost _finalCost;
  GoalID _chosenGoalID;
  StateID _chosenGoalStateID;

  unsigned int _expansions;
  unsigned int _considerations;
  unsigned int _collisionChecks;

  unsigned int _searchNum;

  volatile bool* _runPlan;

  double _lastPlanTime;

  // assuming that goal is in soft collision, this function will do a breadth-first expansion until we
  // "escape" from the soft penalty. It will add these penalties to a heuristic map, so the heuristic can take
  // these soft penalties into account. It works backwards, for the case when goalStateID is the goal
  Cost ExpandCollisionStatesFromGoal(const StateID& goalStateID);

  // heuristic pre-computation stuff
  // for each goal, from closest goal id to min cost at level set around that goal
  // These will NOT remain sorted by GoalID, but the order should match that of _goalStateIDs
  std::vector<std::pair<GoalID,Cost>> _costOutsideHeurMapPerGoal;
  // key is a StateID converted to u32
  std::unordered_map<u32, Cost> _heurMap;
  typedef std::unordered_map<u32, Cost>::iterator HeurMapIter;

  // for debugging only
  FILE* _debugExpPlotFile;
};


}
}

#endif

