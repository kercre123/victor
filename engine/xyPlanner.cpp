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
#include "engine/xyPlannerConfig.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "coretech/planning/engine/geometryHelpers.h"

#include <chrono>


namespace Anki {
namespace Cozmo {

namespace {  
  // priority list for turn radius
  const std::vector<float> arcRadii = { 100., 70., 30., 10. };

  // minimum precision for joining path segments
  const float kPathPrecisionTolerance = .1f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  XYPlanner
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
XYPlanner::XYPlanner(Robot* robot)
: IPathPlanner("XYPlanner")
, _map(robot->GetMapComponent())
, _start()
, _targets()
, _collisionPenalty(0.f) {}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EComputePathStatus XYPlanner::ComputeNewPathIfNeeded(const Pose3d& startPose, bool forceReplanFromScratch, bool allowGoalChange)
{
  if ( !forceReplanFromScratch && FLT_LE(GetPathCollisionPenalty( _path ), _collisionPenalty) ) {
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
  _collisionPenalty = 0.f;

  // expand out of collision state if necessary
  // NOTE: if no safe point exists, the A* search will timeout, but we probably have bigger problems to deal with.
  //       Why can we not find a single safe point anywhere within the searchable range of EscapeObstaclePlanner?
  const Point2f plannerGoal = FindNearestSafePoint(_start.GetTranslation());

  using namespace std::chrono;
  high_resolution_clock::time_point startTime = high_resolution_clock::now();

  AStar<Point2f, PlannerConfig> planner( (PlannerConfig(_map, plannerGoal)) );
  std::vector<Point2f> plan = planner.Search(plannerStart);

  high_resolution_clock::time_point planTime = high_resolution_clock::now();

  if(!plan.empty()) {

    plan.push_back(_start.GetTranslation()); // planner will only go to the nearest safe grid point, so add the real start to shortcut escape obstacle path
    std::reverse(plan.begin(), plan.end());  // planner runs from goal->start, so reverse it before building path

    _path = BuildPath( plan );
    _collisionPenalty = GetPathCollisionPenalty( _path );
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

namespace {
  // Point turns are just collisions with the spherical robot
  inline Ball2f GetPointCollisionSet(const Point2f& a, float padding) {
    return Ball2f(a, kRobotRadius_mm + padding);
  }

  // straight lines are rectangles the length of the line segment, with robot width
  inline FastPolygon GetLineCollisionSet(const LineSegment& l, float padding) {
    Point2f normal(l.GetFrom().y()-l.GetTo().y(), l.GetTo().x()-l.GetFrom().x());
    normal.MakeUnitLength();
    float width = kRobotRadius_mm + padding;

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

    const int nChecks = std::ceil(ABS(sweepAngle.ToFloat() * b.GetRadius()) / kRobotRadius_mm);
    const float checkLen = sweepAngle.ToFloat() / nChecks;

    std::vector<Ball2f> retv;
    for (int i = 0; i <= nChecks; ++i) {
      f32 rad = startAngle.ToFloat() + i*checkLen;
      retv.emplace_back(b.GetCentroid() + Point2f(std::cos(rad), std::sin(rad))*(kRobotRadius_mm + padding), kRobotRadius_mm + padding);
    }

    return retv;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline float XYPlanner::GetArcPenalty(const Arc& arc, float padding) const
{
  const auto disks = GetArcCollisionSet(arc, padding);
  return std::accumulate(disks.begin(), disks.end(), 0.f, 
    [this] (float cost, const auto& disk) { return cost + _map.GetCollisionArea(disk); }
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool XYPlanner::IsArcSafe(const Arc& arc, float padding) const
{
  const auto disks = GetArcCollisionSet(arc, padding);
  return std::none_of(disks.begin(), disks.end(), [this] (const auto& disk) { return _map.CheckForCollisions(disk); });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline float XYPlanner::GetLinePenalty(const LineSegment& seg, float padding) const
{
  return _map.GetCollisionArea( GetLineCollisionSet(seg, padding) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool XYPlanner::IsLineSafe(const LineSegment& seg, float padding) const
{
  return !_map.CheckForCollisions( GetLineCollisionSet(seg, padding) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline float XYPlanner::GetPointPenalty(const Point2f& p, float padding) const
{
  return _map.GetCollisionArea( GetPointCollisionSet(p, padding) );
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
  const auto isSafe = [this] (const Planning::PathSegment& seg ) -> bool {
    switch (seg.GetType()) {
      case Planning::PST_POINT_TURN: { return IsPointSafe( CreatePoint2f(seg), 0. ); }
      case Planning::PST_LINE:       { return IsLineSafe( CreateLineSegment(seg), 0.); }
      case Planning::PST_ARC:        { return IsArcSafe( CreateArc(seg), 0.f ); }
      default:                       { return true; }
    }
  };

  for (int i = 0; i < path.GetNumSegments(); ++i) { 
    if ( !isSafe(path.GetSegmentConstRef(i)) ) { return false; }
  } 
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float XYPlanner::GetPathCollisionPenalty(const Planning::Path& path) const 
{
  const auto getCost = [this] (const Planning::PathSegment& seg ) -> float {
    switch (seg.GetType()) {
      case Planning::PST_POINT_TURN: { return GetPointPenalty( CreatePoint2f(seg), 0. ); }
      case Planning::PST_LINE:       { return GetLinePenalty( CreateLineSegment(seg), 0.); }
      case Planning::PST_ARC:        { return GetArcPenalty( CreateArc(seg), 0.f ); }
      default:                       { return 0.f; }
    }
  };

  float retv = 0.f;
  for (int i = 0; i < path.GetNumSegments(); ++i) { 
    retv += getCost(path.GetSegmentConstRef(i)); 
  } 
  return retv;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Point2f XYPlanner::FindNearestSafePoint(const Point2f& p) const
{
  AStar<Point2f, EscapeObstaclePlanner> planner( (EscapeObstaclePlanner(_map)) );
  std::vector<Point2f> plan = planner.Search({p});

  if (plan.empty()) {
    PRINT_NAMED_WARNING("XYPlanner.FindNearestSafePoint", "Could not find any collision free point near %s", p.ToString().c_str());
  }

  return plan.empty() ? p : plan.back();
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
    const FastPolygon p = GetLineCollisionSet({*iter1, plan[i]}, kPlanningPadding_mm);
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
        safeArc = IsArcSafe(corner, kPlanningPadding_mm);
      } 
      if ( !safeArc && GetCircumscribedArc(pts[i-1], pts[i], pts[i+1], r, corner) ) {
        safeArc = IsArcSafe(corner, kPlanningPadding_mm) &&
                  IsLineSafe({Point2f(x_tail,y_tail), corner.start}, kPlanningPadding_mm) && 
                  IsLineSafe({corner.end, pts[i+1]}, kPlanningPadding_mm);
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
