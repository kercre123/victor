#include "gtest/gtest.h"

#include "anki/common/shared/radians.h"
#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/observableObject.h"
#include "anki/vision/basestation/perspectivePoseEstimation.h"

using namespace Anki;

GTEST_TEST(PoseEstimation, SolveQuartic)
{
#define PRECISION float
  const std::array<PRECISION,5> factors = {
    {-3593989.0, -33048.973667, 316991.744900, 33048.734165, -235.623396}
  };
  
  const std::array<PRECISION,4> roots_groundTruth = {
    {0.334683441970975, 0.006699578943935, -0.136720934135068, -0.213857711381642}
  };
  
  std::array<PRECISION,4> roots_computed;
  EXPECT_TRUE(Vision::P3P::solveQuartic(factors, roots_computed) == RESULT_OK);
  
  for(s32 i=0; i<4; ++i) {
    EXPECT_NEAR(roots_groundTruth[i], roots_computed[i], 1e-6f);
  }
  
#undef PRECISION
} // GTEST_TEST(PoseEstimation, SolveQuartic)

GTEST_TEST(PoseEstimation, FromQuads)
{
  // Parameters
  const Radians Xangle = DEG_TO_RAD(-10);
  const Radians Yangle = DEG_TO_RAD( 4);
  const Radians Zangle = DEG_TO_RAD( 3);

  const Point3f translation(10.f, 15.f, 100.f);
  
  const f32 markerSize = 26.f;
  
  const f32 focalLength_x = 317.2f;
  const f32 focalLength_y = 318.4f;
  const f32 camCenter_x   = 151.9f;
  const f32 camCenter_y   = 129.0f;
  const u16 camNumRows    = 240;
  const u16 camNumCols    = 320;
  
  const Quad2f projNoise(Point2f(0.1740f,    0.0116f),
                         Point2f(0.0041f,    0.0073f),
                         Point2f(0.0381f,    0.1436f),
                         Point2f(0.2249f,    0.0851f));
  /*
  const Quad2f projNoise(Point2f(-0.0310f,    0.1679f),
                         Point2f( 0.3724f,   -0.3019f),
                         Point2f( 0.3523f,    0.1793f),
                         Point2f( 0.3543f,    0.4076f));
  */
  
  const f32 distThreshold      = 3.f;
  const Radians angleThreshold = DEG_TO_RAD(2);
  const f32 pixelErrThreshold  = 1.f;
  
  // Set up the true pose
  Pose3d poseTrue;
  poseTrue.rotateBy(RotationVector3d(Xangle, X_AXIS_3D));
  poseTrue.rotateBy(RotationVector3d(Yangle, Y_AXIS_3D));
  poseTrue.rotateBy(RotationVector3d(Zangle, Z_AXIS_3D));
  poseTrue.set_translation(translation);
  
  // Create the 3D marker and put it in the specified pose relative to the camera
  Quad3f marker3d(Point3f(-1.f, -1.f, 0.f),
                  Point3f(-1.f,  1.f, 0.f),
                  Point3f( 1.f, -1.f, 0.f),
                  Point3f( 1.f,  1.f, 0.f));
  
  marker3d *= markerSize/2.f;
  
  Quad3f marker3d_atPose;
  poseTrue.applyTo(marker3d, marker3d_atPose);
  
  // Compute the ground truth projection of the marker in the image
  Vision::CameraCalibration calib(camNumRows,    camNumCols,
                                  focalLength_x, focalLength_y,
                                  camCenter_x,   camCenter_y,
                                  0.f);           // skew

  Vision::Camera camera;
  camera.set_calibration(calib);
  
  Quad2f proj;
  camera.Project3dPoints(marker3d_atPose, proj);
  
  // Add noise
  proj += projNoise;
  
  // Make sure all the corners projected within the image
  for(Quad::CornerName i_corner=Quad::FirstCorner; i_corner<Quad::NumCorners; ++i_corner)
  {
    ASSERT_TRUE(not std::isnan(proj[i_corner].x()));
    ASSERT_TRUE(not std::isnan(proj[i_corner].y()));
    ASSERT_GE(proj[i_corner].x(), 0.f);
    ASSERT_LT(proj[i_corner].x(), camNumCols);
    ASSERT_GE(proj[i_corner].y(), 0.f);
    ASSERT_LT(proj[i_corner].y(), camNumRows);
  }

  // Compute the pose of the marker w.r.t. camera from the noisy projection
  Pose3d poseEst = camera.ComputeObjectPose(proj, marker3d);
  
  // Check if the estimated pose matches the true pose
  Pose3d poseDiff;
  EXPECT_TRUE(poseEst.IsSameAs(poseTrue, distThreshold, angleThreshold, poseDiff));

  printf("Angular difference is %f degrees (threshold = %f degrees)\n",
         poseDiff.get_rotationMatrix().GetAngle().getDegrees(),
         angleThreshold.getDegrees());
  
  printf("Translation difference is %f, threshold = %f\n",
         poseDiff.get_translation().Length(),  distThreshold);
  
  
  // Check if the reprojected points match the originals
  Quad2f reproj;
  Quad3f marker3d_est;
  poseEst.applyTo(marker3d, marker3d_est);
  camera.Project3dPoints(marker3d_est, reproj);
  for(Quad::CornerName i_corner=Quad::FirstCorner; i_corner<Quad::NumCorners; ++i_corner) {
    EXPECT_NEAR(reproj[i_corner].x(), proj[i_corner].x(), pixelErrThreshold);
    EXPECT_NEAR(reproj[i_corner].y(), proj[i_corner].y(), pixelErrThreshold);
  }
  
} // GTEST_TEST(PoseEstimation, FromQuads)


GTEST_TEST(Camera, VisibilityAndOcclusion)
{
  // Create a camera looking at several objects, check to see that expected
  // objects are visible / occluded
  
  // Create a very simple test object derived from (abstract) ObservableObject.
  // It's just a quad with a single marker on it, the same size as the object
  // itself.
  class TestObject : public Vision::ObservableObject
  {
  public:
    TestObject(const f32 size, const Vision::Marker::Code& withCode, const Pose3d& atPose)
    : _size(size)
    , _marker( AddMarker(withCode, Pose3d(), _size) )
    , _quad( _marker.Get3dCorners() )
    {
      SetPose(atPose);
    }
    
    TestObject(const TestObject& other)
    : TestObject(_size, _marker.GetCode(), other.GetPose())
    {
    
    }
    
    virtual std::vector<RotationMatrix3d> const& GetRotationAmbiguities() const override
    {
      return _EmptyRotationAmbiguities;
    }
    
    virtual TestObject* Clone() const override
    {
      // Call the copy constructor
      return new TestObject(*this);
    }
    
    virtual void GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const override
    {
      using namespace Quad;
      corners.clear();
      for(CornerName iCorner = FirstCorner; iCorner != NumCorners; ++iCorner) {
        corners.emplace_back(atPose * _quad[iCorner]);
      }
    }
    
  protected:
    const f32                   _size;
    const Vision::KnownMarker&  _marker;
    const Quad3f                _quad;
    
    const std::vector<RotationMatrix3d> _EmptyRotationAmbiguities;
    
  }; // class TestObject
  
  const Pose3d camPose(0.f, Z_AXIS_3D, {{0.f, 0.f, 0.f}});
  
  const Vision::CameraCalibration calib(240, 320,
                                  300.f, 300.f,
                                  160.f, 120.f);
  
  Vision::Camera camera(0, calib, camPose);
  
  // Note that object pose is in camera coordinates
  const Pose3d obj1Pose(M_PI_2, X_AXIS_3D, {{0.f, 0.f, 100.f}});
  TestObject object1(15.f, 0, obj1Pose);

  camera.AddOccluder(&object1);
  
  // For readability below:
  const bool RequireObjectBehind      = true;
  const bool DoNotRequireObjectBehind = false;
  
  // Without any occluders, we expect this object, positioned right in front
  // of the camera, to be visible from the camera -- if we don't require
  // there to be an object behind it to say it is visible.
  EXPECT_TRUE( object1.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, DoNotRequireObjectBehind) );
  
  // Same check, but now we require there to be an object behind the object
  // to consider it visible.  There are no other objects, so we can't have seen
  // anything behind this object, so we want this to return false.
  EXPECT_FALSE( object1.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, RequireObjectBehind) );
  
  // Add another object behind first, again in camera coordinates
  const Pose3d obj2Pose(M_PI_2, X_AXIS_3D, {{0.f, 0.f, 150.f}});
  TestObject object2(20.f, 0, obj2Pose);
  
  camera.AddOccluder(&object2);
  
  // Now, object2 is behind object 1...
  // ... we expect object2 not to be visible, irrespective of the
  // "requireObjectBehind" flag, because it is occlued by object1.
  EXPECT_FALSE( object2.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, RequireObjectBehind) );
  EXPECT_FALSE( object2.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, DoNotRequireObjectBehind) );
  
  // ... and we now _do_ expect to see object1 when requireObjectBehind is true,
  // unlike above.
  EXPECT_TRUE( object1.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, RequireObjectBehind) );
  
  // If we clear the occluders, object2 should also be visible, now that it is
  // not occluded, but only if we do not require there to be an object behind
  // it
  camera.ClearOccluders();
  EXPECT_TRUE( object2.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, DoNotRequireObjectBehind) );
  EXPECT_FALSE( object2.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, RequireObjectBehind) );
  
  // Move object1 around and make sure it is NOT visible when...
  // ...it is not facing the camera
  object1.SetPose(Pose3d(M_PI_4, X_AXIS_3D, {{0.f, 0.f, 100.f}}));
  EXPECT_FALSE( object1.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, DoNotRequireObjectBehind) );
  object1.SetPose(Pose3d(0, X_AXIS_3D, {{0.f, 0.f, 100.f}}));
  EXPECT_FALSE( object1.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, DoNotRequireObjectBehind) );
  
  // ...it is too far away (and thus too small in the image)
  object1.SetPose(Pose3d(3*M_PI_2, X_AXIS_3D, {{0.f, 0.f, 1000.f}}));
  EXPECT_FALSE( object1.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, DoNotRequireObjectBehind) );
  
  // ...it is not within the field of view
  object1.SetPose(Pose3d(M_PI_2, X_AXIS_3D, {{100.f, 0.f, 100.f}}));
  EXPECT_FALSE( object1.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, DoNotRequireObjectBehind) );
  
  // ...it is behind the camera
  object1.SetPose(Pose3d(M_PI_2, X_AXIS_3D, {{100.f, 0.f, -100.f}}));
  EXPECT_FALSE( object1.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, DoNotRequireObjectBehind) );

} // GTEST_TEST(Camera, VisibilityAndOcclusion)
