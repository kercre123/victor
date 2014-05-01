#include "anki/planning/basestation/xythetaPlanner.h"

#include <iostream>
#include <assert.h>
#include "xythetaPlanner_internal.h"

namespace Anki {
namespace Planning {

xythetaPlanner::xythetaPlanner(const xythetaEnvironment& env)
{
  _impl = new xythetaPlannerImpl(env);
}

xythetaPlanner::~xythetaPlanner()
{
  delete _impl;
  _impl = NULL;
}

void xythetaPlanner::SetGoal(const State_c& goal)
{
  _impl->SetGoal(goal);
}

void xythetaPlanner::ComputePath()
{
  _impl->ComputePath();
}

const xythetaPlan& xythetaPlanner::GetPlan() const
{
  return _impl->plan_;
}

////////////////////////////////////////////////////////////////////////////////
// implementation functions
////////////////////////////////////////////////////////////////////////////////

xythetaPlannerImpl::xythetaPlannerImpl(const xythetaEnvironment& env)
  : env_(env),
    start_(0,0,0),
    searchNum_(0)
{
  startID_ = start_.GetStateID();
  Reset();
}

void xythetaPlannerImpl::SetGoal(const State_c& goal_c)
{
  State goal = env_.State_c2State(goal_c);
  goalID_ = goal.GetStateID();

  // convert back so this is still lined up perfectly with goalID_
  goal_c_ = env_.State2State_c(goal);
}

void xythetaPlannerImpl::Reset()
{
  plan_.Clear();

  expansions_ = 0;
  considerations_ = 0;
  collisionChecks_ = 0;
}

void xythetaPlannerImpl::ComputePath()
{
  Reset();

  StateID startID = start_.GetStateID();

  // push starting state
  table_.emplace(startID, 
                     open_.insert(startID, 0.0),
                     startID,
                     0, // action doesn't matter
                     0.0);

  bool foundGoal = false;
  while(!open_.empty()) {
    StateID sid = open_.pop();
    if(sid == goalID_) {
      foundGoal = true;
      printf("expanded goal! done!\n");
      break;
    }

    ExpandState(sid);
    expansions_++;

    // TEMP: 
    if(expansions_ % 10000 == 0) {
      printf("%8d %8.5f = %8.5f + %8.5f\n",
                 expansions_,
                 open_.topF(),
                 table_[open_.top()].g_,
                 open_.topF() -
                 table_[open_.top()].g_);
    }
  }

  if(foundGoal)
    BuildPlan();

  printf("finished after %d expansions\n", expansions_);
}

void xythetaPlannerImpl::ExpandState(StateID currID)
{
  Cost currG = table_[currID].g_;
  
  SuccessorIterator it = env_.GetSuccessors(currID, currG);

  if(!it.Done())
    it.Next();

  while(!it.Done()) {
    considerations_++;

    StateID nextID = it.Front().stateID;

    auto oldEntry = table_.find(nextID);

    if(oldEntry == table_.end()) {    
      Cost h = heur(nextID);
      Cost f = it.Front().g + h;
      table_.emplace(nextID,
                         open_.insert(nextID, f),
                         currID,
                         it.Front().actionID,
                         it.Front().g);
    }
    else if(!oldEntry->second.IsClosed(searchNum_)) {
      // only update if g value is lower
      if(it.Front().g < oldEntry->second.g_) {
        Cost h = heur(nextID);
        Cost f = it.Front().g + h;
        oldEntry->second.openIt_ = open_.insert(nextID, f);
        oldEntry->second.closedIter_ = -1;
        oldEntry->second.backpointer_ = currID;
        oldEntry->second.backpointerAction_ = it.Front().actionID;
        oldEntry->second.g_ = it.Front().g;
      }
    }

    it.Next();    
  }

  table_[currID].closedIter_ = searchNum_;
}

Cost xythetaPlannerImpl::heur(StateID sid)
{
  State s(sid);
  // return euclidean distance in mm

  float distSq = 
    pow(env_.GetX_mm(s.x) - goal_c_.x_mm, 2)
    + pow(env_.GetY_mm(s.y) - goal_c_.y_mm, 2);

  return sqrtf(distSq);
}

void xythetaPlannerImpl::BuildPlan()
{
  // start at the goal and go backwards, pushing actions into the plan
  // until we get to the start id

  StateID curr = goalID_;
  // BOUNDED_WHILE(1000, curr != startID_) { // TODO:(bn) include this
  while(!(curr == startID_)) {
    auto it = table_.find(curr);

    assert(it != table_.end());

    plan_.Push(it->second.backpointerAction_);
    curr = it->second.backpointer_;
  }

  std::reverse(plan_.actions_.begin(), plan_.actions_.end());

  plan_.start_ = start_;

  printf("Created plan of length %lu\n", plan_.actions_.size());
}


}
}
