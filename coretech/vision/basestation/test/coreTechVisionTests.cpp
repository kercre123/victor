#include "gtest/gtest.h"

#include "anki/common/shared/radians.h"
#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "anki/vision/basestation/camera.h"
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
  
} // TestRotation:Rotation2dNegativeAngle

