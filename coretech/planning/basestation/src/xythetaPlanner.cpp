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

void xythetaPlanner::SetStart(const State_c& start)
{
  _impl->SetStart(start);
}

void xythetaPlanner::AllowFreeTurnInPlaceAtGoal(bool allow)
{
  _impl->freeTurnInPlaceAtGoal_ = allow;
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

#define PLANNER_DEBUG_PLOT_STATES_CONSIDERED 0

xythetaPlannerImpl::xythetaPlannerImpl(const xythetaEnvironment& env)
  : env_(env),
    start_(0,0,0),
    searchNum_(0),
    freeTurnInPlaceAtGoal_(false)
{
  startID_ = start_.GetStateID();
  Reset();
}

void xythetaPlannerImpl::SetGoal(const State_c& goal_c)
{
  std::cout<<goal_c.x_mm<<" "<<goal_c.y_mm<<" "<<goal_c.theta<<std::endl;

  State goal = env_.State_c2State(goal_c);
  goalID_ = goal.GetStateID();

  std::cout<<goal<<std::endl;

  // convert back so this is still lined up perfectly with goalID_
  goal_c_ = env_.State2State_c(goal);
}

void xythetaPlannerImpl::SetStart(const State_c& start)
{
  start_ = env_.State_c2State(start);
  startID_ = start_.GetStateID();
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

  std::cout<<"planning from '"<<start_<<"' to '"<<State(goalID_)<<"'\n";

  Reset();

  if(PLANNER_DEBUG_PLOT_STATES_CONSIDERED) {
    debugExpPlotFile_ = fopen("expanded.txt", "w");
  }

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
      printf("expanded goal! cost = %f\n", table_[sid].g_);
      break;
    }

    ExpandState(sid);
    expansions_++;

    if(PLANNER_DEBUG_PLOT_STATES_CONSIDERED) {
      State_c c = env_.State2State_c(State(sid));
      fprintf(debugExpPlotFile_, "%f %f %f %d\n",
                  c.x_mm,
                  c.y_mm,
                  c.theta,
                  sid.theta);
    }


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
  else {
    printf("no path found!\n");
  }

  if(PLANNER_DEBUG_PLOT_STATES_CONSIDERED) {
    fclose(debugExpPlotFile_);
  }

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
    float newG = it.Front().g;

    if(freeTurnInPlaceAtGoal_ && currID.x == goalID_.x && currID.y == goalID_.y)
      newG = currG;

    auto oldEntry = table_.find(nextID);

    if(oldEntry == table_.end()) {    
      Cost h = heur(nextID);
      Cost f = newG + h;
      table_.emplace(nextID,
                         open_.insert(nextID, f),
                         currID,
                         it.Front().actionID,
                         newG);
    }
    else if(!oldEntry->second.IsClosed(searchNum_)) {
      // only update if g value is lower
      if(newG < oldEntry->second.g_) {
        Cost h = heur(nextID);
        Cost f = newG + h;
        oldEntry->second.openIt_ = open_.insert(nextID, f);
        oldEntry->second.closedIter_ = -1;
        oldEntry->second.backpointer_ = currID;
        oldEntry->second.backpointerAction_ = it.Front().actionID;
        oldEntry->second.g_ = newG;
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

  return env_.GetDistanceBetween(goal_c_, s) * env_.GetOneOverMaxVelocity();
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
