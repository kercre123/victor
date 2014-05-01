/**
 * File: stateTable.h
 *
 * Author: Brad Neuman
 * Created: 2014-04-30
 *
 * Description: Table of entries for each expanded state
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKICORETECH_PLANNING_STATE_TABLE_H_
#define _ANKICORETECH_PLANNING_STATE_TABLE_H_

#include<map>
#include "anki/planning/basestation/xythetaEnvironment.h"
#include "openList.h"
// TODO:(bn) pull out basic defined from environment, so we don't need the whole thing

namespace Anki
{
namespace Planning
{

struct StateEntry
{
  StateEntry(OpenList::iterator openIt,
                 StateID backpointer,
                 ActionID backpointerAction,
                 Cost g) :
    openIt_(openIt),
    closedIter_(-1),
    backpointer_(backpointer),
    backpointerAction_(backpointerAction),
    g_(g)
    {
    }

  // Check if we are closed on the given search iteration
  bool IsClosed(short currAraIter) const { return currAraIter == closedIter_; }

  OpenList::iterator openIt_;
  int closedIter_;

  // Previous state on the path
  StateID backpointer_;
  ActionID backpointerAction_;

  // TODO:(bn) think hard about if I actually need this or not
  Cost g_;
};

class StateTable
{
public:

  StateTable();

  void Clear();

  // First argument must be a StateID, followed by all arguments for
  // the StateEntry constructor
  template<class... Args>
  void emplace(Args&&... args);

private:
  std::map<StateID, StateEntry> table_;
};

template<class... Args>
void StateTable::emplace(Args&&... args)
{
  // TODO:(bn) double check that this is correct
  table_.emplace(std::forward<Args>(args)...);
}


}
}

#endif
