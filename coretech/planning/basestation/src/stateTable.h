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
      // printf("running argument constructor\n");
    }

  StateEntry(const StateEntry& lval) :
    openIt_(lval.openIt_),
    closedIter_(lval.closedIter_),
    backpointer_(lval.backpointer_),
    backpointerAction_(lval.backpointerAction_),
    g_(lval.g_)
    {
      // printf("running lval copy constructor\n");
    }

  StateEntry(StateEntry&& rval) :
    openIt_(rval.openIt_),
    closedIter_(rval.closedIter_),
    backpointer_(rval.backpointer_),
    backpointerAction_(rval.backpointerAction_),
    g_(rval.g_)
    {
      // printf("running rval copy constructor\n");
    }    

  StateEntry() :
    closedIter_(-1),
    g_(-1.0)
    {
      printf("WARNING: default StateEntry constructor called. This is a performance bug\n");
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
  typedef std::map<StateID, StateEntry>::iterator iterator;
  typedef std::map<StateID, StateEntry>::const_iterator const_iterator;

  StateTable();

  void Clear();

  iterator find(StateID sid);
  const_iterator end() const;

  StateEntry& operator[](const StateID& sid);

  // First argument must be a StateID, followed by all arguments for
  // the StateEntry constructor
  void emplace(StateID sid,
                   OpenList::iterator openIt,
                   StateID backpointer,
                   ActionID backpointerAction,
                   Cost g);

private:
  std::map<StateID, StateEntry> table_;
};


}
}

#endif
