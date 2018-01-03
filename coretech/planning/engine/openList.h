/**
 * File: openList.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-04-30
 *
 * Description: open list based on std::multimap
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKICORETECH_PLANNING_OPENLIST_H_
#define _ANKICORETECH_PLANNING_OPENLIST_H_

#include "coretech/planning/engine/xythetaEnvironment.h"
#include <map>

namespace Anki {
namespace Planning {

class OpenList
{

public:

  typedef std::multimap<float, StateID>::iterator iterator;

  OpenList();

  // Clears the open list
  void clear();

  // Return the first element
  StateID top() const;

  // Returns the f value of the first element
  float topF() const;

  // Remove and return the first element
  StateID pop();

  bool empty();

  unsigned int size();

  // Inserts a new entry into the open list. Do not call this if there
  // is already an entry associated with the state
  iterator insert(StateID stateID, const float f);

  // Returns a "null iterator" that will always be consistent
  static iterator nullIterator() {return nullIterator_;}

  // removes an iterator.
  void remove(iterator it);

  // allows iteration through the internal map
  std::multimap<float, StateID>::iterator begin() { return fMap_.begin(); };
  std::multimap<float, StateID>::iterator end() { return fMap_.end(); };

  // const versions
  std::multimap<float, StateID>::const_iterator begin() const { return fMap_.begin(); };
  std::multimap<float, StateID>::const_iterator end() const { return fMap_.end(); };

  // Returns true if the state is contained in the open list. warning:
  // these are both linear time!! don't call these
  bool contains(StateID id) const;

  // returns very high number if id is not in this list, otherwise
  // returns the f value. Linear time function!
  float fVal(StateID id) const;
  
private:

  // Multimap is internally stored as a min heap
  // This multimap maps from a given f value to a state ID
  std::multimap<float, StateID> fMap_;

  // empty map that is used to obtain a valid "null iterator" to
  // signify an empty slot
  static std::multimap<float, StateID> emptyMap_;

  // the null iterator itself
  static iterator nullIterator_;
};

}

}


#endif // BASESTATION_PLANNING_OPENLIST_H_
