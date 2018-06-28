/**
 * File: xyPlannerConfig.h
 *
 * Author: Michael Willett
 * Created: 2018-06-22
 *
 * Description: Configurations for A* planners used in xyPlanner
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Victor_Engine_XYPlannerConfig_H__
#define __Victor_Engine_XYPlannerConfig_H__

#include "engine/robot.h"
#include "engine/navMap/mapComponent.h"

#include "coretech/planning/engine/aStar.h"

#include <numeric>

namespace std {
  using namespace Anki;
  template <> struct std::hash<Point2f> {
    s64 operator()(const Point2f& p) const { return ((s64) p.x()) << 32 | ((s64) p.y()); }
  };

  template <> struct std::equal_to<Point2f>  {
    bool operator()(const Point2f& p, const Point2f& q) const { 
      return Anki::IsNearlyEqual(p, q, (float) Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE); 
    }
  };
}

namespace Anki {
namespace Cozmo {

namespace {  
  const float kPlanningResolution_mm        = 20.f;
  const float kOneOverPlanningResolution    = 1/kPlanningResolution_mm;
  const float kRobotRadius_mm               = ROBOT_BOUNDING_X / 2.f;
  const float kPlanningPadding_mm           = 3.f;

  const size_t kEscapeObstacleMaxExpansions = 10000;
  const size_t kPlanPathMaxExpansions       = 100000;

  const std::vector<Point2f> fourConnectedGrid = { 
    { kPlanningResolution_mm, 0.f},
    {-kPlanningResolution_mm, 0.f},
    { 0.f, -kPlanningResolution_mm},
    { 0.f,  kPlanningResolution_mm}
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  A* Configuration through collision free space
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class PlannerConfig : public IAStarConfig<Point2f, PlannerConfig> {
public:
  // define a custom iterator class to avoid dynamic memory allocation
  class SuccessorIter : private std::iterator<std::input_iterator_tag, Successor>{
  public:
    SuccessorIter(const Point2f& p, const MapComponent& m, size_t idx = 0 ) 
    : _idx(idx), _parent(p), _map(m) { UpdateState(); }

    bool          operator!=(const SuccessorIter& rhs) { return this->_idx != rhs._idx; }
    Successor     operator*()                          { return {_state, kPlanningResolution_mm}; }
    SuccessorIter operator++()                         { ++_idx; UpdateState(); return *this; }
    SuccessorIter begin()                              { return SuccessorIter(_parent, _map); }
    SuccessorIter end()                                { return SuccessorIter(_parent, _map, fourConnectedGrid.size()); }

  private:
    inline void UpdateState() {
      if (_idx >= fourConnectedGrid.size()) { return; }
      _state = _parent + fourConnectedGrid[_idx];
      if ( _map.CheckForCollisions( Ball2f(_state, kRobotRadius_mm + kPlanningPadding_mm) ) ) { ++(*this); }
    }

    size_t              _idx;
    Point2f             _state;
    const Point2f       _parent;
    const MapComponent& _map;
  };
  
  PlannerConfig(const MapComponent& map, const volatile bool& stopPlanning, const Point2f& goal) 
  : _map(map)
  , _abort(stopPlanning)
  , _goal(goal) {}

  inline bool          IsGoal(const Point2f& p)        const { return IsNearlyEqual(p, _goal, kPlanningResolution_mm/2); }
  inline SuccessorIter GetSuccessors(const Point2f& p) const { return SuccessorIter(GetNearestGridPoint(p), _map); };
  inline size_t        GetNumExpansions()              const { return _numExpansions; }
  inline bool          StopPlanning()                        { return _abort || (++_numExpansions > kPlanPathMaxExpansions); }
  inline float         Heuristic(const Point2f& p)     const { 
    // Manhattan Distance
    Point2f d = (p - _goal).Abs();
    return d.x() + d.y(); 
  }

private:
  // snap to planning grid via (float->int->float) cast
  inline Point2f GetNearestGridPoint(const Point2f& p) const {
    return (p * kOneOverPlanningResolution).CastTo<int>().CastTo<float>() * kPlanningResolution_mm;
  }

  const MapComponent&  _map;
  const volatile bool& _abort;
  const Point2f&       _goal;
  size_t               _numExpansions = 0;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  Dijkstra Configuration that finds the nearest collision free state with uniform action cost
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class EscapeObstaclePlanner : public IAStarConfig<Point2f, EscapeObstaclePlanner> {
public:
  EscapeObstaclePlanner(const MapComponent& map, const volatile bool& stopPlanning) 
  : _map(map)
  , _abort(stopPlanning) {}

  inline float  Heuristic(const Point2f& p) const { return 0.f; };
  inline bool   StopPlanning()                    { return _abort || (++_numExpansions > kEscapeObstacleMaxExpansions); }
  inline bool   IsGoal(const Point2f& p)    const { return !_map.CheckForCollisions( Ball2f(p, kRobotRadius_mm + kPlanningPadding_mm) ); }

  inline std::array<Successor, 4> GetSuccessors(const Point2f& p) const { 
    std::array<Successor, 4> retv;
    std::transform(fourConnectedGrid.begin(), fourConnectedGrid.end(), retv.begin(), 
      [&p](const auto& dir) { return Successor{p + dir, kPlanningResolution_mm}; });
    return retv;
  };

private:
  const MapComponent&  _map;
  const volatile bool& _abort;
  size_t               _numExpansions = 0;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Victor_Engine_XYPlannerConfig_H__