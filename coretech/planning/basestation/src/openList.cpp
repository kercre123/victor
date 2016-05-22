/**
 * File: openList.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-04-30
 *
 * Description: open list based on std::map
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "openList.h"

namespace Anki {
namespace Planning {

using namespace std;

multimap<float, StateID> OpenList::emptyMap_;
OpenList::iterator OpenList::nullIterator_ = OpenList::emptyMap_.end();

OpenList::OpenList()
{}

void OpenList::clear()
{
  fMap_.clear();
}

StateID OpenList::top() const
{
  return (*fMap_.begin()).second;
}

float OpenList::topF() const
{
  return (*fMap_.begin()).first;
}

StateID OpenList::pop()
{
  iterator it = fMap_.begin();
  StateID ret = (*it).second;

  fMap_.erase(it);

  return ret;
}

bool OpenList::empty()
{
  return fMap_.empty();
}

unsigned int OpenList::size()
{
  return (unsigned int)fMap_.size();
}

OpenList::iterator OpenList::insert(StateID id, const float f)
{
  return fMap_.insert(pair<float,StateID>(f, id));
}

void OpenList::remove(OpenList::iterator it)
{
  fMap_.erase(it);
}


bool OpenList::contains(StateID id) const
{
  multimap<float, StateID>::const_iterator it;
  for(it=fMap_.begin(); it!=fMap_.end(); ++it) {
    if (it->second == id) {
      return true;
    }
  }
  return false;
}

float OpenList::fVal(StateID id) const
{
  multimap<float, StateID>::const_iterator it;
  for(it=fMap_.begin(); it!=fMap_.end(); ++it) {
    if (it->second == id) {
      return it->first;
    }
  }
  return FLT_MAX;
}

}
}
