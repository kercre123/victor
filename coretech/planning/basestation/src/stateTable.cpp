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


#include "stateTable.h"

namespace Anki
{
namespace Planning
{

StateTable::StateTable()
{
}

void StateTable::Clear()
{
  table_.clear();
}

void StateTable::emplace(StateID sid,
                             OpenList::iterator openIt,
                             StateID backpointer,
                             ActionID backpointerAction,
                             Cost g)
{
  // this version of implace uses the argument constructor for
  // StateEntry, so only one state entry is ever created, no copying
  table_.emplace(std::piecewise_construct,
                     std::make_tuple(sid),
                     std::make_tuple(openIt,
                                         backpointer,
                                         backpointerAction,
                                         g));
}

}
}
