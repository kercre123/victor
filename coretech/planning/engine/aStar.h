/**
 * File: aStar.h
 *
 * Author: Michael Willett
 * Created: 2018-06-04
 *
 * Description: Minimal AStar implementation
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Coretech_Planning_AStar_H__
#define __Coretech_Planning_AStar_H__

#include <unordered_set>
#include <vector>

namespace Anki {

template<class T>
class IAStarConfig {
public:
  struct Successor { T state; float cost; };
  using SuccessorList = std::vector< Successor >;

  virtual float         Heuristic(const T& p, const T& q)  const = 0;
  virtual bool          Equal(const T& p, const T& q)      const = 0;
  virtual SuccessorList GetSuccessors(const T& p)          const = 0; 
  virtual s64           Hash(const T& p)                   const = 0; 
  virtual size_t        GetMaxExpansions()                 const = 0;
};

template<class T>
class AStar {
public:

  // Construct A* planner
  AStar(const IAStarConfig<T>& config) : _config(config), _open(1024), _closed(1024, StateHasher, StateEqual) {}

  // initialization, search loop, and plan construction of classical A* implementation
  std::vector<T> Search(const std::vector<T>& start, const T& goal); 

private:  
  // record for sorting open and closed lists
  struct Record {
    T state;
    T parent;
    float g;
    float f; // f = g + h
  };

  // given a state in the closed list, follow the backpointers to the start state for that branch
  std::vector<T> GetPlan(const Record& currState);
  
  // Closed List
  std::function<s64 (const Record&)> StateHasher = [this] (const Record& s) { 
    return _config.Hash(s.state); 
  };

  std::function<bool (const Record&, const Record&)> StateEqual = [this] (const Record& s, const Record& t) { 
    return _config.Equal(s.state, t.state); 
  };

  using ClosedList = std::unordered_set<Record, decltype(StateHasher), decltype(StateEqual)>;

  // Open List
  class OpenList : private std::vector<Record>
  {
  public:
    using std::vector<Record>::clear;
    using std::vector<Record>::empty;

    OpenList(size_t reserve) : std::vector<Record>(reserve) {}

    inline void emplace(Record&& s)
    {
      this->emplace_back(s);
      std::push_heap(this->begin(), this->end(), [](const Record& a, const Record& b) { return (a.f > b.f); });
    }

    inline Record pop()
    {
      std::pop_heap(this->begin(), this->end(), 
      [](const Record& a, const Record& b) { return (a.f > b.f); });
      Record result( std::move(this->back()) );
      this->pop_back();
      return result;
    }
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Data Members
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  const IAStarConfig<T>& _config;
  OpenList               _open;
  ClosedList             _closed;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class T>
inline std::vector<T> AStar<T>::Search(const std::vector<T>& start, const T& goal)
{
  // clear search sets if we already ran the planner
  _open.clear();
  _closed.clear();

  // seed the open list with startStates
  for (const auto& p : start) {
    _open.emplace( {p, p, 0, _config.Heuristic(p, goal)} );
  }

  bool foundGoal = false;
  typename ClosedList::iterator current = _closed.begin();

  // search loop
  size_t numExpansions = 0;
  while( (++numExpansions < _config.GetMaxExpansions()) && !foundGoal && !_open.empty() )  {
    const auto& closedRecord = _closed.emplace( std::move(_open.pop()) );

    // if the insert was successful, expand
    if ( closedRecord.second ) {
      current = closedRecord.first;
      foundGoal = _config.Equal(current->state, goal);

      for (const auto& succ : _config.GetSuccessors(current->state)) {
        float newCost = current->g + succ.cost;
        _open.emplace( {succ.state, current->state, newCost, newCost + _config.Heuristic(succ.state, goal)} );
      }
    }
  }

  return ( foundGoal && (current != _closed.end()) ) ? GetPlan(*current) : std::vector<T>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class T>
inline std::vector<T> AStar<T>::GetPlan(const Record& currState) 
{
  std::vector<T> out;
  auto next = _closed.find({currState.state, currState.state, 0, 0});

  while ( next != _closed.end() ) {
    out.push_back(next->state);
    if ( NEAR_ZERO(next->g) ) { break; }
    next = _closed.find({next->parent, next->parent, 0, 0});
  }

  return out;
}

} // namespace Anki


#endif // __Coretech_Planning_AStar_H__