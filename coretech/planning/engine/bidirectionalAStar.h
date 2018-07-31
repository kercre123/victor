/**
 * File: bidirectionalAStar.h
 *
 * Author: Michael Willett
 * Created: 2018-07-26
 *
 * Description: Bidirectional AStar implementation for early termination of infeasible paths
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Coretech_Planning_BidirectionalAStar_H__
#define __Coretech_Planning_BidirectionalAStar_H__

#include <unordered_set>
#include <functional>

namespace {
  constexpr u32 const kSearchQueueInitialSize = 1024;
}

namespace Anki {

// please read https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern to understand the template here
//    We use a template type for the implentation to allow for compile time polymorphism of an abstract class
//    to avoid the overhead of virtual functions. 
template<class T, class Impl>
class BidirectionalAStarConfig {
public:
  struct Successor { T state; float cost; };

  // we use `decltype(auto)` here to allow for custom iterable types if wanted
  decltype(auto) GetSuccessors(const T& p)    const { return static_cast<Impl const&>(*this).GetSuccessors(p); }
  float          ForwardHeuristic(const T& p) const { return static_cast<Impl const&>(*this).ForwardHeuristic(p); }
  float          ReverseHeuristic(const T& p) const { return static_cast<Impl const&>(*this).ReverseHeuristic(p); }
  bool           StopPlanning()                     { return static_cast<Impl&>(*this).StopPlanning(); }
  
  const T&              GetStart()            const { return static_cast<Impl const&>(*this).GetStart(); }
  const std::vector<T>& GetGoals()            const { return static_cast<Impl const&>(*this).GetGoals(); }

private:
  // use private constructor and friend class to prevent accidentally templating on wrong derived class
  BidirectionalAStarConfig(){};
  friend Impl;
};

template<class T, class ConfigT>
class BidirectionalAStar {
public:

  // Construct A* planner
  BidirectionalAStar(ConfigT& config) 
  : _config(config)
  , _openForward(kSearchQueueInitialSize)
  , _openReverse(kSearchQueueInitialSize)
  , _closedForward(kSearchQueueInitialSize, StateHasher, StateEqual)
  , _closedReverse(kSearchQueueInitialSize, StateHasher, StateEqual) {}

  // initialization, search loop, and plan construction of classical A* implementation
  std::vector<T> Search();

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

  BidirectionalAStarConfig<T, ConfigT>& _config;
  OpenList           _openForward;
  OpenList           _openReverse;
  ClosedList         _closedForward;
  ClosedList         _closedReverse;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class T, class ConfigT>
inline std::vector<T> BidirectionalAStar<T, ConfigT>::Search()
{
  // clear search sets if we already ran the planner
  _openForward.clear();
  _openReverse.clear();
  _closedForward.clear();
  _closedReverse.clear();

  // seed the forward list with start state
  _openForward.emplace( {_config.GetStart(), _config.GetStart(), 0, _config.ForwardHeuristic(_config.GetStart())} );
  
  // seed the reverse list with goal states
  for (const auto& g : _config.GetGoals()) {
    _openReverse.emplace( {g, g, 0, _config.ReverseHeuristic(g)} );
  }

  bool foundGoal = false;
  typename ClosedList::iterator current;

  // search loop
  bool isForward = true;
  while( !_config.StopPlanning() && !foundGoal && !_openForward.empty() && !_openReverse.empty() )  {

    OpenList&   open        =  isForward ? _openForward   : _openReverse;
    ClosedList& closed      =  isForward ? _closedForward : _closedReverse;
    ClosedList& closedOther = !isForward ? _closedForward : _closedReverse;

    const auto& closedRecord = closed.emplace( std::move(open.pop()) );

    // if the insert was successful, expand
    if ( closedRecord.second ) {
      current = closedRecord.first;
      foundGoal = closedOther.find(*current) != closedOther.end();

      using Iter = typename BidirectionalAStarConfig<T, ConfigT>::Successor;
      for (const Iter& succ : _config.GetSuccessors(current->state) ) {
        float newCost = current->g + succ.cost;
        float f = newCost + (isForward ? _config.ForwardHeuristic(succ.state) : _config.ReverseHeuristic(succ.state));
        open.emplace( { succ.state, current->state, newCost, f} );
      }
    }

    // flip the direction
    isForward = !isForward;
  }

  return ( foundGoal && (current != _closedForward.end()) ) ? GetPlan(current->state) : std::vector<T>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class T, class ConfigT>
inline std::vector<T> BidirectionalAStar<T, ConfigT>::GetPlan(const T& currState)
{
  std::vector<T> out;

  auto next = _closedForward.find({currState, currState, 0, 0});
  while ( next != _closedForward.end() ) {
    out.push_back(next->state);
    if ( NEAR_ZERO(next->g) ) { break; }
    next = _closedForward.find({next->parent, next->parent, 0, 0});
  }
  std::reverse(out.begin(), out.end());
  out.pop_back();

  next = _closedReverse.find({currState, currState, 0, 0});
  while ( next != _closedReverse.end() ) {
    out.push_back(next->state);
    if ( NEAR_ZERO(next->g) ) { break; }
    next = _closedReverse.find({next->parent, next->parent, 0, 0});
  }

  return out;
}

} // namespace Anki


#endif // __Coretech_Planning_BidirectionalAStar_H__