#include "anki/planning/basestation/xythetaPlanner.h"

#include <iostream>
#include <assert.h>
#include "xythetaPlanner_internal.h"
#include "anki/common/basestation/general.h"

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

bool xythetaPlanner::SetGoal(const State_c& goal)
{
  return _impl->SetGoal(goal);
}

bool xythetaPlanner::GoalIsValid() const
{
  return _impl->GoalIsValid();
}

bool xythetaPlanner::SetStart(const State_c& start)
{
  return _impl->SetStart(start);
}

void xythetaPlanner::AllowFreeTurnInPlaceAtGoal(bool allow)
{
  _impl->freeTurnInPlaceAtGoal_ = allow;
}

bool xythetaPlanner::PlanIsSafe(const float maxDistancetoFollowOldPlan_mm, int currentPathIndex) const
{
  State_c waste1;
  xythetaPlan waste2;
  return _impl->PlanIsSafe(maxDistancetoFollowOldPlan_mm, currentPathIndex, waste1, waste2);
}

bool xythetaPlanner::PlanIsSafe(const float maxDistancetoFollowOldPlan_mm,
                                int currentPathIndex,
                                State_c& lastSafeState,
                                xythetaPlan& validPlan) const
{
  return _impl->PlanIsSafe(maxDistancetoFollowOldPlan_mm, currentPathIndex, lastSafeState, validPlan);
}

size_t xythetaPlanner::FindClosestPlanSegmentToPose(const State_c& state) const
{
  return _impl->FindClosestPlanSegmentToPose(state);
}

bool xythetaPlanner::Replan()
{
  return _impl->ComputePath();
}

void xythetaPlanner::SetReplanFromScratch()
{
  _impl->fromScratch_ = true;
}

const xythetaPlan& xythetaPlanner::GetPlan() const
{
  return _impl->plan_;
}

xythetaPlan& xythetaPlanner::GetPlan()
{
  return _impl->plan_;
}

////////////////////////////////////////////////////////////////////////////////
// implementation functions
////////////////////////////////////////////////////////////////////////////////

#define PLANNER_DEBUG_PLOT_STATES_CONSIDERED 0

xythetaPlannerImpl::xythetaPlannerImpl(const xythetaEnvironment& env)
  : start_(0,0,0)
  , env_(env)
  , goalChanged_(false)
  , freeTurnInPlaceAtGoal_(false)
  , searchNum_(0)
{
  startID_ = start_.GetStateID();
  Reset();
}

bool xythetaPlannerImpl::SetGoal(const State_c& goal_c)
{
  if(env_.IsInCollision(goal_c)) {
    printf("goal is in collision!\n");
    return false;
  }

  State goal = env_.State_c2State(goal_c);

  if(env_.IsInCollision(goal)) {
    printf("looks like goal is in collision but goal_c isn't. This is a failure but will be fixed soon...\n");
    return false;
  }
  goalID_ = goal.GetStateID();

  std::cout<<goal<<std::endl;

  // convert back so this is still lined up perfectly with goalID_
  goal_c_ = env_.State2State_c(goal);

  // TEMP: for now, replan if the goal changes, but this isn't
  // necessary. Instead, we could just re-order the open list and
  // continue as usual
  fromScratch_ = true;

  goalChanged_ = true;
  return true;
}

bool xythetaPlannerImpl::GoalIsValid() const
{
  return !env_.IsInCollision(goal_c_);
}


bool xythetaPlannerImpl::SetStart(const State_c& start)
{
  if(env_.IsInCollision(start)) {
    printf("start is in collision!\n");
    return false;
  }

  start_ = env_.State_c2State(start);

  if(env_.IsInCollision(start_)) {
    printf("rounded start is in collision! This is a failure for now, will be fixed soon.....\n");
    return false;
  }

  startID_ = start_.GetStateID();

  // if the start changes, can't re-use anything
  fromScratch_ = true;

  return true;
}


void xythetaPlannerImpl::Reset()
{
  plan_.Clear();

  // TODO:(bn) shouln't need to clear these if I use searchNum_
  // properly
  table_.Clear();
  open_.clear();

  expansions_ = 0;
  considerations_ = 0;
  collisionChecks_ = 0;

  goalChanged_ = false;
  fromScratch_ = false;
}

bool xythetaPlannerImpl::NeedsReplan() const
{
  State_c waste1;
  xythetaPlan waste2;
  // TODO:(bn) parameter or at least a comment.
  const float default_maxDistanceToReUse_mm = 60.0;
  return !PlanIsSafe(default_maxDistanceToReUse_mm, 0, waste1, waste2);
}

bool xythetaPlannerImpl::PlanIsSafe(const float maxDistancetoFollowOldPlan_mm,
                                    int currentPathIndex,
                                    State_c& lastSafeState,
                                    xythetaPlan& validPlan) const
{
  // collision check the old plan. If it starts at 'start' and ends at
  // 'goal' and doesn't have any collisions, then we are good.

  if(plan_.actions_.empty() || start_ != plan_.start_)
    return false;

  validPlan.actions_.clear();

  size_t numActions = plan_.actions_.size();

  StateID curr(start_.GetStateID());
  validPlan.start_ = curr;

  bool useOldPlan = true;

  State currentRobotState(start_);

  // first go through all the actions that we are "skipping"
  for(size_t i=0; i<currentPathIndex; ++i) {
    // advance without checking collisions
    env_.ApplyAction(plan_.actions_[i], curr, false);
  }

  lastSafeState = env_.State2State_c(curr);
  validPlan.start_ = curr;

  // now go through the rest of the plan, checking for collisions at each step
  for(size_t i = currentPathIndex; i < numActions; ++i) {

    // check for collisions and possibly update curr
    if(!env_.ApplyAction(plan_.actions_[i], curr, true)) {
      printf("Collision along plan action %lu (starting from %d)\n", i, currentPathIndex);
      // there was a collision trying to follow action i, so we are done
      break;
    }

    // no collision. If we are still within
    // maxDistancetoFollowOldPlan_mm, update the valid old plan
    if(useOldPlan) {

      validPlan.Push(plan_.actions_[i]);
      lastSafeState = env_.State2State_c(State(curr));

      // TODO:(bn) this is kind of wrong. It's using euclidean
      // distance, instead we should add up the lengths of each
      // action. That will be faster and better
      if(env_.GetDistanceBetween(lastSafeState, currentRobotState) > maxDistancetoFollowOldPlan_mm) {
        useOldPlan = false;
      }
    }
  }

  // if we get here, we either failed to apply an action, or reached
  // the end of the plan. If we are now at the goal, we don't need to
  // replan, so return true
  return curr == goalID_;
}


size_t xythetaPlannerImpl::FindClosestPlanSegmentToPose(const State_c& state) const
{
  // for now, this is implemented by simply going over every
  // intermediate pose and finding the closest one
  float closest = 999999.9;  // TODO:(bn) talk to people about this
  size_t startPoint = 0;

  State curr = plan_.start_;

  using namespace std;
  // cout<<"Searching for position near "<<state<<" along plan of length "<<plan_.Size()<<endl;

  size_t planSize = plan_.Size();
  for(size_t planIdx = 0; planIdx < planSize; ++planIdx) {
    const MotionPrimitive& prim(env_.GetRawMotionPrimitive(curr.theta, plan_.actions_[planIdx]));

    // the intermediate position (x,y) coordinates are all centered at
    // 0. Instead of shifting them over each time, we just shift the
    // state we are searching for (fewer ops)
    State_c target(state.x_mm - env_.GetX_mm(curr.x),
                   state.y_mm - env_.GetY_mm(curr.y),
                   0.0f);

    // first check the exact state.  // TODO:(bn) no sqrt!
    float initialDist = sqrt(pow(target.x_mm, 2) + pow(target.y_mm, 2));
    // cout<<planIdx<<": "<<"iniitial "<<target<<" = "<<initialDist;
    if(initialDist <= closest + 1e-6) {
      closest = initialDist;
      startPoint = planIdx;
      // cout<<"  closest\n";
    }
    else {
      // cout<<endl;
    }

    for(const auto& position : prim.intermediatePositions) {
      // TODO:(bn) get squared distance
      float dist = env_.GetDistanceBetween(target, position);

      // cout<<planIdx<<": "<<"position "<<position<<" --> "<<target<<" = "<<dist;

      if(dist < closest) {
        closest = dist;
        startPoint = planIdx;
        // cout<<"  closest\n";
      }
      else {
        // cout<<endl;
      }
    }

    StateID currID(curr);
    env_.ApplyAction(plan_.actions_[planIdx], currID, false);
    curr = State(currID);
  }

  return startPoint;
}

bool xythetaPlannerImpl::ComputePath()
{
  if(fromScratch_ || NeedsReplan()) {
    Reset();
  }
  else {
    printf("No replan needed!\n");
    return true;
  }

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

  return foundGoal;
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
  BOUNDED_WHILE(1000, !(curr == startID_)) {
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
