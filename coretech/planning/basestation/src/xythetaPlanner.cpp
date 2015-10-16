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

#include "anki/common/basestation/utils/helpers/boundedWhile.h"
#include "anki/planning/basestation/xythetaPlanner.h"
#include "anki/planning/basestation/xythetaPlannerContext.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "xythetaPlanner_internal.h"
#include <assert.h>
#include <iostream>


namespace Anki {
namespace Planning {

// When doing the soft obstacle goal heuristic expansion, don't do more than this many (to avoid getting stuck
// in a loop there)
#define MAX_HEUR_EXPANSIONS 1000

// When doing the soft obstacle goal heuristic expansion, keep expanding until this many states "escape" the
// penalty zone
#define COLLISION_GOAL_HEURISTIC_NUM_ESCAPES 20
  
xythetaPlanner::xythetaPlanner(const xythetaPlannerContext& context)
{
  _impl = new xythetaPlannerImpl(context);
}

xythetaPlanner::~xythetaPlanner()
{
  Util::SafeDelete( _impl );
}

bool xythetaPlanner::GoalIsValid() const
{
  return _impl->GoalIsValid();
}

bool xythetaPlanner::StartIsValid() const
{
  return _impl->StartIsValid();
}

bool xythetaPlanner::Replan(unsigned int maxExpansions, bool* runPlan)
{
  using namespace std::chrono;

  high_resolution_clock::time_point start = high_resolution_clock::now();
  bool ret = _impl->ComputePath(maxExpansions, runPlan);
  high_resolution_clock::time_point end = high_resolution_clock::now();

  duration<double> time_d = duration_cast<duration<double>>(end - start);
  double time = time_d.count();

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
  , _goalChanged(false)
  , _searchNum(0)
  , _runPlan(nullptr)
{
  _startID = _start.GetStateID();
  Reset();
}

bool xythetaPlannerImpl::CheckContextGoal(StateID& goalID, State_c& roundedGoal_c) const
{
  if( _context.env.IsInCollision( _context.goal ) ) {
    PRINT_NAMED_INFO("xythetaPlanner.Goal.Invalid", "goal is in collision");
    return false;
  }

  State goal = _context.env.State_c2State( _context.goal );
  if( _context.env.IsInCollision( goal ) ){
    if( ! _context.env.RoundSafe( _context.goal, goal ) ) {
      PRINT_NAMED_INFO("xythetaPlanner.Goal.Invalid", "all possible discrete states of goal are in collision!");
      return false;
    }

    assert( ! _context.env.IsInCollision( goal ) );
  }

  goalID = goal.GetStateID();

  // convert back so this is still lined up perfectly with goalID_
  roundedGoal_c = _context.env.State2State_c(goal);

  return true;
}

bool xythetaPlannerImpl::GoalIsValid() const
{
  StateID waste1;
  State_c waste2;

  return CheckContextGoal(waste1, waste2);
}

bool xythetaPlannerImpl::StartIsValid() const
{
  State waste;

  return CheckContextStart(waste);
}

bool xythetaPlannerImpl::CheckContextStart(State& start) const
{
  if( _context.env.IsInCollision( _context.start ) ) {
    PRINT_NAMED_INFO("xythetaPlanner.Start.Invalid", "start is in collision");
    return false;
  }

  start = _context.env.State_c2State( _context.start );

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

  _goalChanged = false;

  _finalCost = 0.0f;
}

bool xythetaPlannerImpl::NeedsReplan() const
{
  // TODO:(bn) json. This value balances between keeping the old plan (to look smooth) and being dumb by going
  // too far out of the way, in case the new plan differs from old
  const float default_maxDistanceToReUse_mm = 60.0f;
  return ! _context.env.PlanIsSafe( _plan, default_maxDistanceToReUse_mm );
}

bool xythetaPlannerImpl::InitializeHeuristic()
{
  _costOutsideHeurMap = 0.0f;
  _heurMap.clear();

  if( _context.env.IsInSoftCollision( _goalID ) ) {
    _costOutsideHeurMap = ExpandCollisionStatesFromGoal();
    PRINT_NAMED_INFO("xythetaPlanner.GoalInSoftCollision.ExpandHeur",
                     "expanded penalty states near goal. Cost outside map = %f",
                     _costOutsideHeurMap);
  }

  if( _costOutsideHeurMap > 1000.0f ) {
    PRINT_NAMED_WARNING("xythetaPlanner.HuerMap.Fail",
                        "very high cost of _costOutsideHeurMap = %f, so bailing",
                        _costOutsideHeurMap);
    return false;
  }

  return true;
}

Cost xythetaPlannerImpl::ExpandCollisionStatesFromGoal()
{
  StateID curr(_goalID);
  
  _heurMap.insert(std::make_pair(curr, 0.0f));

  // use a separate open list to track expansions
  OpenList q;
  q.insert(curr, 0.0f);

  unsigned int heurExpansions = 0;

  // keep track of the number of times we break out of the cost region
  int numEscapes = 0;
  float minCostOutside = 0.0f;

  while(!q.empty()) {

    if( _runPlan && !*_runPlan ) {
      return 0.0;
    }

    Cost c = q.topF();
    curr = q.pop();

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
                         _heurMap.size());
        return minCostOutside;
      }
      else {
        continue;
      }
    }

    // this logic is almost identical to ExpandState, but using reverse successors
    SuccessorIterator it = _context.env.GetSuccessors(curr, c, true);

    if(!it.Done(_context.env))
      it.Next(_context.env);

    while(!it.Done(_context.env)) {

      StateID nextID = it.Front().stateID;
      float newC = it.Front().g;

      // returns FLT_MAX if nextID isn't present
      float oldFval = q.fVal(nextID);
      if(oldFval > newC) {

        // if this is a state we haven't expanded yet, store the heuristic and insert it for further
        // expansion. Also store it in the heurMap, using the actual cost, so we'll have a perfect heuristic
        // value for this state later

        if(_heurMap.count(nextID) == 0) {
          _heurMap.insert(std::make_pair(nextID, newC));
          q.insert(nextID, newC);
        }
      }

      it.Next(_context.env);
    }

    heurExpansions++;
    
    // Check for too many expansions
    if (heurExpansions > MAX_HEUR_EXPANSIONS) {
      PRINT_NAMED_WARNING("xythetaPlanner.ExpandCollisionStatesFromGoal.ExceededMaxExpansions", 
                          "exceeded max allowed expansions of %d",
                          MAX_HEUR_EXPANSIONS);
      // instead of hanging forever, just return an invalid value, which will trigger a plan failure
      // TODO:(bn) if we implement ARA* or similar, we should return 0 here instead
      return FLT_MAX;
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


bool xythetaPlannerImpl::ComputePath(unsigned int maxExpansions, bool* runPlan)
{
  _runPlan = runPlan;

  bool fromScratch = _context.forceReplanFromScratch;

  // handle context
  StateID newGoalID;
  if( ! CheckContextGoal( newGoalID, _goal_c ) ) {
    // invalid goal
    return false;
  }

  if( newGoalID != _goalID ) {
    _goalChanged = true;
    _goalID = newGoalID;

    // for now, replan if the goal changes, but this isn't necessary. Instead, we could just re-order the open
    // list and continue as usual
    fromScratch = true;
  }

  State newStart;
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
  while( !_open.empty() ) {

    if( _runPlan && !*_runPlan ) {
      return false;
    }

    StateID sid = _open.pop();
    if(sid == _goalID) {
      foundGoal = true;
      _finalCost = _table[sid].g_;
      PRINT_NAMED_INFO("xythetaPlanner.ExpandGoal", 
                       "expanded goal! cost = %f",
                       _finalCost);
      break;
    }

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
      State_c c = _context.env.State2State_c(State(sid));
      fprintf(_debugExpPlotFile, "%f %f %f %d\n",
                  c.x_mm,
                  c.y_mm,
                  c.theta,
                  sid.s.theta);
    }


    // TODO:(bn) make this a dev option / parameter
    if(_expansions % 10000 == 0) {
      printf("PLANDEBUG: %8d %8.5f = %8.5f + %8.5f\n",
             _expansions,
             _open.topF(),
             _table[_open.top()].g_,
             _open.topF() - _table[_open.top()].g_);
    }
  }

  if(foundGoal) {
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
  auto currTableEntry = _table[currID];

  Cost currG = currTableEntry.g_;
  
  SuccessorIterator it = _context.env.GetSuccessors(currID, currG);

  if(!it.Done( _context.env ))
    it.Next( _context.env );

  while(!it.Done( _context.env )) {
    _considerations++;

    StateID nextID = it.Front().stateID;
    float newG = it.Front().g;

    if(_context.allowFreeTurnInPlaceAtGoal && currID.s.x == _goalID.s.x && currID.s.y == _goalID.s.y) {
      newG = currG;
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
    else if(!oldEntry->second.IsClosed(_searchNum)) {
      // only update if g value is lower
      if(newG < oldEntry->second.g_) {
        Cost h = heur(nextID);
        Cost f = newG + h;
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
    
  return heur_internal(sid) + _costOutsideHeurMap;
}

Cost xythetaPlannerImpl::heur_internal(StateID sid)
{
  State s(sid);

  // TODO:(bn) opt: consider squaring the entire damn thing so we don't have to sqrt here (within
  // GetDistanceBetween). I.e. heur() would return squared value, and then f = g^2 + h. First, do a profile
  // and see if the sqrt is taking up any significant amount of time

  return _context.env.GetDistanceBetween(_goal_c, s) * _context.env.GetOneOverMaxVelocity();
}

void xythetaPlannerImpl::BuildPlan()
{
  // start at the goal and go backwards, pushing actions into the plan
  // until we get to the start id

  StateID curr = _goalID;
  BOUNDED_WHILE(1000, !(curr == _startID)) {
    auto it = _table.find(curr);

    assert(it != _table.end());

    _plan.Push(it->second.backpointerAction_, it->second.penaltyIntoState_);
    curr = it->second.backpointer_;
  }

  std::reverse(_plan.actions_.begin(), _plan.actions_.end());
  std::reverse(_plan.penalties_.begin(), _plan.penalties_.end());

  _plan.start_ = _start;

  PRINT_NAMED_INFO("xythetaPlanner.BuildPlan", "Created plan of length %lu", _plan.actions_.size());
}

void xythetaPlannerImpl::GetTestPlan(xythetaPlan& plan)
{
  // this is hardcoded for now!
  assert( _context.env.GetNumActions() == 9);

  constexpr int numPlans = 4;
  static int whichPlan = 0;

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
  }

  whichPlan = (whichPlan + 1) % numPlans;

  // TODO:(bn) this line below might have been doing something useful....
  // SetGoal(_context.env.State2State_c(_context.env.GetPlanFinalState(plan)));
}

}
}
