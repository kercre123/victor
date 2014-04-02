#include "gtest/gtest.h"

#include "anki/common/shared/radians.h"
#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/vision/basestation/camera.h"

using namespace Anki;

GTEST_TEST(TestPoseEstimation, FromQuads)
{
  // Parameters
  const Radians Xangle = DEG_TO_RAD(-5);
  const Radians Yangle = DEG_TO_RAD( 4);
  const Radians Zangle = DEG_TO_RAD( 3);

  const Point3f translation(10.f, 12.f, 120.f);
  
  const f32 markerSize = 26.f;
  
  const f32 focalLength_x = 317.2f;
  const f32 focalLength_y = 318.4f;
  const f32 camCenter_x   = 151.9f;
  const f32 camCenter_y   = 129.0f;
  const u16 camNumRows    = 240;
  const u16 camNumCols    = 320;
  
  const Quad2f projNoise(Point2f(-0.0310f,    0.1679f),
                         Point2f( 0.3724f,   -0.3019f),
                         Point2f( 0.3523f,    0.1793f),
                         Point2f( 0.3543f,    0.4076f));
  
  const f32 distThreshold      = 2.f;
  const Radians angleThreshold = DEG_TO_RAD(1);
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
  
  marker3d *= markerSize;
  
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
  camera.project3dPoints(marker3d_atPose, proj);
  
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
  Pose3d poseEst = camera.computeObjectPose(proj, marker3d);
  
  // Check if the estimated pose matches the true pose
  printf("Angular difference is %f degrees (threshold = %f degrees)\n",
         poseEst.get_rotationMatrix().GetAngleDiffFrom(poseTrue.get_rotationMatrix()).getDegrees(),
         angleThreshold.getDegrees());
  printf("Translation difference is (%f, %f, %f), threshold = %f\n",
         poseEst.get_translation().x() - poseTrue.get_translation().x(),
         poseEst.get_translation().y() - poseTrue.get_translation().y(),
         poseEst.get_translation().z() - poseTrue.get_translation().z(),
         distThreshold);
  
  EXPECT_TRUE(poseEst.IsSameAs(poseTrue, distThreshold, angleThreshold));
  
  // Check if the reprojected points match the originals
  Quad2f reproj;
  Quad3f marker3d_est;
  poseEst.applyTo(marker3d, marker3d_est);
  camera.project3dPoints(marker3d_est, reproj);
  for(Quad::CornerName i_corner=Quad::FirstCorner; i_corner<Quad::NumCorners; ++i_corner) {
    EXPECT_NEAR(reproj[i_corner].x(), proj[i_corner].x(), pixelErrThreshold);
    EXPECT_NEAR(reproj[i_corner].y(), proj[i_corner].y(), pixelErrThreshold);
  }
  
} // TestRotation:Rotation2dNegativeAngle

