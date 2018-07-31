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
#include "coretech/planning/engine/bidirectionalAStar.h"

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
  const size_t kPlanPathMaxExpansions       = 50000;

  const std::vector<Point2f> connectedGrid = { 
    { kPlanningResolution_mm, 0.f},
    {-kPlanningResolution_mm, 0.f},
    { 0.f, -kPlanningResolution_mm},
    { 0.f,  kPlanningResolution_mm}
  };
  
  // NOTE: the escape grid resolution needs to be the same as the planner Resolution, otherwise it will
  //       generate invalid goal positions 
  const std::vector<Point2f> escapeGrid = { 
    { kPlanningResolution_mm, 0.f},
    {-kPlanningResolution_mm, 0.f},
    { 0.f, -kPlanningResolution_mm},
    { 0.f,  kPlanningResolution_mm},
    { kPlanningResolution_mm,  kPlanningResolution_mm},
    {-kPlanningResolution_mm,  kPlanningResolution_mm},
    { kPlanningResolution_mm, -kPlanningResolution_mm},
    {-kPlanningResolution_mm, -kPlanningResolution_mm}
  };
  
  // snap to planning grid via (float->int->float) cast
  inline Point2f GetNearestGridPoint(const Point2f& p) {
    Point2f gridPt = p * kOneOverPlanningResolution;
    return Point2f(roundf(gridPt.x()), roundf(gridPt.y())) * kPlanningResolution_mm;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  Bidirectional A* Configuration through collision free space
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class PlannerConfig : public BidirectionalAStarConfig<Point2f, PlannerConfig> {
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
    SuccessorIter end()                                { return SuccessorIter(_parent, _map, connectedGrid.size()); }

  private:
    inline void UpdateState() {
      if (_idx >= connectedGrid.size()) { return; }
      _state = _parent + connectedGrid[_idx];
      if ( _map.CheckForCollisions( Ball2f(_state, kRobotRadius_mm + kPlanningPadding_mm) ) ) { ++(*this); }
    }

    size_t              _idx;
    Point2f             _state;
    const Point2f       _parent;
    const MapComponent& _map;
  };

  PlannerConfig(const Point2f& start, const std::vector<Point2f>& goals, const MapComponent& map, const volatile bool& stopPlanning) 
  : _start(start)
  , _goals(goals)
  , _map(map)
  , _abort(stopPlanning) {}

  inline bool   StopPlanning()                               { return _abort || (++_numExpansions > kPlanPathMaxExpansions); }
  inline size_t GetNumExpansions() const                     { return _numExpansions; }
  inline SuccessorIter GetSuccessors(const Point2f& p) const { return SuccessorIter(GetNearestGridPoint(p), _map); };
  
  inline float ReverseHeuristic(const Point2f& p) const { return ManhattanDistance(p, _start); };
  inline float ForwardHeuristic(const Point2f& p) const { 
    float minDist = std::numeric_limits<float>::max();
    for (const auto& g : _goals) {
      minDist = fmin(minDist, (ManhattanDistance(p, g)));
    }
    return minDist;
  };


  const Point2f&              GetStart() const { return _start; }
  const std::vector<Point2f>& GetGoals() const { return _goals; }

private:
  inline float ManhattanDistance(const Point2f& p, const Point2f& q) const {
    Point2f d = (p - q).Abs();
    return d.x() + d.y(); 
  }

  const Point2f&              _start;
  const std::vector<Point2f>& _goals;
  const MapComponent&         _map;
  const volatile bool&        _abort;
  size_t                      _numExpansions = 0;
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

  inline std::array<Successor, 8> GetSuccessors(const Point2f& p) const { 
    std::array<Successor, 8> retv;
    const Point2f gridP = GetNearestGridPoint(p);
    std::transform(escapeGrid.begin(), escapeGrid.end(), retv.begin(), 
      [&gridP](const auto& dir) { return Successor{gridP + dir, dir.Length()}; });
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