#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header

#include "anki/common/shared/radians.h"
#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/imageCache.h"
#include "anki/vision/basestation/observableObject.h"
#include "anki/vision/basestation/perspectivePoseEstimation.h"
#include "anki/vision/basestation/profiler.h"

using namespace Anki;

template<typename PRECISION>
void SolveQuarticTestHelper(std::array<PRECISION,4>& roots_computed,
                            const PRECISION tolerance)
{
  const std::array<PRECISION,5> factors = {
    {-3593989.0, -33048.973667, 316991.744900, 33048.734165, -235.623396}
  };
  
  const std::array<PRECISION,4> roots_groundTruth = {
    {0.334683441970975, 0.006699578943935, -0.136720934135068, -0.213857711381642}
  };
  
  EXPECT_TRUE(Vision::P3P::solveQuartic(factors, roots_computed) == RESULT_OK);
  for(s32 i=0; i<4; ++i) {
    EXPECT_NEAR(roots_groundTruth[i], roots_computed[i], tolerance);
  }
}

GTEST_TEST(PoseEstimation, SolveQuartic)
{
  { // Single precision
    std::array<float_t,4> roots_computed;
    SolveQuarticTestHelper(roots_computed, 1e-6f);
  }
  
  { // Double precision
    std::array<double_t,4> roots_computed;
    SolveQuarticTestHelper(roots_computed, 1e-12);
  }
  
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
  const Pose3d origin;
  Pose3d poseTrue;
  poseTrue.SetParent(&origin);
  poseTrue.RotateBy(RotationVector3d(Xangle, X_AXIS_3D()));
  poseTrue.RotateBy(RotationVector3d(Yangle, Y_AXIS_3D()));
  poseTrue.RotateBy(RotationVector3d(Zangle, Z_AXIS_3D()));
  poseTrue.SetTranslation(translation);
  
  // Create the 3D marker and put it in the specified pose relative to the camera
  Quad3f marker3d(Point3f(-1.f, -1.f, 0.f),
                  Point3f(-1.f,  1.f, 0.f),
                  Point3f( 1.f, -1.f, 0.f),
                  Point3f( 1.f,  1.f, 0.f));
  
  marker3d *= markerSize/2.f;
  
  Quad3f marker3d_atPose;
  poseTrue.ApplyTo(marker3d, marker3d_atPose);
  
  // Compute the ground truth projection of the marker in the image
  Vision::CameraCalibration calib(camNumRows,    camNumCols,
                                  focalLength_x, focalLength_y,
                                  camCenter_x,   camCenter_y,
                                  0.f);           // skew

  Pose3d cameraPose;
  cameraPose.SetParent(&origin);
  Vision::Camera camera;
  camera.SetPose(cameraPose);
  camera.SetCalibration(calib);
  
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
  Pose3d poseEst;
  Result result = camera.ComputeObjectPose(proj, marker3d, poseEst);
  EXPECT_EQ(result, RESULT_OK);
  
  // Check if the estimated pose matches the true pose
  Vec3f Tdiff;
  Radians angleDiff;
  EXPECT_TRUE(poseEst.IsSameAs(poseTrue, distThreshold, angleThreshold, Tdiff, angleDiff));

  printf("Angular difference is %f degrees (threshold = %f degrees)\n",
         angleDiff.getDegrees(),
         angleThreshold.getDegrees());
  
  printf("Translation difference is %f, threshold = %f\n",
         Tdiff.Length(),  distThreshold);
  
  
  // Check if the reprojected points match the originals
  Quad2f reproj;
  Quad3f marker3d_est;
  poseEst.ApplyTo(marker3d, marker3d_est);
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
  
  Pose3d poseOrigin;
  
  const Pose3d camPose(0.f, Z_AXIS_3D(), {0.f, 0.f, 0.f}, &poseOrigin);
  
  const Vision::CameraCalibration calib(240, 320,
                                  300.f, 300.f,
                                  160.f, 120.f);
  
  Vision::Camera camera(57);
  
  EXPECT_TRUE(camera.GetID() == 57);
  
  camera.SetPose(camPose);
  camera.SetCalibration(calib);
  
  // Note that object pose is in camera coordinates
  const Pose3d obj1Pose(M_PI_2, X_AXIS_3D(), {0.f, 0.f, 100.f}, &poseOrigin);
  Vision::KnownMarker object1(0, obj1Pose, 15.f);

  camera.AddOccluder(object1);
  
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
  const Pose3d obj2Pose(M_PI_2, X_AXIS_3D(), {0.f, 0.f, 150.f}, &poseOrigin);
  Vision::KnownMarker object2(0, obj2Pose, 20.f);
  
  camera.AddOccluder(object2);
  
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
  object1.SetPose(Pose3d(M_PI_4, X_AXIS_3D(), {0.f, 0.f, 100.f}, &poseOrigin));
  EXPECT_FALSE( object1.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, DoNotRequireObjectBehind) );
  object1.SetPose(Pose3d(0, X_AXIS_3D(), {0.f, 0.f, 100.f}, &poseOrigin));
  EXPECT_FALSE( object1.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, DoNotRequireObjectBehind) );
  
  // ...it is too far away (and thus too small in the image)
  object1.SetPose(Pose3d(3*M_PI_2, X_AXIS_3D(), {0.f, 0.f, 1000.f}, &poseOrigin));
  EXPECT_FALSE( object1.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, DoNotRequireObjectBehind) );
  
  // ...it is not within the field of view
  object1.SetPose(Pose3d(M_PI_2, X_AXIS_3D(), {100.f, 0.f, 100.f}, &poseOrigin));
  EXPECT_FALSE( object1.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, DoNotRequireObjectBehind) );
  
  // ...it is behind the camera
  object1.SetPose(Pose3d(M_PI_2, X_AXIS_3D(), {100.f, 0.f, -100.f}, &poseOrigin));
  EXPECT_FALSE( object1.IsVisibleFrom(camera, DEG_TO_RAD(5), 5.f, DoNotRequireObjectBehind) );

} // GTEST_TEST(Camera, VisibilityAndOcclusion)


GTEST_TEST(ColorPixels, Accessors)
{
  using namespace Anki::Vision;
  
  PixelRGB_<u8> p(23,11,45);
  
  ASSERT_EQ(23, p.r());
  ASSERT_EQ(11, p.g());
  ASSERT_EQ(45, p.b());
  
  ASSERT_EQ(11, p.min());
  ASSERT_EQ(45, p.max());
  
  ASSERT_TRUE(p.IsDarkerThan(50, false));
  ASSERT_TRUE(p.IsDarkerThan(50, true));
  ASSERT_TRUE(p.IsBrighterThan(5, false));
  ASSERT_TRUE(p.IsBrighterThan(5, true));
  
  ASSERT_FALSE(p.IsDarkerThan(15, false));
  ASSERT_TRUE(p.IsDarkerThan(15, true));
  ASSERT_FALSE(p.IsBrighterThan(40, false));
  ASSERT_TRUE(p.IsBrighterThan(40, true));
  
  p.r() = 223;
  p.g() = 211;
  p.b() = 245;
  
  ASSERT_EQ(223, p[0]);
  ASSERT_EQ(211, p[1]);
  ASSERT_EQ(245, p[2]);
  
  
  PixelRGBA q(23,11,45, 128);
  
  ASSERT_EQ(23, q.r());
  ASSERT_EQ(11, q.g());
  ASSERT_EQ(45, q.b());
  ASSERT_EQ(128, q.a());
  
  ASSERT_TRUE(q.IsDarkerThan(50, false));
  ASSERT_TRUE(q.IsDarkerThan(50, true));
  ASSERT_TRUE(q.IsBrighterThan(5, false));
  ASSERT_TRUE(q.IsBrighterThan(5, true));
  
  ASSERT_FALSE(q.IsDarkerThan(15, false));
  ASSERT_TRUE(q.IsDarkerThan(15, true));
  ASSERT_FALSE(q.IsBrighterThan(40, false));
  ASSERT_TRUE(q.IsBrighterThan(40, true));
  
  q.r() = 223;
  q.g() = 211;
  q.b() = 245;
  q.a() = 200;
  
  ASSERT_EQ(223, q[0]);
  ASSERT_EQ(211, q[1]);
  ASSERT_EQ(245, q[2]);
  ASSERT_EQ(200, q[3]);

}

GTEST_TEST(ColorPixels, GrayConverters)
{
  using namespace Anki::Vision;
  
  const PixelRGB_<u8> p_u8(23,11,45);
  
  ASSERT_EQ(22, p_u8.gray());
  ASSERT_EQ(18, p_u8.weightedGray());
  
  const PixelRGB_<f32> p_f32(23.f, 11.f, 45.f);
  
  ASSERT_NEAR(22.5f, p_f32.gray(), 1e-3f);
  ASSERT_NEAR(18.464f, p_f32.weightedGray(), 1e-3f);
  
  const PixelRGBA p_alpha(23,11,45, 255);
  
  ASSERT_EQ(22, p_alpha.gray());
  ASSERT_EQ(18, p_alpha.weightedGray());
  
}

GTEST_TEST(ImageCache, ImageCacheGray)
{
  using namespace Anki::Vision;
  
  ImageCache cache;
  
  // Nothing in the cache: should fail
  ASSERT_DEATH(cache.GetRGB(), "");
  ASSERT_DEATH(cache.GetGray(), "");
  
  const s32 nrows = 16;
  const s32 ncols = 32;
  Vision::Image imgGray(nrows, ncols);
  imgGray(0,0) = 42;
  cache.Reset(imgGray);
  
  ASSERT_EQ(false, cache.HasColor());
  ASSERT_EQ(nrows, cache.GetOrigNumRows());
  ASSERT_EQ(ncols, cache.GetOrigNumCols());
  
  ImageCache::GetType getType;
  
  // Cached gray image should share data pointer with original
  const Vision::Image& getResult = cache.GetGray(ImageCache::Size::Full, &getType);
  ASSERT_EQ(imgGray.GetDataPointer(), getResult.GetDataPointer());
  
  // Getting original should not have required any resizing/computation
  ASSERT_EQ(ImageCache::GetType::FullyCached, getType);
  
  // Compute new resized entry
  const Vision::Image& halfSize = cache.GetGray(ImageCache::Size::Half_NN, &getType);
  ASSERT_EQ(nrows/2, halfSize.GetNumRows());
  ASSERT_EQ(ncols/2, halfSize.GetNumCols());
  ASSERT_EQ(ImageCache::GetType::NewEntry, getType);
  
  // Compute double sized entry
  const Vision::Image& doubleSize = cache.GetGray(ImageCache::Size::Double_NN, &getType);
  ASSERT_EQ(nrows*2, doubleSize.GetNumRows());
  ASSERT_EQ(ncols*2, doubleSize.GetNumCols());
  ASSERT_EQ(ImageCache::GetType::NewEntry, getType);
  
  // Second request should be cached
  const Vision::Image& halfSize2 = cache.GetGray(ImageCache::Size::Half_NN, &getType);
  ASSERT_EQ(&halfSize, &halfSize2);
  ASSERT_EQ(ImageCache::GetType::FullyCached, getType);
  
  // Request with different resize method should compute
  const Vision::Image& halfSize3 = cache.GetGray(ImageCache::Size::Half_Linear, &getType);
  ASSERT_EQ(nrows/2, halfSize3.GetNumRows());
  ASSERT_EQ(ncols/2, halfSize3.GetNumCols());
  ASSERT_EQ(ImageCache::GetType::NewEntry, getType);
  
  // Get full-scale color version (should require computation)
  const Vision::ImageRGB& colorVersion = cache.GetRGB(ImageCache::Size::Full, &getType);
  ASSERT_EQ(nrows, colorVersion.GetNumRows());
  ASSERT_EQ(ncols, colorVersion.GetNumCols());
  ASSERT_EQ(ImageCache::GetType::ComputeFromExisting, getType);
  
  // Gray value should be replicated to all three color channels
  ASSERT_EQ(Vision::PixelRGB(42,42,42), colorVersion(0,0));
  
  // Getting colorized gray should not change cache's HasColor status
  ASSERT_EQ(false, cache.HasColor());
  
  // Second request for color should be cached
  const Vision::ImageRGB& colorVersion2 = cache.GetRGB(ImageCache::Size::Full, &getType);
  ASSERT_EQ(&colorVersion, &colorVersion2);
  ASSERT_EQ(ImageCache::GetType::FullyCached, getType);
  
  // Request for quarter size color should create new entry
  const Vision::ImageRGB& qtrSizeColor = cache.GetRGB(ImageCache::Size::Quarter_Linear, &getType);
  ASSERT_EQ(ImageCache::GetType::NewEntry, getType);
  ASSERT_EQ(nrows/4, qtrSizeColor.GetNumRows());
  ASSERT_EQ(ncols/4, qtrSizeColor.GetNumCols());
  
  // Asking for quarter size gray should compute it from the color one we just created
  const Vision::Image& qtrSizeGray = cache.GetGray(ImageCache::Size::Quarter_Linear, &getType);
  ASSERT_EQ(ImageCache::GetType::ComputeFromExisting, getType);
  ASSERT_EQ(nrows/4, qtrSizeGray.GetNumRows());
  ASSERT_EQ(ncols/4, qtrSizeGray.GetNumCols());
  
  // Resetting with a new image and asking for half size should resize into an existing entry and use same data
  Vision::Image newImg(nrows,ncols);
  cache.Reset(newImg);
  const Vision::Image& newHalfSize = cache.GetGray(ImageCache::Size::Half_NN, &getType);
  ASSERT_EQ(ImageCache::GetType::ResizeIntoExisting, getType);
  ASSERT_EQ(newHalfSize.GetDataPointer(), halfSize.GetDataPointer());
  
  // Resetting with a new color image should mean asking for full-sized gray will cause a compute
  Vision::ImageRGB newColorImg(nrows,ncols);
  const Vision::PixelRGB colorPixel(5, 10, 15);
  newColorImg(0,0) = colorPixel;
  cache.Reset(newColorImg);
  ASSERT_EQ(true, cache.HasColor());
  const Vision::Image& newGray = cache.GetGray(ImageCache::Size::Full, &getType);
  ASSERT_EQ(ImageCache::GetType::ComputeFromExisting, getType);
  ASSERT_EQ(nrows, newGray.GetNumRows());
  ASSERT_EQ(ncols, newGray.GetNumCols());
  
  // Gray value should be weighted average of the RGB values
  ASSERT_EQ(colorPixel.weightedGray(), newGray(0,0));
  
  // ReleaseMemory should clear out the cache and thus a request for an image should fail
  cache.ReleaseMemory();
  ASSERT_DEATH(cache.GetGray(), "");
  
  // Resetting with a color image and requesting a resized version should trigger a
  // new allocation and not a resize into existing memory, since we called ReleaseMemory
  cache.Reset(newColorImg);
  ASSERT_EQ(true, cache.HasColor());
  cache.GetRGB(ImageCache::Size::Half_NN, &getType);
  ASSERT_EQ(ImageCache::GetType::NewEntry, getType);
}
