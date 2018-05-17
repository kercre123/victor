/**
 * File: plannerStates.h
 *
 * Author: Michael Willett
 * Created: 2018-05-01
 *
 * Description: containers for optimizing graph search in planners
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef _ANKICORETECH_PLANNING_PLANNER_STATES_H_
#define _ANKICORETECH_PLANNING_PLANNER_STATES_H_

#include "coretech/planning/engine/xythetaActions.h"
#include <initializer_list>
#include <algorithm>
#include <utility>
#include <unordered_set>

namespace Anki {
namespace Planning {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// PlannerState - basic container for info needed by the planner
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct PlannerState {
  StateID  curr;
  StateID  prev;
  ActionID action;
  // MRW - we need to look into `penalty`. Right now, the only time we use this value is for `CheckPathIsSafe`, which 
  //       checks if the penalty has increased at all from when the path is created, but isn't necessarily fatal.
  //       the side affect is that the replanner always plans from the state right before any obstacle, and I don't
  //       think it is handling soft obstacle costs appropriately.
  Cost     penalty; 
  Cost     g;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ClosedList - hash for quick lookup if a state has been expanded
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct ClosedState : public PlannerState
{ 
  ClosedState() = default;  
  explicit ClosedState(StateID c) : PlannerState({.curr=c}) {}
  ClosedState(PlannerState&& o)   : PlannerState(std::move(o)) {}

  inline bool operator==(const ClosedState& o) const { return (curr == o.curr); }  // for set hashing
};

struct ClosedStateHasher
{
  size_t operator()(const ClosedState& s) const { return s.curr.v; }
};

using ClosedList = std::unordered_set<ClosedState, ClosedStateHasher>;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  OpenList - Priority Queue for getting best candidate for graph search. Note this is a slight adaptation of the 
//    STL priority_queue, but allows for Move semantics when calling `pop()` to save excess copying   
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct OpenState : public PlannerState
{ 
  OpenState() = default;
  explicit OpenState(StateID c) : PlannerState({.curr=c}), f(0) {}

  OpenState(StateID c, StateID l, ActionID a, Cost p, Cost g, Cost h) 
  : PlannerState({c, l, a, p, g}), f(g+h) {}

  Cost f;
};

class OpenList2 : private std::vector<OpenState>
{
public:
  using std::vector<OpenState>::clear;
  using std::vector<OpenState>::empty;
  using std::vector<OpenState>::size;

  inline OpenState top() const { return front(); }

  template<typename... Args>
  inline void emplace(Args&&... args)
  {
    emplace_back(std::forward<Args>(args)...);
    std::push_heap(begin(), end(), [](const OpenState& a, const OpenState& b) { return a.f > b.f; });
  }

  inline PlannerState pop()
  {
    std::pop_heap(begin(), end(), [](const OpenState& a, const OpenState& b) { return a.f > b.f; });
    PlannerState result( std::move(back()) );
    pop_back();
    return result;
  }
};

} // Planning
} // Anki


#endif // _ANKICORETECH_PLANNING_PLANNER_STATES_H_
