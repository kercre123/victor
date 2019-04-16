/**
 * File: gazeDirection.cpp
 *
 * Author: Robert Cosgriff
 * Date:   11/26/2018
 *
 * Description: see header
 *
 * Copyright: Anki, Inc. 2018
 **/
#include "gazeDirection.h"

#include "coretech/common/shared/types.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vision {

namespace {
  CONSOLE_VAR(u32, kGazeDirectionHistorySize,                 "Vision.GazeDirection",  6);
  CONSOLE_VAR(u32, kGazeDirectionMinNumberOfInliers,          "Vision.GazeDirection",  2);
  CONSOLE_VAR(u32, kGazeDirectionExpireThreshold_ms,          "Vision.GazeDirection",  1000);
  CONSOLE_VAR(f32, kGazeDirectionInlierXThreshold_mm,         "Vision.GazeDirection",  300.f);
  CONSOLE_VAR(f32, kGazeDirectionInlierYThreshold_mm,         "Vision.GazeDirection",  100.f);
  CONSOLE_VAR(f32, kGazeDirectionInlierZThreshold_mm,         "Vision.GazeDirection",  20.f);
  CONSOLE_VAR(f32, kGazeDirectionShiftOutputPointX_mm,        "Vision.GazeDirection",  0.0f);//100.f);
  // This value was chosen to be sufficiently large that the difference in the z coordinates
  // of the two points used to find the intersection with the ground plane weren't too close
  // as to cause numerical instabilities. 500 was too small.
  CONSOLE_VAR(f32, kGazeDirectionSecondPointTranslationY_mm,  "Vision.GazeDirection",  1500.f);
}

GazeDirection::GazeDirection()
  : _gazeDirectionAverage(Point3f(GazeDirectionData::kDefaultDistance_mm,
                                  GazeDirectionData::kDefaultDistance_mm,
                                  GazeDirectionData::kDefaultDistance_mm))
{
  _gazeDirectionHistory.resize(kGazeDirectionHistorySize);
}

void GazeDirection::Update(const TrackedFace& face, const Pose3d& robotPose)
{
  _headPose = face.GetHeadPose();
  _lastUpdated = face.GetTimeStamp();

  if (_currentIndex > (kGazeDirectionHistorySize - 1)) {
    // Make sure we place a "real" value
    // in every element of the history
    // before returning a result
    _initialized = true;
    _currentIndex = 0;
  }

  Point3f gazeDirectionPoint;
  const bool include = GetPointFromHeadPose(_headPose, robotPose, gazeDirectionPoint);
  _gazeDirectionHistory[_currentIndex].Update(gazeDirectionPoint, include);

  // This computations only need to happen once we are initialized because
  // otherwise we're not going to output a point as stable
  if (_initialized) {
    _gazeDirectionAverage = ComputeEntireGazeDirectionAverage();
    _numberOfInliers = FindInliers(_gazeDirectionAverage);
    _gazeDirectionAverage = RecomputeGazeDirectionAverage();
  }

  _currentIndex++;
}

Point3f GazeDirection::ComputeEntireGazeDirectionAverage()
{
  return ComputeGazeDirectionAverage(false);
}

Point3f GazeDirection::ComputeGazeDirectionAverage(const bool filterOutliers)
{
  Point3f averageGazeDirection = Point3f(0.f, 0.f, 0.f);

  // Find the average, these are Cartesian
  // coordinates. However we only want to include
  // those coordinates that intersected the ground plane.
  u32 pointsInAverage = 0;
  for (const auto& gazeDirection: _gazeDirectionHistory) {
    if (gazeDirection.include) {
      if (filterOutliers) {
        if (gazeDirection.inlier) {
          pointsInAverage++;
          averageGazeDirection += gazeDirection.point;
        }
      } else {
        pointsInAverage++;
        averageGazeDirection += gazeDirection.point;
      }
    }
  }

  if (pointsInAverage != 0) {
    averageGazeDirection *= 1.f/pointsInAverage;
  } else {
    averageGazeDirection = Point3f(GazeDirectionData::kDefaultDistance_mm,
                                   GazeDirectionData::kDefaultDistance_mm,
                                   GazeDirectionData::kDefaultDistance_mm);
  }
  return averageGazeDirection;
}

int GazeDirection::FindInliers(const Point3f& gazeDirectionAverage)
{
  // Find the inliers given the average, and only include
  // points that have intersected with the ground plane.
  // Here inliners are determined using a l-1 distance.
  int numberOfInliers = 0;
  for (auto& gazeDirection: _gazeDirectionHistory) {

    if (!gazeDirection.include) {
      continue;
    }
                      
    auto difference = gazeDirection.point - gazeDirectionAverage;
    if (std::abs(difference.x()) < kGazeDirectionInlierXThreshold_mm &&
        std::abs(difference.y()) < kGazeDirectionInlierYThreshold_mm &&
        std::abs(difference.z()) < kGazeDirectionInlierZThreshold_mm) {
      gazeDirection.inlier = true;
      numberOfInliers++;
    } else {
      gazeDirection.inlier = false;
    }
  }
  return numberOfInliers;
}

Point3f GazeDirection::RecomputeGazeDirectionAverage()
{
  return ComputeGazeDirectionAverage(true);
}

bool GazeDirection::GetExpired(const TimeStamp_t currentTime) const
{
  return (currentTime - _lastUpdated) > kGazeDirectionExpireThreshold_ms;
}
  
// returns true if the plane/line are not orthogonal, and if a ray starting from from in the direction of (from, to) intersects (i.e., the line segment is extended in one direction infinitely [or rather until we have numerical issues])
bool IntersectPlaneLine( const Vec3f& planeA, const float planeB, const Point3f& from, const Point3f& to, Point3f& intersection)
{
  const Vec3f dline = to - from;
  
  // assuming line and plane intersect:
  // line for t in R
  //  x_ = from + t*dline
  // plane:
  //  planeA .* x_ = planeB
  // intersection:
  //  planeA .* (from + t*dline) = planeB
  //  ==> planeA .* from  +  t * planeA.*line = planeB
  //  ==> t = (planeB - planeA.*from) / (planeA.*line)
  //  ==> x_ = from + t*dline
  
  auto dot = [](const Vec3f& a, const Vec3f& b) -> float {
    return a.x()*b.x() + a.y()*b.y() + a.z()*b.z();
  };
  
  // check not parallel
  const float dotVal = dot(planeA,dline);
  if( fabsf(dotVal) < 1e-4f ) {
    return false;
  }
  
  const float t = (planeB - dot(planeA, from)) / dotVal;
  intersection = from + dline*t;
  PRINT_NAMED_WARNING("ALLOWNOW2", "t=%f", t);
  return (t >= 0.0f);
}

bool GazeDirection::GetPointFromHeadPose(const Pose3d& headPose, const Pose3d& robotPose, Point3f& gazeDirectionPoint)
{
  // intersect ray from headPose forward with robot's x=0 plane:
  
  // make line
  const Vec3f from = headPose.GetTranslation();
  // face Y hat is out the back of the head. anti parallel to nose axis
  Vec3f offset(0.0f, -kGazeDirectionSecondPointTranslationY_mm, 0.0f);
  Rotation3d rot = Rotation3d(0.f, Z_AXIS_3D());
  const Vec3f to = (headPose.GetTransform() * Transform3d(rot, offset)).GetTranslation();
  
  // make plane:
  // consider vectors {robot y hat} and {global zhat}, which are orth to the robot xhat, call it w.
  const auto& yhat = robotPose.GetRotation() * Y_AXIS_3D();
  // also consider vector v = {x-x0, y-y0, z-z0} for x,y,z in R.
  // Since this is on the plane formed by {robot y hat} x {global z hat}, w .* v = 0, so
  // yhat.y()*x - yhat.x()*y + (from.y()*yhat.x() - from.x()*yhat.y()) = 0
  // ==> A^T x = b:
  const Vec3f planeA = {yhat.y(), -yhat.x(), 0.0f};
  const float planeB = -(robotPose.GetTranslation().y()*yhat.x() - robotPose.GetTranslation().x()*yhat.y());
  
  Point3f intersect = {0.0f, 0.0f, 0.0f};
  
  bool intersects = IntersectPlaneLine( planeA, planeB, from, to, intersect );
  if( intersects ) {
    // get relative to robot pose
    gazeDirectionPoint = intersect - robotPose.GetTranslation();
    ANKI_VERIFY( fabsf(gazeDirectionPoint.x()) < 50.0, "WHATNOW", "");
  }
  
  PRINT_NAMED_WARNING("WHATNOW",
                      "A={%1.3f,%1.3f,%1.3f}, b=%1.3f   from={%1.3f,%1.3f,%1.3f}, to={%1.3f,%1.3f,%1.3f} int %d {%1.3f,%1.3f,%1.3f} gdp={%1.3f,%1.3f,%1.3f}",
                      planeA.x(), planeA.y(), planeA.z(),
                      planeB,
                      from.x(), from.y(), from.z(),
                      to.x(), to.y(), to.z(),
                      intersects,
                      intersect.x(), intersect.y(), intersect.z(),
                      gazeDirectionPoint.x(), gazeDirectionPoint.y(), gazeDirectionPoint.z());
  
  // allow it event if the intersection point has z<0 because of noise
  return intersects;
  
  
//  // Get another point in the direction of the rotation of the head pose, to then find the
//  // intersection of that line with ground plane, note that this point is in world coordinates
//  // "behind" the head if the pose were looking at the ground plane if the y translation is
//  // positive.
//  Pose3d translatedPose(0.f, Z_AXIS_3D(), {0.f, kGazeDirectionSecondPointTranslationY_mm, 0.f}, headPose);
//  const auto& point = translatedPose.GetWithRespectToRoot().GetTranslation();
//  const auto& translation = headPose.GetTranslation();
//
//  if (Util::IsFltNear(translation.z(), point.z())) {
//    gazeDirectionPoint = point;
//    return false;
//  } else {
//    // This is adopting the definition of a line l as the set
//    // { P_1 * alpha + P_2 * (1-alpha) | alpha in R}
//    // and the points (including the zero vector) are
//    // in R^3. P_1 is the position of the head (translation) and P_2 is
//    // position of the point in the direction of the head pose's rotation
//    // (point).
//    const float alpha = ( -point.z() ) / ( translation.z() - point.z() );
//
//    // Alpha less than zero means that the ground plane point we found is "behind"
//    // the head pose. For some reason the head pose rotation is such
//    // that the face normal is pointed "above" the horizon. If alpha > 0 the point is
//    // either in the line segment between our two points (0 < alpha < 1) or in the ray
//    // starting at the second point and extending in the same direction as the segment.
//    // This avoids finding a intersection with the ground plane when face normal is
//    // directed above the horizon.
//    if (FLT_GT(alpha, 1.f)) {
//      gazeDirectionPoint = translation * alpha + point * (1 - alpha);
//      return true;
//    } else {
//      gazeDirectionPoint = point;
//      return false;
//    }
//  }
}

bool GazeDirection::IsStable() const
{
  return ( (_numberOfInliers > kGazeDirectionMinNumberOfInliers) && _initialized );
}

Point3f GazeDirection::GetGazeDirectionAverage() const
{
  return _gazeDirectionAverage + Point3f(kGazeDirectionShiftOutputPointX_mm, 0.f, 0.f);
}

Point3f GazeDirection::GetCurrentGazeDirection() const
{
  return _gazeDirectionHistory[_currentIndex].point + Point3f(kGazeDirectionShiftOutputPointX_mm, 0.f, 0.f);
}

void GazeDirection::ClearHistory()
{
  _initialized = false;
  for (auto& gazeDirection: _gazeDirectionHistory) {
      gazeDirection.point.x() = GazeDirectionData::kDefaultDistance_mm;
      gazeDirection.point.y() = GazeDirectionData::kDefaultDistance_mm;
      gazeDirection.point.z() = GazeDirectionData::kDefaultDistance_mm;
      gazeDirection.inlier = false;
      gazeDirection.include  = false;
  }
}

} // namespace Vision
} // namespace Anki
