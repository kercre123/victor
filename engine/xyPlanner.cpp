/**
 * File: xyPlanner.cpp
 *
 * Author: Michael Willett
 * Created: 2018-05-11
 *
 * Description: Simple 2d grid uniform planner with path smoothing step
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/xyPlanner.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/navMap/mapComponent.h"

#include "coretech/planning/engine/aStar.h"
#include "coretech/planning/engine/geometryHelpers.h"

#include <chrono>

#define RESOLUTION_MM 20.f
#define ROBOT_RADIUS_MM (ROBOT_BOUNDING_X / 2.f)
#define PLANNING_PADDING_MM 2.f

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  A* Configuration for 2d - 4 connected grid with disk checks
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {  
  // definition for GetSuccessors in PlannerConfig
  const std::vector<Point2f> fourConnectedGrid = { 
    { RESOLUTION_MM, 0.f},
    {-RESOLUTION_MM, 0.f},
    { 0.f, -RESOLUTION_MM},
    { 0.f,  RESOLUTION_MM}
  };

  // priority list for turn radius
  const std::vector<float> arcRadii = { 100., 70., 30., 10. };

  // minimum precision for joining path segments
  const float kPathPrecisionTolerance = .1f;
}

class PlannerConfig : public IAStarConfig<Point2f> {
public:
  PlannerConfig(const MapComponent& map) : _map(map) {}

  // if point is within 1/2 planner resolution step
  inline virtual bool Equal(const Point2f& p, const Point2f& q) const override
  { 
    Point2f d = (p - q).Abs();
    return FLT_LT(d.x(), RESOLUTION_MM/2) && FLT_LT(d.y(), RESOLUTION_MM/2);
  };

  // Manhattan Distance
  inline virtual float Heuristic(const Point2f& p, const Point2f& q) const override 
  {
    Point2f d = (p - q).Abs();
    return (d.x() + d.y());
  };

  class PointIter : public SuccessorIter {
  public:
    PointIter(const Point2f& p, const MapComponent& m) 
    : _idx(0), _parent(p), _map(m) 
    {     
      _state = _parent + fourConnectedGrid[0];
      if ( IsUnsafe() ) { Next(); } 
    }

    inline virtual const Point2f& GetState() const override { return _state; } 
    inline virtual        float GetCost()    const override { return RESOLUTION_MM; } 

    inline virtual bool Done() const override { return (_idx >= fourConnectedGrid.size()); };
    inline virtual void Next() override {
      if (++_idx < fourConnectedGrid.size()) { 
        _state = _parent + fourConnectedGrid[_idx];
        if ( IsUnsafe() ) { Next(); }
      } 
    }

  private:
    inline bool IsUnsafe() const {
      return _map.CheckForCollisions( Ball2f(_state, ROBOT_RADIUS_MM + PLANNING_PADDING_MM) );
    }

    size_t              _idx;
    Point2f             _state;
    const Point2f       _parent;
    const MapComponent& _map;
  };

  // Check for collisions and return if not obstructed
  inline virtual std::unique_ptr<SuccessorIter> GetSuccessors(const Point2f& p) const override
  { 
    return std::make_unique<PointIter>(GetNearestGridPoint(p), _map);
  };

  // hash function for sorting states
  inline virtual s64 Hash(const Point2f& p) const override
  {
    return ((s64) p.x()) << 32 | ((s64) p.y()); 
  };

  // max number of state expansions before terminating
  inline virtual size_t GetMaxExpansions() const override { return 100000; }

private:
  // snap to planning grid
  inline Point2f GetNearestGridPoint(const Point2f& p) const {
    Point2f nearestCenterIdx = p * (1 / RESOLUTION_MM);

    // truncate via (float->int->float) cast instead of round for speed
    return nearestCenterIdx.CastTo<int>().CastTo<float>() * RESOLUTION_MM;
  }

  const MapComponent& _map;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  Helpers to create ConvexPointSets for collision detection
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
  // Point turns are just collisions with the spherical robot
  inline Ball2f GetPointCollisionSet(const Point2f& a, float padding) {
    return Ball2f(a, ROBOT_RADIUS_MM + padding);
  }

  // straight lines are rectangles the length of the line segment, with robot width
  inline FastPolygon GetLineCollisionSet(const LineSegment& l, float padding) {
    Point2f normal(l.GetFrom().y()-l.GetTo().y(), l.GetTo().x()-l.GetFrom().x());
    normal.MakeUnitLength();
    float width = ROBOT_RADIUS_MM + padding;

    return FastPolygon({ l.GetFrom() + normal * width, 
                         l.GetTo() + normal * width,
                         l.GetTo() - normal * width,
                         l.GetFrom() - normal * width });
  }

  // for simplicity, check if arcs are safe using multiple disk checks
  inline std::vector<Ball2f> GetArcCollisionSet(const Arc& a, float padding) {
    // convert to Ball2f to get center and radius
    Ball2f b = ArcToBall(a);

    // calculate start and sweep angles
    Vec2f startVec   = a.start - b.GetCentroid();
    Vec2f endVec     = a.end - b.GetCentroid();
    Radians startAngle( std::atan2(startVec.y(), startVec.x()) );
    Radians sweepAngle( std::atan2(endVec.y(), endVec.x()) - startAngle );

    const int nChecks = std::ceil(ABS(sweepAngle.ToFloat() * b.GetRadius()) / RESOLUTION_MM);
    const float checkLen = sweepAngle.ToFloat() / nChecks;

    std::vector<Ball2f> retv;
    for (int i = 0; i <= nChecks; ++i) {
      f32 rad = startAngle.ToFloat() + i*checkLen;
      retv.emplace_back(b.GetCentroid() + Point2f(std::cos(rad), std::sin(rad))*(ROBOT_RADIUS_MM + padding), ROBOT_RADIUS_MM + padding);
    }

    return retv;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  XYPlanner
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
XYPlanner::XYPlanner(Robot* robot)
: IPathPlanner("XYPlanner")
, _map(robot->GetMapComponent())
, _start()
, _targets() {}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EComputePathStatus XYPlanner::ComputeNewPathIfNeeded(const Pose3d& startPose, bool forceReplanFromScratch, bool allowGoalChange)
{
  if ( !forceReplanFromScratch && CheckIsPathSafe(_path, 0.f) ) {
    return EComputePathStatus::NoPlanNeeded;
  }

  // convert targets to planner states
  std::vector<Point2f> plannerStart;
  if (allowGoalChange || (_path.GetNumSegments() == 0)) {
    for (const auto& g : _targets) {
      plannerStart.push_back( g.GetTranslation() );
    }
  } else {
    // no goal change, so use the end point of the last computed path
    float x, y, t;
    _path[_path.GetNumSegments()-1].GetEndPose(x,y,t);
    plannerStart.emplace_back(x,y);
  }

  _start = startPose;

  using namespace std::chrono;

  high_resolution_clock::time_point startTime = high_resolution_clock::now();

  AStar<Point2f, PlannerConfig> planner( (PlannerConfig(_map)) );
  std::vector<Point2f> plan = planner.Search(plannerStart, _start.GetTranslation());

  high_resolution_clock::time_point planTime = high_resolution_clock::now();

  if(!plan.empty()) {
    _path = BuildPath( plan );
    _status = EPlannerStatus::CompleteWithPlan;
  } else {
    PRINT_NAMED_WARNING("XYPlanner.ComputeNewPathIfNeeded", "No path found!");
    _status = EPlannerStatus::CompleteNoPlan;
  }

  auto smoothTime_ms = duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - planTime);
  auto planTime_ms = duration_cast<std::chrono::milliseconds>(planTime - startTime);
  PRINT_NAMED_INFO("XYPlanner.ComputeNewPathIfNeeded", "planning took %s ms, smoothing took %s ms", 
                    std::to_string(planTime_ms.count()).c_str(), 
                    std::to_string(smoothTime_ms.count()).c_str() );
  
  return (plan.empty()) ? EComputePathStatus::Error : EComputePathStatus::Running;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EComputePathStatus XYPlanner::ComputePath(const Pose3d& startPose, const Pose3d& targetPose) 
{ 
  _path.Clear();
  std::vector<Pose3d> targets = {targetPose};
  return ComputePath(startPose, targets); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EComputePathStatus XYPlanner::ComputePath(const Pose3d& startPose, const std::vector<Pose3d>& targetPoses)
{
  _path.Clear();
  _targets.clear();
  for (const auto t: targetPoses) { _targets.push_back(t); }
  
  return ComputeNewPathIfNeeded(startPose, true, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool XYPlanner::GetCompletePath_Internal(const Pose3d& robotPose, Planning::Path &path)
{ 
  path = _path; 
  return (_path.GetNumSegments() > 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool XYPlanner::GetCompletePath_Internal(const Pose3d& robotPose, Planning::Path &path, Planning::GoalID& targetIndex)
{ 
  path = _path; 
  float x, y, t;
  _path[_path.GetNumSegments()-1].GetEndPose(x,y,t);
  targetIndex = FindGoalIndex({x,y});
  return (_path.GetNumSegments() > 0) && (targetIndex < _targets.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline Planning::GoalID XYPlanner::FindGoalIndex(const Point2f& p) const
{
  const auto isPose = [&p](const Pose2d& t) { return IsNearlyEqual(p, t.GetTranslation()); };
  return std::find_if(_targets.begin(), _targets.end(), isPose) - _targets.begin();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  Collision Detection
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline bool XYPlanner::IsArcSegmentSafe(const Planning::PathSegment& s) const
{
  return IsArcSafe( CreateArc(s), 0.f );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool XYPlanner::IsArcSafe(const Arc& a, float padding) const
{
  const auto disks = GetArcCollisionSet(a, padding);
  return std::none_of(disks.begin(), disks.end(), [this] (const auto& b) { return _map.CheckForCollisions(b); });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool XYPlanner::IsLineSegmentSafe(const Planning::PathSegment& s) const
{
  return IsLineSafe( CreateLineSegment(s), 0.);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool XYPlanner::IsLineSafe(const LineSegment& l, float padding) const
{
  return !_map.CheckForCollisions( GetLineCollisionSet(l, padding) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool XYPlanner::IsPointSegmentSafe(const Planning::PathSegment& s) const
{
  return IsPointSafe( CreatePoint2f(s), 0. );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool XYPlanner::IsPointSafe(const Point2f& p, float padding) const
{
  return !_map.CheckForCollisions( GetPointCollisionSet(p, padding) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool XYPlanner::CheckIsPathSafe(const Planning::Path& path, float startAngle) const 
{
  Planning::Path ignore;
  return CheckIsPathSafe(path, startAngle, ignore);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool XYPlanner::CheckIsPathSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const 
{
  // for now, always return the empty path. If we reuse part of the existing path, it tends to create a lot
  // of point turns for the first couple of segments
  for (int i = 0; i < path.GetNumSegments(); ++i) {
    const Planning::PathSegment& seg = path.GetSegmentConstRef(i);
    switch (seg.GetType()) {
      case Planning::PST_LINE:
      {
        if ( !IsLineSegmentSafe(seg) )  { return false; }
        break;
      }
      case Planning::PST_ARC:
      {
        if ( !IsArcSegmentSafe(seg) )   { return false; }
        break;
      }
      case Planning::PST_POINT_TURN:
      {
        if ( !IsPointSegmentSafe(seg) ) { return false; }
        break;
      }
      default:
        // error?
        break;
    }
  } 
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  Path Smoothing Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Planning::Path XYPlanner::BuildPath(const std::vector<Point2f>& plan) const
{  
  // return empty path if there are no waypoints
  if (plan.size() == 0) {
    return Planning::Path();
  }

  using namespace Planning;
  Planning::Path path;

  std::vector<PathSegment> turns = SmoothCorners( GenerateWayPoints(plan) );

  // start turn is always a point turn, don't add if it is a small turn
  if (!NEAR(turns.front().GetDef().turn.targetAngle, _start.GetAngle().ToFloat(), kPathPrecisionTolerance)) {
    path.AppendSegment(turns[0]);
  }

  // connect all turns via straight lines and add to path
  float x0, y0, x1, y1, theta;
  for (int i = 1; i < turns.size(); ++i) 
  {
    turns[i-1].GetEndPose(x0,y0,theta);
    turns[i].GetStartPoint(x1,y1);

    // don't add trivial Straight Segements
    if ( (Point2f(x0,y0) - Point2f(x1,y1)).Length() > kPathPrecisionTolerance ) {
      path.AppendSegment( CreateLinePath({{x0,y0}, {x1,y1}}) );
    }

    path.AppendSegment( turns[i] );
  }

  if ( !path.CheckContinuity(.0001) ) {
    PRINT_NAMED_WARNING("XYPlanner.BuildPath", "Path smoother gnereated a non-continuous plan");
  }

  return path;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<Point2f> XYPlanner::GenerateWayPoints(const std::vector<Point2f>& plan) const
{
  std::vector<Point2f> out;

  out.push_back(_start.GetTranslation());
  
  const Point2f* iter1 = &out.front();
  const Point2f* iter2 = iter1;

  for (int i = 0; i < plan.size(); ++i) {
    const FastPolygon p = GetLineCollisionSet({*iter1, plan[i]}, PLANNING_PADDING_MM);
    const bool collision = _map.CheckForCollisions(p);

    if (collision) {
      out.push_back(*iter2);
      iter1 = iter2;
    }
    iter2 = &plan[i];
  }

  // always push the last point
  out.push_back(plan.back());

  return out;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<Planning::PathSegment> XYPlanner::SmoothCorners(const std::vector<Point2f>& pts) const
{
  std::vector<Planning::PathSegment> turns;

  // exit if no corners in defined by the input list
  if (pts.size() < 2) { return turns; }

  // for now, always start and end with a point turn the correct heading. Generating the first/last arc
  // uses different logic since heading angles are constrained, while all intermediate headings are not.

  turns.emplace_back( CreatePointTurnPath(_start, pts[1]) );

  // middle turns
  for (int i = 1; i < pts.size() - 1; ++i) 
  {
    Arc corner;
    bool safeArc = false;
    float x_tail, y_tail, t;
    turns.back().GetEndPose(x_tail,y_tail,t);

    // add the first safe arc that can be constructed for the current waypoints, prioritizing inscribed arcs
    // over circumscribed arcs
    for (const float r : arcRadii) {
      // try inscribed arc first since it is faster, otherwise try circumscibed arc
      if ( GetInscribedArc(pts[i-1], pts[i], pts[i+1], r, corner) ) {
        safeArc = IsArcSafe(corner, PLANNING_PADDING_MM);
      } 
      if ( !safeArc && GetCircumscribedArc(pts[i-1], pts[i], pts[i+1], r, corner) ) {
        safeArc = IsArcSafe(corner, PLANNING_PADDING_MM) &&
                  IsLineSafe({Point2f(x_tail,y_tail), corner.start}, PLANNING_PADDING_MM) && 
                  IsLineSafe({corner.end, pts[i+1]}, PLANNING_PADDING_MM);
      }
      if (safeArc) { break; }
    }

    turns.emplace_back( (safeArc) ? CreateArcPath(corner) : CreatePointTurnPath( {pts[i-1], pts[i], pts[i+1]} ) );
  }

  // add last point turn when at goal pose
  size_t idx = FindGoalIndex(pts.back());
  if (idx < _targets.size()) {
    turns.emplace_back( CreatePointTurnPath(*(pts.end()-2), _targets[idx]) );
  }

  return turns;
}


} // Cozmo
} // Anki