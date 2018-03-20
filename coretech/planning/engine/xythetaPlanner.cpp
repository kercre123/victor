/**
 * File: xythetaPlanner.cpp
 *
 * Author: Brad Neuman
 * Created: 2015-09-14
 *
 * Description: implementation of the xytheta lattice planner
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "coretech/planning/engine/xythetaPlanner.h"
#include "coretech/planning/engine/xythetaPlannerContext.h"
#include "util/global/globalDefinitions.h"
#include "util/helpers/boundedWhile.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "xythetaPlanner_internal.h"
#include <assert.h>
#include <iostream>
#include <unordered_set>


namespace Anki {
namespace Planning {

// When doing the soft obstacle goal heuristic expansion, don't do more than this many (to avoid getting stuck
// in a loop there)
#define MAX_HEUR_EXPANSIONS 1000

// When doing the soft obstacle goal heuristic expansion, prune goals if they exceed this cost
#define MAX_COST_HEUR_EXPANSIONS 1000.0f

// When doing the soft obstacle goal heuristic expansion, keep expanding until this many states "escape" the
// penalty zone
#define COLLISION_GOAL_HEURISTIC_NUM_ESCAPES 1
  
xythetaPlanner::xythetaPlanner(const xythetaPlannerContext& context)
{
  _impl = new xythetaPlannerImpl(context);
}

xythetaPlanner::~xythetaPlanner()
{
  Util::SafeDelete( _impl );
}

bool xythetaPlanner::GoalIsValid(GoalID goalID) const
{
  return _impl->GoalIsValid(goalID);
}

bool xythetaPlanner::GoalIsValid(const std::pair<GoalID, State_c>& goal_cPair) const
{
  return _impl->GoalIsValid(goal_cPair);
}

bool xythetaPlanner::GoalsAreValid() const
{
  return _impl->GoalsAreValid();
}

bool xythetaPlanner::StartIsValid() const
{
  return _impl->StartIsValid();
}
  
bool xythetaPlanner::PathIsSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const
{
  return _impl->PathIsSafe(path, startAngle, validPath);
}

bool xythetaPlanner::Replan(unsigned int maxExpansions, volatile bool* runPlan)
{
  using namespace std::chrono;

  high_resolution_clock::time_point start = high_resolution_clock::now();
  bool ret = _impl->ComputePath(maxExpansions, runPlan);
  high_resolution_clock::time_point end = high_resolution_clock::now();

  duration<double> time_d = duration_cast<duration<double>>(end - start);
  double time = time_d.count();
  _impl->_lastPlanTime = time;

  PRINT_NAMED_INFO("xythetaPlanner.ReplanComplete", 
                   "planning took %f seconds. (%f exps/sec, %f cons/sec, %f checks/sec)",
                   time,
                   ((double)_impl->_expansions) / time,
                   ((double)_impl->_considerations) / time,
                   ((double)_impl->_collisionChecks) / time);

  return ret;
}

const xythetaPlan& xythetaPlanner::GetPlan() const
{
  return _impl->_plan;
}

xythetaPlan& xythetaPlanner::GetPlan()
{
  return _impl->_plan;
}

Cost xythetaPlanner::GetFinalCost() const
{
  return _impl->_finalCost;
}

GoalID xythetaPlanner::GetChosenGoalID() const
{
  return _impl->_chosenGoalID;
}

float xythetaPlanner::GetLastPlanTime() const
{
  return _impl->_lastPlanTime;
}

int xythetaPlanner::GetLastNumExpansions() const
{
  return _impl->_expansions;
}

int xythetaPlanner::GetLastNumConsiderations() const
{
  return _impl->_considerations;
}

void xythetaPlanner::GetTestPlan(xythetaPlan& plan)
{
  _impl->GetTestPlan(plan);
}


////////////////////////////////////////////////////////////////////////////////
// implementation functions
////////////////////////////////////////////////////////////////////////////////

#define PLANNER_DEBUG_PLOT_STATES_CONSIDERED 0

xythetaPlannerImpl::xythetaPlannerImpl(const xythetaPlannerContext& context)
  : _context(context)
  , _start(0,0,0)
  , _goalsChanged(false)
  , _searchNum(0)
  , _runPlan(nullptr)
  , _lastPlanTime(-1.0)
{
  _startID = _start.GetStateID();
  Reset();
}

bool xythetaPlannerImpl::CheckContextGoal(const GoalID& goalID, StateID& goalStateID, State_c& roundedGoal_c) const
{
  for(const auto& goalPair : _context.goals_c) {
    if(goalPair.first == goalID) {
      return CheckGoal(goalPair, goalStateID, roundedGoal_c);
    }
  }
  
  PRINT_NAMED_ERROR("xythetaPlanner.GoalID.Invalid", "invalid goal id %d", (int)goalID);
  return false;
}

bool xythetaPlannerImpl::CheckGoal(const std::pair<GoalID,State_c>& goal, StateID& goalStateID, State_c& roundedGoal_c) const
{
  const GoalID& goalID = goal.first;
  const State_c& goal_c = goal.second;
  
  if( _context.env.IsInCollision( goal_c ) ) {
    PRINT_NAMED_INFO("xythetaPlanner.Goal.Invalid", "goal %d is in collision", (int)goalID);
    return false;
  }

  GraphState goalState( goal_c );
  if( _context.env.IsInCollision( goalState ) ){
    if( ! _context.env.RoundSafe( goal_c, goalState ) ) {
      PRINT_NAMED_INFO("xythetaPlanner.Goal.Invalid", "all possible discrete states of goal %d are in collision!", (int)goalID);
      return false;
    }
    assert( ! _context.env.IsInCollision( goalState ) );
  }
  goalStateID = goalState.GetStateID();
  // convert back so this is still lined up perfectly with goalID_
  roundedGoal_c = _context.env.State2State_c(goalState);

  return true;
}

bool xythetaPlannerImpl::CheckContextGoals(GoalStateIDPairs& goalStateIDs,
                                           GoalState_cPairs& roundedGoals_c) const
{
  GoalState_cPairs validGoals_c;
  validGoals_c.reserve(_context.goals_c.size());
  
  for( const auto& goalPair : _context.goals_c ) {
    if( _context.env.IsInCollision( goalPair.second ) ) {
      PRINT_NAMED_INFO("xythetaPlanner.Goal.Invalid", "goal %d is in collision", (int)goalPair.first);
    } else {
      validGoals_c.push_back(goalPair);
    }
  }
  
  if (validGoals_c.empty()) {
    PRINT_NAMED_INFO("xythetaPlanner.Goal.AllInvalid", "all goals were in collision");
    return false;
  }
  
  goalStateIDs.clear();
  roundedGoals_c.clear();
  goalStateIDs.reserve(validGoals_c.size());
  roundedGoals_c.reserve(validGoals_c.size());

  for( const auto& goalPair : validGoals_c) {
    const GoalID& goalID = goalPair.first;
    const State_c& goal_c = goalPair.second;
    GraphState goal( goal_c );
    if( _context.env.IsInCollision( goal ) ){
      if( ! _context.env.RoundSafe( goal_c, goal ) ) {
        PRINT_NAMED_INFO("xythetaPlanner.Goal.Invalid",
                         "all possible discrete states of goal %d are in collision!",
                         (int)goalID);
      }
      assert( ! _context.env.IsInCollision( goal ) );
      continue;
    }
    goalStateIDs.emplace_back(goalID, goal.GetStateID());
    // convert back so this is still lined up perfectly with goalID_
    roundedGoals_c.emplace_back(goalID, _context.env.State2State_c(goal));
  }
  
  if(goalStateIDs.empty()) {
    PRINT_NAMED_INFO("xythetaPlanner.Goal.AllInvalid",
                     "all possible discrete states of all goals are in collision!");
    return false;
  }
  
  return true;
}

bool xythetaPlannerImpl::GoalsAreValid() const
{
  GoalStateIDPairs waste1;
  GoalState_cPairs waste2;

  return CheckContextGoals(waste1, waste2);
}

bool xythetaPlannerImpl::GoalIsValid(GoalID goalID) const
{
  StateID waste1;
  State_c waste2;

  return CheckContextGoal(goalID, waste1, waste2);
}
bool xythetaPlannerImpl::GoalIsValid(const std::pair<GoalID, State_c>& goal_cPair) const
{
  StateID waste1;
  State_c waste2;
  
  return CheckGoal(goal_cPair, waste1, waste2);
}

bool xythetaPlannerImpl::StartIsValid() const
{
  GraphState waste;

  return CheckContextStart(waste);
}

bool xythetaPlannerImpl::CheckContextStart(GraphState& start) const
{
  if( _context.env.IsInCollision( _context.start ) ) {
    PRINT_NAMED_INFO("xythetaPlanner.Start.Invalid", "start is in collision");
    return false;
  }

  start = GraphState( _context.start );

  if( _context.env.IsInCollision(start) ) {
    if( ! _context.env.RoundSafe( _context.start, start ) ) {
      PRINT_NAMED_INFO("xythetaPlanner.Start.Invalid", "all possible discrete states of start are in collision!");
      return false;
    }

    assert( ! _context.env.IsInCollision(start) );
  }

  return true;
}


void xythetaPlannerImpl::Reset()
{
  _plan.Clear();

  // TODO:(bn) shouln't need to clear these if I use searchNum_ properly
  _table.Clear();
  _open.clear();

  _expansions = 0;
  _considerations = 0;
  _collisionChecks = 0;

  _goalsChanged = false;

  _finalCost = 0.0f;
}

bool xythetaPlannerImpl::NeedsReplan() const
{
  // TODO:(bn) json. This value balances between keeping the old plan (to look smooth) and being dumb by going
  // too far out of the way, in case the new plan differs from old
  const float default_maxDistanceToReUse_mm = 40.0f;
  return ! _context.env.PlanIsSafe( _plan, default_maxDistanceToReUse_mm );
}

bool xythetaPlannerImpl::PathIsSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const
{
  return _context.env.PathIsSafe(path, startAngle, validPath);
}

bool xythetaPlannerImpl::InitializeHeuristic()
{
  _costOutsideHeurMapPerGoal.clear();
  _heurMap.clear();
  
  _costOutsideHeurMapPerGoal.resize(_goalStateIDs.size());

  // this goes backwards because we want to remove some elements without copying the vector
  for(int i = static_cast<int>(_goalStateIDs.size())-1; i>=0; --i)
  {
    // Goals are expected to have matching ids, although not necessarily be sorted by them
    DEV_ASSERT(_goalStateIDs[i].first == _goals_c[i].first, "xythetaPlannerImpl.InitializeHeuristic.IdMismatch");
    
    const auto& goalIDPair = _goalStateIDs[i];
    Cost costOutsideHeurMap = 0.0f;
    if( _context.env.IsInSoftCollision( goalIDPair.second ) ) {
      costOutsideHeurMap = ExpandCollisionStatesFromGoal( goalIDPair.second );
      PRINT_NAMED_INFO("xythetaPlanner.GoalInSoftCollision.ExpandHeur",
                       "expanded penalty states near goal %d. Cost outside map = %f",
                       (int)goalIDPair.first,
                       costOutsideHeurMap);
    }
    if( costOutsideHeurMap > MAX_COST_HEUR_EXPANSIONS ) {
      PRINT_NAMED_WARNING("xythetaPlanner.HeurMap.Fail",
                          "very high cost of _costOutsideHeurMap = %f, so removing goal %d",
                          costOutsideHeurMap,
                          (int)goalIDPair.first);
      
      // remove the i-th. we already checked back() (or this is back), so swap with back and pop the new back
      // things will be out of order, but we don't care at this point
      std::swap(_goalStateIDs[i], _goalStateIDs.back());
      _goalStateIDs.pop_back();
      // same thing for _goals_c
      std::swap(_goals_c[i], _goals_c.back());
      _goals_c.pop_back();
      // same thing for _costOutsideHeurMapPerGoal
      std::swap(_costOutsideHeurMapPerGoal[i], _costOutsideHeurMapPerGoal.back());
      _costOutsideHeurMapPerGoal.pop_back();
    } else {
      _costOutsideHeurMapPerGoal[i].first = goalIDPair.first;
      _costOutsideHeurMapPerGoal[i].second = costOutsideHeurMap;
    }
  }
  
  if( ANKI_DEVELOPER_CODE ) {
    // _goals_c _costOutsideHeurPerGoal and _goalStateIDs should have GoalIDs in matching order
    size_t len = _goals_c.size();
    assert(_costOutsideHeurMapPerGoal.size() == len);
    assert(_goalStateIDs.size() == len);
    for(size_t i=0; i<len; ++i) {
      assert(_goals_c[i].first == _costOutsideHeurMapPerGoal[i].first);
      assert(_goals_c[i].first == _goalStateIDs[i].first);
    }
  }
  
  const bool anyGoalsOK = !_goalStateIDs.empty();
  return anyGoalsOK;
}

Cost xythetaPlannerImpl::ExpandCollisionStatesFromGoal(const StateID& goalStateID)
{
  _heurMap[goalStateID] = 0.0f;
  
  std::unordered_set<u32> visited;

  // use a separate open list to track expansions
  OpenList q;
  q.insert(goalStateID, 0.0f);

  unsigned int heurExpansions = 0;

  // keep track of the number of times we break out of the cost region
  int numEscapes = 0;
  float minCostOutside = 0.0f;

  while(!q.empty()) {

    if( _runPlan && !*_runPlan ) {
      return 0.0;
    }

    Cost c = q.topF();
    StateID curr = q.pop();

    if(!_context.env.IsInSoftCollision(curr)) {
      if( numEscapes == 0 ) {
        minCostOutside = c;
      }

      numEscapes++;

      if( numEscapes >= COLLISION_GOAL_HEURISTIC_NUM_ESCAPES ) {
        PRINT_NAMED_INFO("xythetaPlanner.ExpandCollisionStatesFromGoal", 
                         "expanded %u states for heuristic, reached free space %d times with cost of %f and have %lu in heurMap",
                         heurExpansions,
                         numEscapes,
                         minCostOutside,
                         (unsigned long)_heurMap.size());
        return minCostOutside;
      }

      // NOTE: need more testing to prove to myself that allowing any COLLISION_GOAL_HEURISTIC_NUM_ESCAPES > 1
      // actually works, and is helpful at all.
    }
    else {
      auto itHeur = _heurMap.find(curr);
      if(itHeur == _heurMap.end()) {
        _heurMap.emplace(curr, c);
        visited.insert(curr);
      } else if (c < itHeur->second) {
        itHeur->second = c;
      }
    }

    // this logic is almost identical to ExpandState, but using reverse successors
    SuccessorIterator it = _context.env.GetSuccessors(curr, c, true);

    if(!it.Done(_context.env))
      it.Next(_context.env);

    while(!it.Done(_context.env)) {

      StateID nextID = it.Front().stateID;
      float newC = it.Front().g;

      it.Next(_context.env);

      if( visited.find(nextID) != visited.end() ) {
        continue;
      }

      // simplified best-first expansion backwards from the goal

      // this might repeat work (no closed list), but who cares, this is a short bit of code
      q.insert(nextID, newC);
    }

    heurExpansions++;
    
    // Check for too many expansions
    if (heurExpansions > MAX_HEUR_EXPANSIONS) {
      PRINT_NAMED_WARNING("xythetaPlanner.ExpandCollisionStatesFromGoal.ExceededMaxExpansions", 
                          "exceeded max allowed expansions of %d",
                          MAX_HEUR_EXPANSIONS);
      // instead of hanging forever, just return the cost calculated so far
      // TODO:(bn) if we implement ARA* or similar, we should return 0 here instead
      return c;
    }
  }

  // We should usually be able to escape form a soft collision, but if we run out of states to expand, it
  // means we couldn't escape, but hopefully we filled up heurMap enough to plan. This could happen if the
  // start and goal were both inside a soft obstacle that had a hard obstacle around it
  PRINT_NAMED_INFO("xythetaPlanner.ExpandCollisionStatesFromGoal.EmptyOpenList",
                   "ran out of open list entries during ExpandStatesForHeur after %u exps!",
                   heurExpansions);
  return minCostOutside;
}


bool xythetaPlannerImpl::ComputePath(unsigned int maxExpansions, volatile bool* runPlan)
{
  _runPlan = runPlan;

  bool fromScratch = _context.forceReplanFromScratch;

  // handle context
  GoalStateIDPairs newGoalIDs;
  if( ! CheckContextGoals( newGoalIDs, _goals_c ) ) {
    // all goals invalid
    return false;
  }

  if( newGoalIDs != _goalStateIDs ) {
    _goalsChanged = true;
    _goalStateIDs = newGoalIDs;
    
    // invert the map for seeing if a popped state is a goal
    _goalState2GoalID.clear();
    _goalState2GoalID.reserve(newGoalIDs.size());
    for(const auto& goalPair : newGoalIDs) {
      _goalState2GoalID[goalPair.second] = goalPair.first;
    }

    // for now, replan if any goals change, but this isn't necessary. Instead, we could just re-order the open
    // list and continue as usual
    fromScratch = true;
  }
  assert(!_goalStateIDs.empty());
  

  GraphState newStart;
  if( ! CheckContextStart( newStart ) ) {
    // invalid start
    return false;
  }

  StateID newStartID = newStart.GetStateID();
  if( newStartID != _startID ) {
    // start has changed, so current expansions are invalid
    fromScratch = true;
    _start = newStart;
    _startID = newStartID;
  }


  if(fromScratch || NeedsReplan()) {
    Reset();
  }
  else {
    PRINT_NAMED_INFO("xythetaPlanner.ComputePath.NoReplan", "No replan needed");
    return true;
  }

  if( ! InitializeHeuristic() ) {
    // invalid heuristic, planning wouldn't work, or would be incredibly slow
    return false;
  }

  if(PLANNER_DEBUG_PLOT_STATES_CONSIDERED) {
    _debugExpPlotFile = fopen("expanded.txt", "w");
  }

  StateID startID = _start.GetStateID();

  // push starting state
  _table.emplace(startID, 
                 _open.insert(startID, 0.0),
                 startID,
                 0, // action doesn't matter
                 0.0,
                 0.0);

  bool foundGoal = false;
  GoalID foundGoalID = 0;
  StateID foundGoalStateID = 0;
  while( !_open.empty() ) {

    if( _runPlan && !*_runPlan ) {
      return false;
    }

    StateID sid = _open.pop();
    
    auto it = _goalState2GoalID.find(sid);
    if( it != _goalState2GoalID.end() ) {
      foundGoal = true;
      foundGoalID = it->second;
      foundGoalStateID = it->first;

      _finalCost = _table[sid].g_;
      PRINT_NAMED_INFO("xythetaPlanner.ExpandGoal",
                       "expanded goal %d at state %u! cost = %f",
                       (int)foundGoalID,
                       (u32)foundGoalStateID,
                       _finalCost);
      break;
    }

    #ifndef NDEBUG
    // set iterator to null for an assert check later
    _table[sid].openIt_ = _open.nullIterator();
    #endif

    ExpandState(sid);
    _expansions++;
    if(_expansions > maxExpansions) {
      PRINT_NAMED_WARNING("xythetaPlanner.ExceededMaxExpansions", 
                          "exceeded max expansions of %u, stopping",
                          maxExpansions);
      printf("topF =  %8.5f (%8.5f + %8.5f)\n",
             _open.topF(),
             _table[_open.top()].g_,
             _open.topF() -
             _table[_open.top()].g_);
      return false;
    }

    if(PLANNER_DEBUG_PLOT_STATES_CONSIDERED) {
      State_c c = _context.env.State2State_c(GraphState(sid));
      fprintf(_debugExpPlotFile, "%f %f %f %d\n",
                  c.x_mm,
                  c.y_mm,
                  c.theta,
                  sid.s.theta);
    }


    // TODO:(bn) make this a dev option / parameter
    if(_expansions % 10000 == 0) {
      PRINT_NAMED_INFO("xythetaPlanner.PLANDEBUG",
                       "%8d %8.5f = %8.5f + %8.5f",
                       _expansions,
                       _open.topF(),
                       _table[_open.top()].g_,
                       _open.topF() - _table[_open.top()].g_);
    }
  }

  if(foundGoal) {
    _chosenGoalID = foundGoalID;
    _chosenGoalStateID = foundGoalStateID;
    BuildPlan();
  }
  else {
    PRINT_NAMED_INFO("xythetaPlanner.NoPlanFound", "");
  }

  if(PLANNER_DEBUG_PLOT_STATES_CONSIDERED) {
    fclose(_debugExpPlotFile);
  }

  PRINT_NAMED_INFO("xythetaPlanner.Complete", 
                   "finished after %d expansions. foundGoal = %d",
                   _expansions,
                   foundGoal);

  return foundGoal;
}

void xythetaPlannerImpl::ExpandState(StateID currID)
{
  // hold iterator to current table entry
  auto& currTableEntry = _table[currID];

  if( currTableEntry.closedIter_ == _searchNum ) {
    PRINT_NAMED_ERROR("xythetaPlanner.ExpandingClosedState", "This is a planner bug! Tell Brad immediately!");
    return;
  }

  Cost currG = currTableEntry.g_;
  
  SuccessorIterator it = _context.env.GetSuccessors(currID, currG);

  if(!it.Done( _context.env ))
    it.Next( _context.env );

  while(!it.Done( _context.env )) {
    _considerations++;

    const StateID nextID = it.Front().stateID;
    float newG = it.Front().g;

    if(_context.allowFreeTurnInPlaceAtGoal) {
      for(const auto& goalPair : _goalStateIDs) {
        if ((currID.s.x == goalPair.second.s.x) && (currID.s.y == goalPair.second.s.y)) {
          newG = currG;
          break;
        }
      }
    }

    // hold iterator to next state entry
    auto oldEntry = _table.find(nextID);

    if(oldEntry == _table.end()) {
      // no existing entry, so add a new one
      Cost h = heur(nextID);
      Cost f = newG + h;
      _table.emplace(nextID,
                     _open.insert(nextID, f),
                     currID,
                     it.Front().actionID,
                     it.Front().penalty,
                     newG);
    }
    // TODO:(bn) opt: delay computing the cost. If the node is closed, don't need to compute it. As I'm
    // computing it, pass in the oldEntry g value, because as soon as we hit that, we could bail out early
    else if(!oldEntry->second.IsClosed(_searchNum)) {
      // only update if g value is lower
      if(newG < oldEntry->second.g_) {
        Cost h = heur(nextID);
        Cost f = newG + h;

        // if the states are in the table, then they were in the open list at some point. Since they aren't
        // closed now, they must still be in Open, so remove the old entry to insert the new one
        assert( oldEntry->second.openIt_ != _open.nullIterator() );
        _open.remove(oldEntry->second.openIt_);
        oldEntry->second.openIt_ = _open.insert(nextID, f);

        oldEntry->second.closedIter_ = -1;
        oldEntry->second.backpointer_ = currID;
        oldEntry->second.backpointerAction_ = it.Front().actionID;
        oldEntry->second.g_ = newG;
      }
    }

    it.Next( _context.env );
  }

  currTableEntry.closedIter_ = _searchNum;
}

Cost xythetaPlannerImpl::heur(StateID sid)
{
  HeurMapIter it = _heurMap.find(sid);
  if(it != _heurMap.end()) {
    return it->second;
  }
  
  // if not in _heurMap
  return heur_internal(sid);
}

Cost xythetaPlannerImpl::heur_internal(StateID sid)
{
  GraphState s(sid);
  size_t len = _goals_c.size();
  
  Cost minCost = FLT_MAX;
  for(size_t i=0; i<len; ++i) {
    const auto& goal = _goals_c[i].second;
    // we checked i is in bounds when adding to the cost vector so dont here
    Cost costOutsideHeurMap = _costOutsideHeurMapPerGoal[i].second;
    Cost d = _context.env.GetDistanceBetween(goal, s) * _context.env.GetOneOverMaxVelocity() + costOutsideHeurMap;
    if (d < minCost) {
      minCost = d;
    }
  }
  
  return minCost;
}

void xythetaPlannerImpl::BuildPlan()
{
  // start at the goal and go backwards, pushing actions into the plan
  // until we get to the start id

  StateID curr = _chosenGoalStateID;
  BOUNDED_WHILE(1000, !(curr == _startID)) {
    auto it = _table.find(curr);
    assert(it != _table.end());

    _plan.Push(it->second.backpointerAction_, it->second.penaltyIntoState_);
    curr = it->second.backpointer_;
  }  

  _plan.Reverse();
  _plan.start_ = _start;

  PRINT_NAMED_INFO("xythetaPlanner.BuildPlan", "Created plan of length %lu", (unsigned long)_plan.Size());
}

void xythetaPlannerImpl::GetTestPlan(xythetaPlan& plan)
{
  // this is hardcoded for now!
  assert( _context.env.GetNumActions() == 9);

  static int whichPlan = 4;

  plan.Clear();
  plan.start_ = _start;

  switch(whichPlan) {
  case 0:
    // this one starts with a backup,, does a swerve then incresingly
    // right turns, heads back and does the opposite, and lands
    // exactly where it starts
    plan.Push(1);
    plan.Push(2);
    plan.Push(3);
    plan.Push(5);
    plan.Push(5);
    plan.Push(5);
    plan.Push(5);
    plan.Push(3);
    plan.Push(2);
    plan.Push(1);
    plan.Push(5);
    plan.Push(5);
    plan.Push(7);
    plan.Push(7);
    plan.Push(7);
    plan.Push(7);
    plan.Push(0);
    plan.Push(0);
    plan.Push(0);
    break;
  case 1:
    // start with slight left, T-shaped kind of thing, ends where it starts
    plan.Push(2);
    plan.Push(4);
    plan.Push(2);
    plan.Push(5);
    plan.Push(5);
    plan.Push(5);
    plan.Push(5);
    plan.Push(1);
    plan.Push(0);
    plan.Push(0);
    plan.Push(0);
    plan.Push(0);
    plan.Push(0);
    plan.Push(0);
    plan.Push(1);
    plan.Push(5);
    plan.Push(5);
    plan.Push(5);
    plan.Push(5);
    plan.Push(2);
    plan.Push(4);
    plan.Push(2);
    plan.Push(0);
    plan.Push(7);
    plan.Push(7);
    plan.Push(7);
    plan.Push(7);
    plan.Push(0);
    plan.Push(7);
    plan.Push(7);
    plan.Push(7);
    plan.Push(7);
    break;
  case 2:
    // funky figure 8, starting with hard right, ends where it starts
    plan.Push(5);
    plan.Push(5);
    plan.Push(5);
    plan.Push(5);
    plan.Push(5);
    plan.Push(5);
    plan.Push(5);
    plan.Push(5);
    plan.Push(4);
    plan.Push(4);
    plan.Push(4);
    plan.Push(4);
    plan.Push(4);
    plan.Push(4);
    plan.Push(4);
    plan.Push(4);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(3);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    plan.Push(2);
    break;

  case 3:
    // straights and 180 point turns
    plan.Push(7);
    plan.Push(7);
    plan.Push(7);
    plan.Push(7);
    plan.Push(7);
    plan.Push(7);
    plan.Push(7);
    plan.Push(7);
    plan.Push(0);
    plan.Push(0);
    plan.Push(0);
    plan.Push(0);
    plan.Push(0);
    plan.Push(6);
    plan.Push(6);
    plan.Push(6);
    plan.Push(6);
    plan.Push(6);
    plan.Push(6);
    plan.Push(6);
    plan.Push(6);
    plan.Push(0);
    plan.Push(0);
    plan.Push(0);
    plan.Push(0);
    break;

  case 4:
    plan.Push(8);
    plan.Push(7);
    plan.Push(8);
    plan.Push(5);
    plan.Push(0);
    plan.Push(0);
    plan.Push(0);
    plan.Push(0);
    plan.Push(2);
    plan.Push(4);
    plan.Push(0);
    break;      
  }

  // whichPlan = (whichPlan + 1) % numPlans;

  // TODO:(bn) this line below might have been doing something useful....
  // SetGoal(_context.env.State2State_c(_context.env.GetPlanFinalState(plan)));
}

}
}
