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
#include "coretech/common/engine/math/point.h"
#include "coretech/common/engine/math/pose.h"

#include "util/helpers/noncopyable.h"

#include <initializer_list>
#include <algorithm>
#include <utility>
#include <unordered_set>
#include <vector>

namespace Anki {

struct Arc;
class LineSegment;

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

  virtual EPlannerStatus CheckPlanningStatus() const override { return _status; }
  
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

  bool GetCompletePath_Internal(const Pose3d& robotPose, Planning::Path &path) override;
  bool GetCompletePath_Internal(const Pose3d& robotPose, Planning::Path &path, Planning::GoalID& targetIndex) override;

private:
  // convert a set of way points to a smooth path
  Planning::Path BuildPath(const std::vector<Point2f>& plan) const;

  // builds a simplified list of waypoints from closed set
  std::vector<Point2f> GenerateWayPoints(const std::vector<Point2f>& plan) const;

  // given a set of points, generate the largest safe circumscibed arc for each turn
  std::vector<Planning::PathSegment> SmoothCorners(const std::vector<Point2f>& pts) const;

  // if p corresponds to a pose in _targets, return the correspondance index. if it is not, returns _targets.size()
  Planning::GoalID FindGoalIndex(const Point2f& p) const;

  // returns true if the provided path segment is safe. Since PathSegments are unions, it assumes
  // the caller of this method has already determined the segment type.
  bool IsArcSegmentSafe(const Planning::PathSegment& p) const;
  bool IsLineSegmentSafe(const Planning::PathSegment& p) const;
  bool IsPointSegmentSafe(const Planning::PathSegment& p) const;

  bool IsArcSafe(const Arc& a, float padding) const;
  bool IsLineSafe(const LineSegment& l, float padding) const;
  bool IsPointSafe(const Point2f& p, float padding) const;

  const MapComponent&  _map;
  Pose2d               _start;
  std::vector<Pose2d>  _targets;
  EPlannerStatus       _status;
};
    
    
} // namespace Cozmo
} // namespace Anki


#endif // __Victor_Engine_XYPlanner_H__

