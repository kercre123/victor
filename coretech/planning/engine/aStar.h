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
#include <functional>

namespace Anki {


// please read https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern to understand the template here
//    We use a template type for the implentation to allow for compile time polymorphism of an abstract class
//    to avoid the overhead of virtual functions. 
template<class T, class Impl>
class IAStarConfig {
public:
  struct Successor { T state; float cost; };

  // we use `decltype(auto)` here to allow for custom iterable types if wanted
  decltype(auto) GetSuccessors(const T& p) const { return static_cast<Impl const&>(*this).GetSuccessors(p); }
  float          Heuristic(const T& p)     const { return static_cast<Impl const&>(*this).Heuristic(p); }
  bool           IsGoal(const T& p)        const { return static_cast<Impl const&>(*this).IsGoal(p); }
  bool           StopPlanning()                  { return static_cast<Impl&>(*this).StopPlanning(); }

private:
  // use private constructor and friend class to prevent accidentally templating on wrong derived class
  IAStarConfig(){};
  friend Impl;
};

template<class T, class ConfigT>
class AStar {
public:

  // Construct A* planner
  AStar(ConfigT& config) : _config(config), _open(1024), _closed(1024, StateHasher, StateEqual) {}

  // initialization, search loop, and plan construction of classical A* implementation
  std::vector<T> Search(const std::vector<T>& start);

private:  
  // given a state in the closed list, follow the backpointers to the start state for that branch
  std::vector<T> GetPlan(const T& currState);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Data Containers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // record for sorting open and closed lists
  struct Record {
    T state;
    T parent;
    float g;
    float f; // f = g + h
  };

  // Closed List
  std::function<size_t (const Record&)> StateHasher = [] (const Record& s) { 
    return std::hash<T>{}(s.state); 
  };

  std::function<bool (const Record&, const Record&)> StateEqual = [] (const Record& s, const Record& t) { 
    return std::equal_to<T>{}(s.state, t.state); 
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
      std::pop_heap(this->begin(), this->end(), [](const Record& a, const Record& b) { return (a.f > b.f); });
      Record result( std::move(this->back()) );
      this->pop_back();
      return result;
    }
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Data Members
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  IAStarConfig<T, ConfigT>& _config;
  OpenList                  _open;
  ClosedList                _closed;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class T, class ConfigT>
inline std::vector<T> AStar<T, ConfigT>::Search(const std::vector<T>& start)
{
  // clear search sets if we already ran the planner
  _open.clear();
  _closed.clear();

  // seed the open list with startStates
  for (const auto& p : start) {
    _open.emplace( {p, p, 0, _config.Heuristic(p)} );
  }

  bool foundGoal = false;
  typename ClosedList::iterator current = _closed.begin();

  // search loop
  while( !_config.StopPlanning() && !foundGoal && !_open.empty() )  {
    const auto& closedRecord = _closed.emplace( std::move(_open.pop()) );

    // if the insert was successful, expand
    if ( closedRecord.second ) {
      current = closedRecord.first;
      foundGoal = _config.IsGoal(current->state);

      using Iter = typename IAStarConfig<T, ConfigT>::Successor;
      for (const Iter& succ : _config.GetSuccessors(current->state) ) {
        float newCost = current->g + succ.cost;
        _open.emplace( { succ.state, current->state, newCost, newCost + _config.Heuristic(succ.state)} );
      }
    }
  }

  return ( foundGoal && (current != _closed.end()) ) ? GetPlan(current->state) : std::vector<T>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class T, class ConfigT>
inline std::vector<T> AStar<T, ConfigT>::GetPlan(const T& currState)
{
  std::vector<T> out;
  auto next = _closed.find({currState, currState, 0, 0});

  while ( next != _closed.end() ) {
    out.push_back(next->state);
    if ( NEAR_ZERO(next->g) ) { break; }
    next = _closed.find({next->parent, next->parent, 0, 0});
  }

  std::reverse(out.begin(), out.end());
  return out;
}

} // namespace Anki


#endif // __Coretech_Planning_AStar_H__