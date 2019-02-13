#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header

#include "coretech/common/shared/math/rotation.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/shared/math/matrix_impl.h"

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
  EXPECT_TRUE(IsNearlyEqual(Rmat_neg, Rt));
  EXPECT_TRUE(IsNearlyEqual(Rt, Rinv));
  
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

GTEST_TEST(TestRotation, PointRotation3d)
{
  
  Point3f p(1.f, 0.f, 0.f);
  Rotation3d R_30(M_PI/6.f, Z_AXIS_3D());
  Rotation3d R_45(M_PI/4.f, Z_AXIS_3D());
  Rotation3d R_60(M_PI/3.f, Z_AXIS_3D());
  Rotation3d R_90(M_PI/2.f, Z_AXIS_3D());
  Rotation3d R_135(3.f*M_PI/4.f, Z_AXIS_3D());
  Rotation3d R_180(M_PI, Z_AXIS_3D());
  Rotation3d R_330(11.f*M_PI/6.f, Z_AXIS_3D());
  
  Point3f q = R_45 * Point3f(0.f, 0.f, 0.f);
  EXPECT_EQ(q.x(), 0.f);
  EXPECT_EQ(q.y(), 0.f);
  EXPECT_EQ(q.z(), 0.f);
  
  const float eps = 10.f*std::numeric_limits<float>::epsilon();
  const float epsBig = 100*eps;
  
  q = R_30 * p;
  EXPECT_TRUE( NEAR(q.x(), std::sqrt(3.f)/2.f, eps) );
  EXPECT_TRUE( NEAR(q.y(), 0.5f, eps) );
  EXPECT_TRUE( NEAR(q.z(), 0.f, eps) );
  EXPECT_NEAR(q.Length(), p.Length(), epsBig);
  
  q = R_45 * p;
  EXPECT_TRUE( NEAR(q.x(), std::sqrt(2.f)/2.f, eps) );
  EXPECT_TRUE( NEAR(q.y(), std::sqrt(2.f)/2.f, eps) );
  EXPECT_TRUE( NEAR(q.z(), 0.f, eps) );
  EXPECT_NEAR(q.Length(), p.Length(), epsBig);
  
  q = R_60 * p;
  EXPECT_TRUE( NEAR(q.x(), 0.5f, eps) );
  EXPECT_TRUE( NEAR(q.y(), std::sqrt(3.f)/2.f, eps) );
  EXPECT_TRUE( NEAR(q.z(), 0.f, eps) );
  EXPECT_NEAR(q.Length(), p.Length(), epsBig);
  
  q = R_90 * p;
  EXPECT_TRUE( NEAR(q.x(), 0.f, eps) );
  EXPECT_TRUE( NEAR(q.y(), 1.f, eps) );
  EXPECT_TRUE( NEAR(q.z(), 0.f, eps) );
  EXPECT_NEAR(q.Length(), p.Length(), epsBig);
  
  q = R_135 * p;
  EXPECT_TRUE( NEAR(q.x(), -std::sqrt(2.f)/2.f, eps) );
  EXPECT_TRUE( NEAR(q.y(),  std::sqrt(2.f)/2.f, eps) );
  EXPECT_TRUE( NEAR(q.z(), 0.f, eps) );
  EXPECT_NEAR(q.Length(), p.Length(), epsBig);
  
  q = R_180 * p;
  EXPECT_TRUE( NEAR(q.x(), -1.f, eps) );
  EXPECT_TRUE( NEAR(q.y(),  0.f, eps) );
  EXPECT_TRUE( NEAR(q.z(), 0.f, eps) );
  EXPECT_NEAR(q.Length(), p.Length(), epsBig);
  
  q = R_330 * p;
  EXPECT_TRUE( NEAR(q.x(), std::sqrt(3.f)/2.f, eps) );
  EXPECT_TRUE( NEAR(q.y(), -0.5f, eps) );
  EXPECT_TRUE( NEAR(q.z(), 0.f, eps) );
  EXPECT_NEAR(q.Length(), p.Length(), epsBig);
  
  p = {44.f, 44.f, 44.f};
  Rotation3d R(1.076822398155559f, {0.573788055924467f,   0.808395453061550f,   0.131392763681385f});
  q = R * p;
  EXPECT_NEAR( q.x(), 67.183776745385970f, epsBig);
  EXPECT_NEAR( q.y(), 32.034582545499632f, epsBig);
  EXPECT_NEAR( q.z(), 16.374543149709780f, epsBig);
  
  EXPECT_NEAR(q.Length(), p.Length(), epsBig);
  
  
} // TestRotation:PointRotation3d


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
      
      RotationMatrix3d Rx(curAngle, X_AXIS_3D());
      RotationMatrix3d Ry(curAngle, Y_AXIS_3D());
      RotationMatrix3d Rz(curAngle, Z_AXIS_3D());
      
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


GTEST_TEST(TestRotation, ExtractAnglesFromRotation3d)
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
      
      Rotation3d Rx(curAngle, X_AXIS_3D());
      Rotation3d Ry(curAngle, Y_AXIS_3D());
      Rotation3d Rz(curAngle, Z_AXIS_3D());
      
      EXPECT_NEAR((Rx.GetAngleAroundXaxis() - curAngle).ToFloat(), 0.f, TOLERANCE);
      
      EXPECT_NEAR((Ry.GetAngleAroundYaxis() - curAngle).ToFloat(), 0.f, TOLERANCE);
      
      EXPECT_NEAR((Rz.GetAngleAroundZaxis() - curAngle).ToFloat(), 0.f, TOLERANCE);
      
    }
  }
  
} // TestRotation:ExtractAnglesFromRotation3d


GTEST_TEST(TestRotation, EulerAngles)
{
  {
    // Rotate 90deg around Z axis:
    // - Rotate X axis should result in Y axis
    RotationMatrix3d R(0, 0, M_PI_2);
    Vec3f axis = R * X_AXIS_3D();
    EXPECT_TRUE(IsNearlyEqual(axis, Y_AXIS_3D(), 1e-6f));
    
    // - Rotate Y axis should result in -X axis
    axis = R * Y_AXIS_3D();
    EXPECT_TRUE(IsNearlyEqual(axis, -X_AXIS_3D(), 1e-6f));
    
    // - Rotate Z axis should result in Z axis
    axis = R * Z_AXIS_3D();
    EXPECT_TRUE(IsNearlyEqual(axis, Z_AXIS_3D(), 1e-6f));
  }
  
  {
    // Rotate 90deg around X axis:
    // - Rotate X axis should result in X axis
    RotationMatrix3d R(M_PI_2, 0, 0);
    Vec3f axis = R * X_AXIS_3D();
    EXPECT_TRUE(IsNearlyEqual(axis, X_AXIS_3D(), 1e-6f));
    
    // - Rotate Y axis should result in Z axis
    axis = R * Y_AXIS_3D();
    EXPECT_TRUE(IsNearlyEqual(axis, Z_AXIS_3D(), 1e-6f));
    
    // - Rotate Z axis should result in -Y axis
    axis = R * Z_AXIS_3D();
    EXPECT_TRUE(IsNearlyEqual(axis, -Y_AXIS_3D(), 1e-6f));
  }
  
  {
    // Rotate 90deg around Y axis:
    // - Rotate X axis should result in -Z axis
    RotationMatrix3d R(0, M_PI_2, 0);
    Vec3f axis = R * X_AXIS_3D();
    EXPECT_TRUE(IsNearlyEqual(axis, -Z_AXIS_3D(), 1e-6f));
    
    // - Rotate Y axis should result in Y axis
    axis = R * Y_AXIS_3D();
    EXPECT_TRUE(IsNearlyEqual(axis, Y_AXIS_3D(), 1e-6f));
    
    // - Rotate Z axis should result in X axis
    axis = R * Z_AXIS_3D();
    EXPECT_TRUE(IsNearlyEqual(axis, X_AXIS_3D(), 1e-6f));
  }
} // TestRotation::EulerAngles


GTEST_TEST(TestRotation, ToFromEulerAngles)
{
  // Will use all combinations of these angles for setting X,Y,Z angles:
   const std::vector<Radians> angles = {
     1.83f, .345f,  -.521f, 0, M_PI_2, -M_PI_2, M_PI, -M_PI, M_PI_4, -M_PI_4
   };
  
  for(auto angleX : angles)
  {
    for(auto angleY : angles)
    {
      for(auto angleZ : angles)
      {
        // Construct the rotation matrix
        RotationMatrix3d R(angleX, angleY, angleZ);
        
        // Get the Euler angles from the rotation matrix (allowing for
        // possible alernate solutions)
        Radians checkX=0.f, checkY=0.f, checkZ=0.f;
        Radians checkX2=0.f, checkY2=0.f, checkZ2=0.f; // for alt. solution
        bool inGimbalLock = R.GetEulerAngles(checkX, checkY, checkZ,
                                             &checkX2, &checkY2, &checkZ2);
        
        // Don't bother even checking when in Gimbal lock (?)
        if(!inGimbalLock)
        {
          // Either solution can be correct for the purpose of unit tests!
          // Note the use of angle differences, courtesy of the Radians class.
          const Point3f diff((angleX - checkX).ToFloat(),
                             (angleY - checkY).ToFloat(),
                             (angleZ - checkZ).ToFloat());
          
          const Point3f diff2((angleX - checkX2).ToFloat(),
                              (angleY - checkY2).ToFloat(),
                              (angleZ - checkZ2).ToFloat());
          
          EXPECT_TRUE(IsNearlyEqual(diff, {0.f, 0.f, 0.f}, 1e-6f) ||
                      IsNearlyEqual(diff2, {0.f, 0.f, 0.f}, 1e-6f));
        }
         
      }
    }
  }
  
} // TestRotation::ToFromEulerAngles

GTEST_TEST(TestRotation, AxisRotationMatrix3d)
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
  EXPECT_TRUE( IsNearlyEqual(q, Xaxis) );
  
  q = R_90y * Xaxis;
  EXPECT_TRUE( IsNearlyEqual(q, negZaxis) );
  
  q = R_90z * Xaxis;
  EXPECT_TRUE( IsNearlyEqual(q, Yaxis) );

  q = R_90x * Yaxis;
  EXPECT_TRUE( IsNearlyEqual(q, Zaxis) );
  
  q = R_90z * Yaxis;
  EXPECT_TRUE( IsNearlyEqual(q, negXaxis) );

  RotationMatrix3d R_neg90x(-angle, Xaxis);
  RotationMatrix3d R_neg90y(-angle, Yaxis);
  RotationMatrix3d R_neg90z(-angle, Zaxis);
  
  q = R_neg90x * Xaxis;
  EXPECT_TRUE( IsNearlyEqual(q, Xaxis) );
  
  q = R_neg90y * Xaxis;
  EXPECT_TRUE( IsNearlyEqual(q, Zaxis) );
  
  q = R_neg90z * Xaxis;
  EXPECT_TRUE( IsNearlyEqual(q, negYaxis) );
  
} // TestRotation:AxisRotation3d

GTEST_TEST(TestRotation, AxisRotation3d)
{
  
  Radians angle = M_PI/2.f;
  Vec3f   Xaxis(1.f, 0.f, 0.f);
  Vec3f   Yaxis(0.f, 1.f, 0.f);
  Vec3f   Zaxis(0.f, 0.f, 1.f);
  
  Vec3f negXaxis(-1.f,  0.f,  0.f);
  Vec3f negYaxis( 0.f, -1.f,  0.f);
  Vec3f negZaxis( 0.f,  0.f, -1.f);
  
  Rotation3d R_90x(angle, Xaxis);
  Rotation3d R_90y(angle, Yaxis);
  Rotation3d R_90z(angle, Zaxis);
  
  Point3f q = R_90x * Xaxis;
  EXPECT_TRUE( IsNearlyEqual(q, Xaxis) );
  
  q = R_90y * Xaxis;
  EXPECT_TRUE( IsNearlyEqual(q, negZaxis) );
  
  q = R_90z * Xaxis;
  EXPECT_TRUE( IsNearlyEqual(q, Yaxis) );
  
  q = R_90x * Yaxis;
  EXPECT_TRUE( IsNearlyEqual(q, Zaxis) );
  
  q = R_90z * Yaxis;
  EXPECT_TRUE( IsNearlyEqual(q, negXaxis) );
  
  Rotation3d R_neg90x(-angle, Xaxis);
  Rotation3d R_neg90y(-angle, Yaxis);
  Rotation3d R_neg90z(-angle, Zaxis);
  
  q = R_neg90x * Xaxis;
  EXPECT_TRUE( IsNearlyEqual(q, Xaxis) );
  
  q = R_neg90y * Xaxis;
  EXPECT_TRUE( IsNearlyEqual(q, Zaxis) );
  
  q = R_neg90z * Xaxis;
  EXPECT_TRUE( IsNearlyEqual(q, negYaxis) );
  
} // TestRotation:AxisRotation3d

GTEST_TEST(TestRotation, Rotation3d)
{
  // pi/7 radians around [0.6651 0.7395 0.1037] axis
  const RotationMatrix3d Rmat1 = {
     0.944781277013538,   0.003728131819389,   0.327680392513508,
     0.093691220482485,   0.955123218207869,  -0.281001055593651,
    -0.314022760017760,   0.296185312048692,   0.902033240583432
  };
  
  // 0.6283 radians around [-0.8190   -0.5670   -0.0875] axis
  const RotationMatrix3d Rmat2 = {
    0.937130929836233,   0.140108185633360,  -0.319617453626684,
    0.037286544290262,   0.870425010004489,   0.490886968225451,
    0.346980307739744,  -0.471942791318199,   0.810478048909173
  };
  
  // Matrix product as computed in Matlab, R1*R2
  const RotationMatrix3d Rprod = {
    0.999221409206381,  -0.019029749384142,  -0.034560729477129,
    0.025912352001530,   0.977106466215908,   0.211167004271049,
    0.029751057079763,  -0.211898141373248,   0.976838805681470
  };
  
  Rotation3d R1(Rmat1), R2(Rmat2);
  
  const f32 TOL = 1e-4f;
  
  EXPECT_NEAR(R1.GetAngle().ToFloat(), M_PI/7.f, TOL);
  EXPECT_NEAR(R2.GetAngle().ToFloat(), 0.62832, TOL);
  EXPECT_TRUE(IsNearlyEqual(R1.GetAxis(), {0.6651, 0.7395, 0.1037}, TOL));
  EXPECT_TRUE(IsNearlyEqual(R2.GetAxis(), {-0.8190, -0.5670, -0.0875}, TOL));
  
  // Testing rotation multiplication (as well as conversion from quaternion to Rotation Matrix)
  Rotation3d R3( R1 * R2 );
  
  EXPECT_TRUE(IsNearlyEqual(R3.GetRotationMatrix(), Rprod, 1e-6f));
  
  // Testing multiplication in place
  Rotation3d R4(R1);
  R4 *= R2;
  
  EXPECT_TRUE(IsNearlyEqual(R3, R4, 1e-6f));
  
  const RotationVector3d Rvec(R3.GetRotationVector());
  EXPECT_NEAR(Rvec.GetAngle().ToFloat(), 0.2168, TOL);
  EXPECT_TRUE(IsNearlyEqual(Rvec.GetAxis(), {-0.9832, -0.1495, 0.1044}, TOL));
  
} // TestRotation:Rotation3d


GTEST_TEST(TestPose, Inverse)
{
  const Pose3d pose(DEG_TO_RAD(45), Z_AXIS_3D(), {100.f, 0.f, 0.f});
  const Pose3d invPose(pose.GetInverse());
  
  Pose3d check = pose * invPose;
  
  EXPECT_TRUE(check.GetTranslation().GetAbs().AllLT(1e-5f));
  EXPECT_TRUE(check.GetRotationAngle().getAbsoluteVal().ToFloat() < 1e-5f);
  
  check = invPose * pose;
  
  EXPECT_TRUE(check.GetTranslation().GetAbs().AllLT(1e-5f));
  EXPECT_TRUE(check.GetRotationAngle().getAbsoluteVal().ToFloat() < 1e-5f);
  
}

// TODO: move these pose tests to their own testPose.cpp file
GTEST_TEST(TestPose, IsSameAs)
{
  const Pose3d origin, origin2;
  
  const std::vector<Pose3d> P1s{
    Pose3d(0, Z_AXIS_3D(), {10.f,20.f,30.f}, origin),
    Pose3d(DEG_TO_RAD(90), Y_AXIS_3D(), {10.f,20.f,30.f}, origin),
    Pose3d(DEG_TO_RAD(10), X_AXIS_3D(), {-10.f, 0.f, 100.f}, origin),
    Pose3d(DEG_TO_RAD(45), Z_AXIS_3D(), {1.f,0.f,-30.f}, origin),
    Pose3d(DEG_TO_RAD(180), Y_AXIS_3D(), {0.f,0.f,0.f}, origin),
  };
  
  for(auto const& P1 : P1s)
  {
    // P2 is P1 with a slight perturbation
    Pose3d P2(P1);
    
    Pose3d T_perturb(DEG_TO_RAD(2.f), Z_AXIS_3D(), {1.f, 1.f, 1.f});
    P2.PreComposeWith(T_perturb);
    
    // IsSameAs should return true
    EXPECT_TRUE(P1.IsSameAs(P2, 5.f, DEG_TO_RAD(3.f)));
    P2 = P1;
    P2 *= T_perturb;
    EXPECT_TRUE(P1.IsSameAs(P2, 5.f, DEG_TO_RAD(3.f)));
    
    // P3 is a larger perturbation and should fail IsSameAs
    Pose3d P3(P1);
    Pose3d T_bigAnglePerturb(DEG_TO_RAD(10), X_AXIS_3D(), {0.f, 0.f, 0.f});
    P3.PreComposeWith(T_bigAnglePerturb);
    EXPECT_FALSE(P1.IsSameAs(P3, 5.f, DEG_TO_RAD(3.f)));
    P3 = P1;
    P3 *= T_bigAnglePerturb;
    EXPECT_FALSE(P1.IsSameAs(P3, 5.f, DEG_TO_RAD(3.f)));
    Pose3d T_bigTransPerturb(0.f, Z_AXIS_3D(), {10.f, -10.f, 5.f});
    P3 = P1;
    P3.PreComposeWith(T_bigTransPerturb);
    EXPECT_FALSE(P1.IsSameAs(P3, 5.f, DEG_TO_RAD(3.f)));
    P3 = P1;
    P3 *= T_bigTransPerturb;
    EXPECT_FALSE(P1.IsSameAs(P3, 5.f, DEG_TO_RAD(3.f)));
    
    // Poses with different origins should always fail IsSameAs
    P3 = P1;
    P3.SetParent(origin2);
    EXPECT_FALSE(P1.IsSameAs(P3, 1000.f, M_PI));
  }
  
  
  // Assymmetric distance threshold: loose check only in X direction, so
  // shifting P1 along X a bunch should be fine, but small shift along Y should
  // fail
  const Pose3d P1(0, Z_AXIS_3D(), {10.f,20.f,30.f}, origin);
  const Point3f distThreshold(100.f, 5.f, 5.f);
  Pose3d P4(P1);
  
  P4.SetTranslation( P1.GetTranslation() + Point3f{50.f, 1.f, -1.f});
  EXPECT_TRUE(P1.IsSameAs(P4, distThreshold, DEG_TO_RAD(3.f)));

  P4.SetTranslation(P1.GetTranslation() + Point3f{0.f, -6.f, 0.f});
  EXPECT_FALSE(P1.IsSameAs(P4, distThreshold, DEG_TO_RAD(3.f)));
  
  P4.SetTranslation(P1.GetTranslation() + Point3f{0.f, 0.f, 2.5f});
  EXPECT_TRUE(P1.IsSameAs(P4, distThreshold, DEG_TO_RAD(3.f)));
  
  // P6 is not axis aligned. distThreshold will be considered in its rotated frame
  const Pose3d P6(DEG_TO_RAD(45), Z_AXIS_3D(), {100.f, 100.f, 0.f}, origin);
  
  Pose3d P7;
  P7.SetParent(origin);
  EXPECT_FALSE(P6.IsSameAs(P7, distThreshold, DEG_TO_RAD(30)));
  
  P7.SetTranslation({80.f, 80.f, 0.f});
  EXPECT_FALSE(P6.IsSameAs(P7, distThreshold, DEG_TO_RAD(30))); // Should fail: angle thresh too tight
  EXPECT_TRUE(P6.IsSameAs(P7, distThreshold, DEG_TO_RAD(90)));  // Should pass with looser angle threshold
  
  P7.SetRotation(P6.GetRotation());  // Now rotation matches perfectly
  P7.SetTranslation(Point3f{105.f,95.f,0.f}); // But translation is just outside the (rotated) box around P6
  EXPECT_FALSE(P6.IsSameAs(P7, distThreshold, DEG_TO_RAD(90)));
  
  P7.SetTranslation(Point3f{120.f, 120.f, 1.f}); // Rotation still matches, translation along lenient dimension
  EXPECT_TRUE(P6.IsSameAs(P7, distThreshold, DEG_TO_RAD(5))); // Should match even with tight angle threshold
  
} // TestPose:IsSameAs


GTEST_TEST(TestPose, IsSameWithAmbiguity)
{
  const Pose3d origin;
  Pose3d P_ref(M_PI/2, {0,1.f,0}, {10.f,20.f,30.f}, origin);
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
  
  RotationAmbiguities ambiguities(true, {
    RotationMatrix3d({1,0,0,  0,1,0,  0,0,1}),
    RotationMatrix3d({0,1,0,  0,0,1,  1,0,0}),
    RotationMatrix3d({0,0,1,  1,0,0,  0,1,0})
  });
  
  // The IsSameAs_WithAmbiguity function should see these two poses as the same,
  // though it is our job to factor out the reference pose from each, by post-
  // multiplying by its inverse.
  Pose3d P1_mod(P1);
  P1_mod *= P_ref.GetInverse(); // Use in-place multiplication to preserve origin
  Pose3d P2_mod(P2);
  P2_mod *= P_ref.GetInverse(); //  "
  
  EXPECT_TRUE( P1_mod.IsSameAs_WithAmbiguity(P2_mod,
                                             ambiguities, 5.f,
                                             5*M_PI/180.f) );
  
  // The symmetric test should also pass
  EXPECT_TRUE( P2_mod.IsSameAs_WithAmbiguity(P1_mod,
                                             ambiguities, 5.f,
                                             5*M_PI/180.f) );
  
}




