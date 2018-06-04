/**
 * File: xyPlanner.h
 *
 * Author: Michael Willett
 * Created: 2018-05-11
 *
 * Description: Simple 2d grid uniform planner with path smoothing step
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Victor_Engine_XYPlanner_H__
#define __Victor_Engine_XYPlanner_H__

#include "engine/pathPlanner.h"
#include "coretech/planning/shared/goalDefs.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/pose.h"

#include "util/helpers/noncopyable.h"

#include <initializer_list>
#include <algorithm>
#include <utility>
#include <unordered_set>
#include <vector>

namespace Anki {


namespace Cozmo {

class Path;
class Robot;
class MapComponent;

class XYPlanner : public IPathPlanner, private Util::noncopyable
{
public:

  explicit XYPlanner(Robot* robot);
  virtual ~XYPlanner() override {};

  // ComputePath functions start computation of a path.
  // Return value of Error indicates that there was a problem starting the plan and it isn't running. Running
  // means it is (or may have already finished)
  virtual EComputePathStatus ComputePath(const Pose3d& startPose, const std::vector<Pose3d>& targetPoses) override;

  // While we are following a path, we can do a more efficient check to see if we need to update that path
  // based on new obstacles or other information. 
  virtual EComputePathStatus ComputeNewPathIfNeeded(const Pose3d& startPose,
                                                    bool forceReplanFromScratch = false,
                                                    bool allowGoalChange = true) override;

  virtual void StopPlanning() override {}

  virtual EPlannerStatus CheckPlanningStatus() const override;
  
  
  // Returns true if this planner checks for fatal obstacle collisions
  bool ChecksForCollisions() const override { return true; }
  
  // Returns true if the path avoids obstacles. Some planners don't know about obstacles, so the default is always true.
  // If provided, clears and fills validPath to be that portion of path that is below the max obstacle penalty.
  virtual bool CheckIsPathSafe(const Planning::Path& path, float startAngle) const override;
  virtual bool CheckIsPathSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const override;

  
  // This class maintains a const reference to the NavMap, so there is no need to ever call this method since
  // the map will update itself with obstacles
  virtual bool PreloadObstacles() override { return true; }

  // return a test path
  virtual void GetTestPath(const Pose3d& startPose, Planning::Path &path, const PathMotionProfile* motionProfile = nullptr) override {}
  
protected:

  EComputePathStatus ComputePath(const Pose3d& startPose, const Pose3d& targetPose) override;

  bool GetCompletePath_Internal(const Pose3d& robotPose, Planning::Path &path) override                                { path = _path; return true;}
  bool GetCompletePath_Internal(const Pose3d& robotPose, Planning::Path &path, Planning::GoalID& targetIndex) override { path = _path; return true;}

private:

  struct State {
    Point2f pose;
    Point2f prevPose;
    float g;
    float f; // f = g + h
  };

  class OpenList : private std::vector<State>
  {
  public:
    using std::vector<State>::clear;
    using std::vector<State>::empty;

    inline void emplace(State&& s)
    {
      emplace_back(s);
      std::push_heap(begin(), end(), [](const State& a, const State& b) { return (a.f > b.f); });
    }

    inline State pop()
    {
      std::pop_heap(begin(), end(), [](const State& a, const State& b) { return (a.f > b.f); });
      State result( std::move(back()) );
      pop_back();
      return result;
    }
  };
  
  struct StateHasher
  {
    // hash to integer values since we probably don't care about sub-millimeter precision anyway
    s64 operator()(const State& s) const { return ((s64) s.pose.x()) << 32 | ((s64) s.pose.y()); }
  };
   
  struct StateEqual
  {
    long operator()(const State& s, const State& t) const { return ((s32)s.pose.x() == (s32)t.pose.x()) && ((s32)s.pose.y() == (s32)t.pose.y()); }
  };

  using ClosedList = std::unordered_set<State, StateHasher, StateEqual>;

  // initialization, main loop, and path post processing of classical A* implementation that searches from 
  // all states in _targets to closest planner state to _start
  // TODO: break this in into its own helper class since it plans in reverse
  bool AStar(); 

  // check if the state is goal of the planner (different depending on forward/backward search)
  bool IsGoalState(const State& candidate);

  // returns true if State is in the closed set
  // bool IsClosedState(const State& s);
  bool IsClosedState(const Point2f& p);

  // returns true if the state is one of the target poses we are planning to
  bool IsTargetState(const Point2f& p);

  // Gets the valid successors of the state and inserts them to the open list
  void ExpandState(const State& currState);

  // builds a simplified list of waypoints from closed set
  std::vector<Point2f> GenerateWayPoints(const State& start);

  // convert a set of way points to a smooth path
  Planning::Path BuildPlan(const std::vector<Point2f>& waypoints);

  const MapComponent&  _map;
  Pose2d               _start;
  std::vector<Pose2d>  _targets;
  OpenList             _open;
  ClosedList           _closed;
  EPlannerStatus       _status;
};
    
    
} // namespace Cozmo
} // namespace Anki


#endif // __Victor_Engine_XYPlanner_H__

