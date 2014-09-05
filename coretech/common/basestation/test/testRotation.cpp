#include "gtest/gtest.h"

#include "anki/common/basestation/math/rotation.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/matrix_impl.h"

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
  axis.MakeUnitLength();
  
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

GTEST_TEST(TestRotation, ExtractAnglesFromMatrix)
{
  std::vector<f32> testAngles = {
    0.f, 30.f, 45.f, 60.f, 90.f, 121.f, 147.f, 180.f, 182.f, 233.f, 270.f,
    291.f, 333.f, 352.f, 360.f, 378.f
  };
  
  const f32 signs[2] = {-1.f, 1.f};
  
  const f32 TOLERANCE = 1e-6f;
  
  for(auto angle : testAngles) {
    for(auto sign : signs) {
      const Radians curAngle = DEG_TO_RAD(sign*angle);
      
      RotationMatrix3d Rx(curAngle, X_AXIS_3D);
      RotationMatrix3d Ry(curAngle, Y_AXIS_3D);
      RotationMatrix3d Rz(curAngle, Z_AXIS_3D);
      
      EXPECT_NEAR((Rx.GetAngleAroundXaxis() - curAngle).ToFloat(), 0.f, TOLERANCE);
      //EXPECT_NEAR(Rx.GetAngleAroundYaxis().ToFloat(), 0.f, TOLERANCE);
      //EXPECT_NEAR(Rx.GetAngleAroundZaxis().ToFloat(), 0.f, TOLERANCE);

      EXPECT_NEAR((Ry.GetAngleAroundYaxis() - curAngle).ToFloat(), 0.f, TOLERANCE);
      //EXPECT_NEAR(Ry.GetAngleAroundXaxis().ToFloat(), 0.f, TOLERANCE);
      //EXPECT_NEAR(Ry.GetAngleAroundZaxis().ToFloat(), 0.f, TOLERANCE);

      EXPECT_NEAR((Rz.GetAngleAroundZaxis() - curAngle).ToFloat(), 0.f, TOLERANCE);
      //EXPECT_NEAR(Rz.GetAngleAroundXaxis().ToFloat(), 0.f, TOLERANCE);
      //EXPECT_NEAR(Rz.GetAngleAroundYaxis().ToFloat(), 0.f, TOLERANCE);
      
    }
  }
  
} // TestRotation:ExtractAnglesFromMatrix

/*
GTEST_TEST(TestRotation, EulerAngles)
{
  std::vector<f32> testAngles = {
    0.f, 30.f, 45.f, 60.f, 90.f, 121.f, 147.f, 180.f, 182.f, 233.f, 270.f,
    291.f, 333.f, 352.f, 360.f, 378.f
  };
  
  const f32 signs[2] = {-1.f, 1.f};
  
  const f32 TOLERANCE = 1e-6f;
  
  for(auto Xangle : testAngles) {
    for(auto Xsign : signs) {
      for(auto Yangle : testAngles) {
        for(auto Ysign : signs) {
          for(auto Zangle : testAngles) {
            for(auto Zsign : signs) {
              const Radians curXangle = DEG_TO_RAD(Xsign*Xangle);
              const Radians curYangle = DEG_TO_RAD(Ysign*Yangle);
              const Radians curZangle = DEG_TO_RAD(Zsign*Zangle);
              
              RotationMatrix3d Rx(curXangle, X_AXIS_3D);
              RotationMatrix3d Ry(curYangle, Y_AXIS_3D);
              RotationMatrix3d Rz(curZangle, Z_AXIS_3D);

              EXPECT_NEAR((Rx.GetAngleAroundXaxis() - curXangle).ToFloat(), 0.f, TOLERANCE);
              EXPECT_NEAR((Ry.GetAngleAroundYaxis() - curYangle).ToFloat(), 0.f, TOLERANCE);
              EXPECT_NEAR((Rz.GetAngleAroundZaxis() - curZangle).ToFloat(), 0.f, TOLERANCE);

              RotationMatrix3d R( Rz*Ry*Rx );
              
              Radians checkXangle, checkYangle, checkZangle;
              R.GetEulerAngles(checkXangle, checkYangle, checkZangle);
              EXPECT_NEAR((checkXangle - curXangle).ToFloat(), 0.f, TOLERANCE);
              EXPECT_NEAR((checkYangle - curYangle).ToFloat(), 0.f, TOLERANCE);
              EXPECT_NEAR((checkZangle - curZangle).ToFloat(), 0.f, TOLERANCE);
            }
          }
        }
      }
    }
  }
 
  
} // TestRotation:EuelerAngles
*/

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


// TODO: move these pose tests to their own testPose.cpp file
GTEST_TEST(TestPose, IsSameWithAmbiguity)
{
  Pose3d P_ref(M_PI/2, {0,1.f,0}, {10.f,20.f,30.f});
  Pose3d P1(P_ref);
  
  RotationMatrix3d R_amb({0,-1,0,  0,0,1,  1,0,0});
  Pose3d Tform_amb(R_amb, {0,0,0});

  // P2 is P1 adjusted by an "ambiguous" rotation
  Pose3d P2(P1);
  P2.PreComposeWith(Tform_amb);
  
  // Now adjust both P1 and P2 by some arbtrary transformation
  Pose3d Tform(M_PI/3.f, {0.5f*sqrtf(2.f), 0.5f*sqrtf(2.f), 0.f}, {0,0,0});
  P1.PreComposeWith(Tform);
  P2.PreComposeWith(Tform);
  
  // the only difference between P1 and P2 at this point should be Tform_amb
  
  std::vector<RotationMatrix3d> ambiguities = {
    RotationMatrix3d({1,0,0,  0,1,0,  0,0,1}),
    RotationMatrix3d({0,1,0,  0,0,1,  1,0,0}),
    RotationMatrix3d({0,0,1,  1,0,0,  0,1,0})
  };
  
  // The IsSameAs_WithAmbiguity functino should see these two poses as the same,
  // though it is our job to factor out the reference pose from each, by post-
  // multiplying by its inverse.
  EXPECT_TRUE( (P1*P_ref.GetInverse()).IsSameAs_WithAmbiguity(P2*P_ref.GetInverse(),
                                                              ambiguities, 5.f,
                                                              5*M_PI/180.f, true) );
  
  // The returned difference should be the ambiguous rotation we applied to P2
  // above
  //EXPECT_NEAR( R_amb.GetAngleDiffFrom(P_diff.GetRotationMatrix()).ToFloat(), 0.f, 1.f*M_PI/180.f );
  
  // P3 is P1 with a slight perturbation
  Pose3d P3(P1);
  
  Pose3d T_perturb(2.f * M_PI/180.f, Z_AXIS_3D, {1.f, 1.f, 1.f});
  P3.PreComposeWith(T_perturb);
  
  // IsSameAs should return true, and the pose difference should be T_perturb
  EXPECT_TRUE(P1.IsSameAs(P3, 5.f, 3.f*M_PI/180.f));
  
  //Pose3d P_temp;
  //EXPECT_TRUE(T_perturb.IsSameAs(P_diff, .01f, .1*M_PI/180.f, P_temp));
  
} // TestPose:IsSameWithAmbiguity



