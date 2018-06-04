/**
 * File: xyPlanner.cpp
 *
 * Author: Michael Willett
 * Created: 2018-05-11
 *
 * Description: TODO
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "xyPlanner.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/navMap/mapComponent.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/ball.h"

#define RESOLUTION_MM 20.f
#define ROBOT_RADIUS_MM (ROBOT_BOUNDING_X / 2.f)

namespace Anki {
namespace Cozmo {

XYPlanner::XYPlanner(Robot* robot)
: IPathPlanner("XYPlanner")
, _map(robot->GetMapComponent())
, _start()
, _targets()
, _open()   // TODO: reserve?
, _closed() {}

namespace {
  const float kDiagScale = std::sqrt(2.f)-1.f;

  inline float distance_octile(const Point2f& s, const Point2f& g) {
    Point2f d = (s - g).Abs();
    return (std::fmax(d.x(), d.y()) + kDiagScale * std::fmin(d.x(), d.y()));
  }

  inline Poly2f MakeStraightRegion(Point2f a, Point2f b) {
    // TODO: add robot radius to start and end of path?
    Point2f normal(a.y()-b.y(), b.x()-a.x());
    normal.MakeUnitLength();
    // TODO: replace the .9 multiple with a custom padding for path collision checking vs planning collision checking
    return Poly2f({ a + normal * ROBOT_RADIUS_MM * .9, 
                    b + normal * ROBOT_RADIUS_MM * .9, 
                    b - normal * ROBOT_RADIUS_MM * .9, 
                    a - normal * ROBOT_RADIUS_MM * .9 });
  }

  const std::vector<Point2f> fourConnectedGrid = { 
    { RESOLUTION_MM, 0.f},
    {-RESOLUTION_MM, 0.f},
    { 0.f, -RESOLUTION_MM},
    { 0.f,  RESOLUTION_MM}
  };

  const std::vector<Point2f> eightConnectedGrid = { 
    { RESOLUTION_MM, 0.f},
    {-RESOLUTION_MM, 0.f},
    { 0.f,  RESOLUTION_MM},
    { 0.f, -RESOLUTION_MM},
    { RESOLUTION_MM, RESOLUTION_MM},
    {-RESOLUTION_MM, RESOLUTION_MM},
    { RESOLUTION_MM, -RESOLUTION_MM},
    {-RESOLUTION_MM, -RESOLUTION_MM}
  };
}

EPlannerStatus XYPlanner::CheckPlanningStatus() const 
{
  return _status;
}

bool XYPlanner::CheckIsPathSafe(const Planning::Path& path, float startAngle) const 
{
  Planning::Path ignore;
  return CheckIsPathSafe(path, startAngle, ignore);
}

bool XYPlanner::CheckIsPathSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const 
{
  for (int i = 0; i < path.GetNumSegments(); ++i) {
    const Planning::PathSegment& seg = path.GetSegmentConstRef(i);
    switch (seg.GetType()) {
      case Planning::PST_LINE:
      {
        float x0, y0, x1, y1, theta;
        seg.GetStartPoint(x0,y0);
        seg.GetEndPose(x1,y1,theta);

        // build quad to check robot width
        const bool collision = _map.CheckForCollisions( MakeStraightRegion({x0,y0}, {x1,y1}) );
        if (collision) { return false; }
      }
      case Planning::PST_ARC:
      case Planning::PST_POINT_TURN:
        // TODO: actually check collisions for arcs...
        validPath.AppendSegment(seg);
        break;
      default:
        break;
    }
  } 
  return true;
}

EComputePathStatus XYPlanner::ComputeNewPathIfNeeded(const Pose3d& startPose, bool forceReplanFromScratch, bool allowGoalChange)
{
  // TODO: should we check check if the current robot pose is too far from the path, or trust that the path
  // follower code will fail and allow the behavior manager to replan?
  if ( CheckIsPathSafe(_path, 0.f) && !forceReplanFromScratch ) {
    return EComputePathStatus::NoPlanNeeded;
  }
  // rerun planner
  _path.Clear();
  _start = startPose;

  const bool success = AStar();
  
  _status = success ? EPlannerStatus::CompleteWithPlan : EPlannerStatus::CompleteNoPlan;
  return success ? EComputePathStatus::Running : EComputePathStatus::Error;
}

EComputePathStatus XYPlanner::ComputePath(const Pose3d& startPose, const Pose3d& targetPose) 
{ 
  std::vector<Pose3d> targets = {targetPose};
  return ComputePath(startPose, targets); 
}

EComputePathStatus XYPlanner::ComputePath(const Pose3d& startPose, const std::vector<Pose3d>& targetPoses)
{
  _start = startPose;
  _targets.clear();
  std::copy(targetPoses.begin(), targetPoses.end(), _targets.begin());

  const bool success = AStar();
  if (success) {
    PRINT_NAMED_WARNING("xyPlanner.ComputePath", "generated path of length %d with %zu expansions", _path.GetNumSegments(), _closed.size() );
  } 

  _status = success ? EPlannerStatus::CompleteWithPlan : EPlannerStatus::CompleteNoPlan;
  return success ? EComputePathStatus::Running : EComputePathStatus::Error;
}

bool XYPlanner::AStar()
{
  _open.clear();
  _closed.clear();

  // push goal states
  for (const auto& g : _targets) {
    // NOTE: this does not enforce that we are snapping to the planner resolution, so each target can spawn a whole
    // independant search tree...
    // TODO: this should get pose relative to the origin in case we rejigger in the middle of executing a path
    Point2f gt = g.GetTranslation();
    _open.emplace( {gt, gt, 0, distance_octile(gt, _start.GetTranslation())} );
  }

  bool foundGoal = false;
  State current;

  // search loop
  // TODO: make this a bounded number of expansions to prevent planning to infinitely far away points
  while( !foundGoal && !_open.empty() )  {
    current = _open.pop();
    foundGoal = IsGoalState(current);

    // if the insert was successful, expand
    const auto& it = _closed.emplace( current );
    if ( it.second ) {
      ExpandState( *(it.first) );
    }
  }

  if(foundGoal) {    
    _path = BuildPlan( GenerateWayPoints(current) );
  }

  return foundGoal;
}

inline bool XYPlanner::IsGoalState(const State& candidate)
{
  // within one grid cell of start state, so assume no colisions
  return (candidate.pose - _start.GetTranslation()).LengthSq() <= (2*RESOLUTION_MM*RESOLUTION_MM);
}

inline bool XYPlanner::IsClosedState(const Point2f& p)
{
  return (_closed.find({p,p,0,0}) != _closed.end());
}

inline bool XYPlanner::IsTargetState(const Point2f& p)
{
  for ( const auto& t : _targets) {
    if ( (p - t.GetTranslation()).LengthSq() <= (2*RESOLUTION_MM*RESOLUTION_MM) ) {
      return true;
    }
  }
  return false;
}

inline void XYPlanner::ExpandState(const State& currState)
{ 
  for (const Point2f& p: fourConnectedGrid) {
    Point2f nextPose  = p + currState.pose;
    float   actionLen = p.Length();

    if ( !IsClosedState(nextPose) && !_map.CheckForCollisions(Ball2f(nextPose, ROBOT_RADIUS_MM)) ) {
      float g = currState.g + actionLen;
      State nextState = { nextPose, currState.pose, g, g + distance_octile(nextPose, _start.GetTranslation()) };
      _open.emplace( std::move(nextState) );
    }
  }
}

std::vector<Point2f> XYPlanner::GenerateWayPoints(const State& s) 
{
  std::vector<Point2f> out;

  // start by adding the continuous point _start and the last state from the search
  out.push_back(_start.GetTranslation());
  out.push_back(s.pose);

  auto next = _closed.find({s.pose, s.pose, 0, 0});

  while ( next->g > 0 ) {
    // get the successor from closed list
    next = _closed.find({next->prevPose, next->prevPose, 0, 0});
    Poly2f p = MakeStraightRegion(*(out.end()-2), next->pose);
    
    // if no collion, so replace the last point in the waypoints
    if (!_map.CheckForCollisions(p)) {
      out.pop_back();
    }
    out.push_back(next->pose);
  }

  return out;
}

Planning::Path XYPlanner::BuildPlan(const std::vector<Point2f>& waypoints)
{  
  Planning::Path path;
  
  Radians lastHeading = _start.GetAngle();
  Radians currHeading;

  for (int i = 1; i < waypoints.size(); ++i) 
  {
    currHeading = atan2(waypoints[i].y() - waypoints[i-1].y(), waypoints[i].x() - waypoints[i-1].x());
    path.AppendPointTurn(waypoints[i-1].x(), waypoints[i-1].y(), lastHeading.ToFloat(), currHeading.ToFloat(), 1.f, 1.f, 1.f, .1f, true );
    path.AppendLine( waypoints[i-1].x(),  waypoints[i-1].y(), waypoints[i].x(), waypoints[i].y(), 1.f, 1.f, 1.f);
    lastHeading = currHeading;
  }
  return path;
}

} // Cozmo
} // Anki