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

StateEntry& StateTable::operator[](const StateID& sid)
{
  return table_[sid];
}

StateTable::iterator StateTable::find(StateID sid)
{
  return table_.find(sid);
}

StateTable::const_iterator StateTable::begin() const
{
  return table_.begin();
}

StateTable::const_iterator StateTable::end() const
{
  return table_.end();
}

void StateTable::emplace(StateID sid,
                         OpenList::iterator openIt,
                         StateID backpointer,
                         ActionID backpointerAction,
                         Cost penalty,
                         Cost g)
{
  // this version of implace uses the argument constructor for
  // StateEntry, so only one state entry is ever created, no copying
  table_.emplace(std::piecewise_construct,
                 std::forward_as_tuple(sid),
                 std::forward_as_tuple(openIt,
                                       backpointer,
                                       backpointerAction,
                                       penalty,
                                       g));
}

}
}
