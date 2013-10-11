#include "gtest/gtest.h"

#include "anki/common/general.h"
#include "anki/math/rotation.h"
#include <iostream>

// For test debug printing
#define DEBUG_TEST_ROTATION

#define ASSERT_NEAR_EQ(a,b) ASSERT_NEAR(a,b,FLOATING_POINT_COMPARISON_TOLERANCE)


using namespace std;
using namespace Anki;



GTEST_TEST(TestRotation, Rotation2dNegativeAngle)
{
  const Radians angle = M_PI/3.f;
  
  RotationMatrix2d R(angle);
  
  RotationMatrix2d R_neg(-angle);
  
  RotationMatrix2d Rt(R);
  Rt.Transpose();
  
  RotationMatrix2d Rinv(R);
  Rinv.Invert();
  
  // A rotaiton matrix constructed from the negative angle
  // should be the same as the tranpose of the original matrix
  // which should be the same as the inverse of the rotation
  // matrix.
  EXPECT_EQ(R_neg, Rt);
  EXPECT_EQ(Rt, Rinv);
  
} // TestRotation:Rotation2dNegativeAngle


GTEST_TEST(TestRotation, Rotation3dNegativeAngle)
{
  const Radians angle = M_PI/3.f;
  Vec3f axis(0.3f, 0.4f, -.15f);
  axis.makeUnitLength();
  
  RotationVector3d Rvec(angle, axis);
  RotationMatrix3d Rmat(Rvec);
  
  RotationVector3d Rvec_neg(-angle, axis);
  RotationMatrix3d Rmat_neg(Rvec_neg);
  
  RotationMatrix3d Rt(Rmat);
  Rt.Transpose();
  
  RotationMatrix3d Rinv(Rmat);
  Rinv.Invert();
  
  // A rotaiton matrix constructed from the negative angle
  // should be the same as the tranpose of the original matrix
  // which should be the same as the inverse of the rotation
  // matrix.
  EXPECT_TRUE(nearlyEqual(Rmat_neg, Rt));
  EXPECT_TRUE(nearlyEqual(Rt, Rinv));
  
} // TestRotation:Rotation3dNegativeAngle


GTEST_TEST(TestRotation, PointRotation2d)
{
  
  Point2f p(1.f, 0.f);
  RotationMatrix2d R_30(M_PI/6.f);
  RotationMatrix2d R_45(M_PI/4.f);
  RotationMatrix2d R_60(M_PI/3.f);
  RotationMatrix2d R_90(M_PI/2.f);
  RotationMatrix2d R_135(3.f*M_PI/4.f);
  RotationMatrix2d R_180(M_PI);
  RotationMatrix2d R_330(11.f*M_PI/6.f);
  
  Point2f q = R_45 * Point2f(0.f, 0.f);
  EXPECT_EQ(q.x(), 0.f);
  EXPECT_EQ(q.y(), 0.f);
  
  const float eps = 10.f*std::numeric_limits<float>::epsilon();
  
  q = R_30 * p;
  EXPECT_TRUE( NEAR(q.x(), std::sqrt(3.f)/2.f, eps) );
  EXPECT_TRUE( NEAR(q.y(), 0.5f, eps) );
  
  q = R_45 * p;
  EXPECT_TRUE( NEAR(q.x(), std::sqrt(2.f)/2.f, eps) );
  EXPECT_TRUE( NEAR(q.y(), std::sqrt(2.f)/2.f, eps) );
  
  q = R_60 * p;
  EXPECT_TRUE( NEAR(q.x(), 0.5f, eps) );
  EXPECT_TRUE( NEAR(q.y(), std::sqrt(3.f)/2.f, eps) );
  
  q = R_90 * p;
  EXPECT_TRUE( NEAR(q.x(), 0.f, eps) );
  EXPECT_TRUE( NEAR(q.y(), 1.f, eps) );
  
  q = R_135 * p;
  EXPECT_TRUE( NEAR(q.x(), -std::sqrt(2.f)/2.f, eps) );
  EXPECT_TRUE( NEAR(q.y(),  std::sqrt(2.f)/2.f, eps) );
  
  q = R_180 * p;
  EXPECT_TRUE( NEAR(q.x(), -1.f, eps) );
  EXPECT_TRUE( NEAR(q.y(),  0.f, eps) );
  
  q = R_330 * p;
  EXPECT_TRUE( NEAR(q.x(), std::sqrt(3.f)/2.f, eps) );
  EXPECT_TRUE( NEAR(q.y(), -0.5f, eps) );
  
} // TestRotation:PointRotation2d


GTEST_TEST(TestRotation, AxisRotation3d)
{
  
  Radians angle = M_PI/2.f;
  Vec3f   Xaxis(1.f, 0.f, 0.f);
  Vec3f   Yaxis(0.f, 1.f, 0.f);
  Vec3f   Zaxis(0.f, 0.f, 1.f);
  
  Vec3f negXaxis(-1.f,  0.f,  0.f);
  Vec3f negYaxis( 0.f, -1.f,  0.f);
  Vec3f negZaxis( 0.f,  0.f, -1.f);
  
  RotationMatrix3d R_90x(angle, Xaxis);
  RotationMatrix3d R_90y(angle, Yaxis);
  RotationMatrix3d R_90z(angle, Zaxis);
  
  Point3f q = R_90x * Xaxis;
  EXPECT_TRUE( nearlyEqual(q, Xaxis) );
  
  q = R_90y * Xaxis;
  EXPECT_TRUE( nearlyEqual(q, negZaxis) );
  
  q = R_90z * Xaxis;
  EXPECT_TRUE( nearlyEqual(q, Yaxis) );

  q = R_90x * Yaxis;
  EXPECT_TRUE( nearlyEqual(q, Zaxis) );
  
  q = R_90z * Yaxis;
  EXPECT_TRUE( nearlyEqual(q, negXaxis) );

  RotationMatrix3d R_neg90x(-angle, Xaxis);
  RotationMatrix3d R_neg90y(-angle, Yaxis);
  RotationMatrix3d R_neg90z(-angle, Zaxis);
  
  q = R_neg90x * Xaxis;
  EXPECT_TRUE( nearlyEqual(q, Xaxis) );
  
  q = R_neg90y * Xaxis;
  EXPECT_TRUE( nearlyEqual(q, Zaxis) );
  
  q = R_neg90z * Xaxis;
  EXPECT_TRUE( nearlyEqual(q, negYaxis) );
  
  
  
} // TestRotation:AxisRotation3d
