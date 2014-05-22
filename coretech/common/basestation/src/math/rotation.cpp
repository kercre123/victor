//
//  rotation.cpp
//  CoreTech_Math
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/common/basestation/math/rotation.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/calib3d/calib3d.hpp"
#endif

namespace Anki {
  
  
  RotationMatrix2d::RotationMatrix2d(const Radians angle)
  {
    const float cosAngle = std::cos(angle.ToFloat());
    const float sinAngle = std::sin(angle.ToFloat());
    
    (*this)(0,0) =  cosAngle;
    (*this)(1,0) =  sinAngle;
    (*this)(0,1) = -sinAngle;
    (*this)(1,1) =  cosAngle;
  }

  
  RotationVector3d::RotationVector3d(void)
  : angle(0.f), axis(X_AXIS_3D)
  {
    
  }
  
  RotationVector3d::RotationVector3d(const Radians angleIn, const Vec3f &axisIn)
  : angle(angleIn), axis(axisIn)
  {
    f32 axisLength = axis.MakeUnitLength();
    if(axisLength != 1.f) {
      // TODO: Issue a warning that axis provided was not unit length?
    }
  }
  
  RotationVector3d::RotationVector3d(const Vec3f &rvec)
  : axis(rvec)
  {
    this->angle = axis.MakeUnitLength();
    if(this->angle == Radians(0)) {
      // If the specified vector was all zero, then the axis is ambiguous,
      // so default to the X axis
      axis = X_AXIS_3D;
    }
  }
  
  RotationVector3d::RotationVector3d(const RotationMatrix3d &Rmat)
  : RotationVector3d()
  {
    Rodrigues(Rmat, *this);
  }
  
  
  UnitQuaternion::UnitQuaternion(const RotationVector3d& Rvec)
  {
    const float halfAngle = Rvec.get_angle().ToFloat() * 0.5f;
    this->data[0] = std::cos(halfAngle);
    
    const float sinHalfAngle = std::sin(halfAngle);
    this->data[1] = Rvec.get_axis().x() * sinHalfAngle;
    this->data[2] = Rvec.get_axis().y() * sinHalfAngle;
    this->data[3] = Rvec.get_axis().z() * sinHalfAngle;
  }
  
  RotationMatrix3d::RotationMatrix3d(void)
//  : rotationVector(0.f, {Z_AXIS_3D})
  {
    (*this)(0,0) = 1.f;
    (*this)(0,1) = 0.f;
    (*this)(0,2) = 0.f;
    
    (*this)(1,0) = 0.f;
    (*this)(1,1) = 1.f;
    (*this)(1,2) = 0.f;
  
    (*this)(2,0) = 0.f;
    (*this)(2,1) = 0.f;
    (*this)(2,2) = 1.f;
    
  } // Constructor: RotationMatrix3d()
  
  RotationMatrix3d::RotationMatrix3d(const RotationVector3d &rotVec_in)
//  : rotationVector(rotVec_in)
  {
    Rodrigues(rotVec_in, *this);
  }
  
  RotationMatrix3d::RotationMatrix3d(const Matrix_3x3f &matrix3x3)
  : Matrix_3x3f(matrix3x3)
  {
    CORETECH_ASSERT(std::abs(this->GetColumn(0).Length() - 1.f) < 1e-6f &&
                    std::abs(this->GetColumn(1).Length() - 1.f) < 1e-6f &&
                    std::abs(this->GetColumn(2).Length() - 1.f) < 1e-6f);
  }
  
  
  RotationMatrix3d::RotationMatrix3d(std::initializer_list<float> initValues)
  : Matrix_3x3f(initValues)
  {
    CORETECH_ASSERT(std::abs(this->GetColumn(0).Length() - 1.f) < 1e-6f &&
                    std::abs(this->GetColumn(1).Length() - 1.f) < 1e-6f &&
                    std::abs(this->GetColumn(2).Length() - 1.f) < 1e-6f);
    
    //Rodrigues(*this, rotationVector);
  }
                                  
  
  
  RotationMatrix3d::RotationMatrix3d(const Radians angle, const Vec3f &axis)
  //: rotationVector(angle, axis)
  {
    RotationVector3d Rvec(angle, axis);
    Rodrigues(Rvec, *this);
  }
  
  /* Isn't this inherited from the base class now?
  Point3f RotationMatrix3d::operator*(const Point3f &p) const
  {
#if ANKICORETECH_USE_OPENCV
    // TODO: pretty sure there's a better way to do this:
    cv::Matx<float,3,1> vec(p.x, p.y, p.z);
    cv::Matx<float,3,1> out(this->get_CvMatx_() * vec);
    Point3f rotatedPoint(out(0,0), out(1,0), out(2,0));
#else
    assert(false);
#endif
    
    return rotatedPoint;
    
  } // RotationMatrix3d::operator*(Point3f)
  */
  
  
  Radians RotationMatrix3d::GetAngleDiffFrom(const RotationMatrix3d &other) const
  {
    // See also:
    // "Metrics for 3D Rotations: Comparison and Analysis", Du Q. Huynh
    //
    // TODO: use quaternion difference?  Might be cheaper than the matrix multiplication below
    //     UnitQuaternion qThis(this->rotationVector), qOther(otherPose.rotationVector);
    //
    // const Radians angleDiff( 2.f*std::acos(std::abs(DotProduct(qThis, qOther))) );
    //
    
    // R = R_this * R_other^T
    RotationMatrix3d R;
    other.GetTranspose(R);
    R.preMultiplyBy(*this);
    
    //const Radians angleDiff( std::acos(0.5f*(R.Trace() - 1.f)) );
    return R.GetAngle();
  }
  
  Radians RotationMatrix3d::GetAngle() const
  {
    return Radians( std::acos(0.5f*(this->Trace() - 1.f)) );
  }
  
  /*
  // This is not working yet: perhaps because of a different definition of
  // angle signs implicit in the way I did Euler angles inside of planar6dof
  // tracker on the robot -- which is where this code originated.
  void RotationMatrix3d::GetEulerAngles(Radians& angle_x, Radians& angle_y, Radians& angle_z) const
  {
    const f32 R20 = (*this)(2,0);
    
    if(fabs(fabs(R20) - 1.f) < 1e-6f) { // TODO: change to some utility macro/function like FLT_NEAR
      
      angle_z = 0.f;
      const f32 R01 = (*this)(0,1);
      const f32 R02 = (*this)(0,2);
      
      if(R20 > 0) { // R(2,0) = +1
        angle_y = M_PI_2;
        angle_x = atan2f(R01, R02);
      } else { // R(2,0) = -1
        angle_y = -M_PI_2;
        angle_x = atan2f(-R01, -R02);
      }
      
    } else {
      angle_y = -asinf(R20);
      
      const f32 R21 = (*this)(2,1);
      const f32 R22 = (*this)(2,2);
      const f32 R10 = (*this)(1,0);
      const f32 R00 = (*this)(0,0);
      
      const f32 inv_cy = 1.f / cosf(angle_y.ToFloat());
      angle_x = -atan2f(-R21*inv_cy, R22*inv_cy);
      angle_z = -atan2f(-R10*inv_cy, R00*inv_cy);
      
    }
    
  } // RotationMatrix3d::GetEulerAngles()
   */
  

  Radians RotationMatrix3d::GetAngleAroundXaxis() const
  {
    // Apply the matrix to the Y axis and see where its (y,z) coordinates
    // end up.
    const f32 R11 = (*this)(1,1);
    const f32 R21 = (*this)(2,1);
    return Radians( atan2f(R21, R11));
    
  } // RotationMatrix3d::GetAngleAroundYaxis()
  
  
  Radians RotationMatrix3d::GetAngleAroundYaxis() const
  {
    // Apply the matrix to the X axis and see where its (x,z) coordinates
    // end up.
    const f32 R20 = (*this)(2,0);
    const f32 R00 = (*this)(0,0);
    return Radians( atan2f(-R20, R00));
    
  } // RotationMatrix3d::GetAngleAroundYaxis()
  
  
  Radians RotationMatrix3d::GetAngleAroundZaxis() const
  {
    // Apply the matrix to the X axis and see where its (x,y) coordinates
    // end up.
    const f32 R10 = (*this)(1,0);
    const f32 R00 = (*this)(0,0);
    return Radians( atan2f(R10, R00));
    
  } // RotationMatrix3d::GetAngleAroundZaxis()
  
  
  void Rodrigues(const RotationVector3d &Rvec_in,
                 RotationMatrix3d &Rmat_out)
  {
#if ANKICORETECH_USE_OPENCV
    cv::Vec3f cvRvec(Rvec_in.get_axis().get_CvPoint3_());
    cvRvec *= Rvec_in.get_angle().ToFloat();
    cv::Rodrigues(cvRvec, Rmat_out.get_CvMatx_());    
#else
    
    assert(false);
    // TODO: implement our own version?
#endif
    
  } // Rodrigues(Rvec, Rmat)
  
  
  void Rodrigues(const RotationMatrix3d &Rmat_in,
                 RotationVector3d &Rvec_out)
  {
   
#if ANKICORETECH_USE_OPENCV
    cv::Vec3f cvRvec;
    cv::Rodrigues(Rmat_in.get_CvMatx_(), cvRvec);
    Anki::Vec3f tempVec(cvRvec);
    Rvec_out = RotationVector3d(tempVec);
#else
    assert(false);
    
    // TODO: implement our own version?
    
#endif
    
  } // Rodrigues(Rmat, Rvec)
  
  /*
  void RotationMatrix3d::operator*=(const RotationMatrix3d &other)
  {
    // Regular matrix multiplcation
    Matrix_3x3f::operator*=(other);
    
    // Keep the rotation vector updated
    Rodrigues(*this, this->rotationVector);
  }
  
  void RotationMatrix3d::preMultiplyBy(const RotationMatrix3d &other)
  {
    // Regular matrix multiplication
    Matrix_3x3f::preMultiplyBy(other);
    
    // Keep the rotation vector updated
    Rodrigues(*this, this->rotationVector);
  }
   */
  
  RotationMatrix3d& RotationMatrix3d::Transpose(void)
  {
    Matrix_3x3f::Transpose();
   
    /*
    // Just negate rotation angle:
    Radians angle;
    Vec3f   axis;
    rotationVector.get_angleAndAxis(angle, axis);
    rotationVector = RotationVector3d(-angle, axis);
    */
    return *this;
  } // Transpose()
  
  void RotationMatrix3d::GetTranspose(RotationMatrix3d& outTransposed) const
  {
    outTransposed = *this;
    outTransposed.Transpose();
  }
  
  RotationMatrix3d& RotationMatrix3d::Invert(void)
  {
    this->Transpose();
    return *this;
  }
  
  void RotationMatrix3d::GetInverse(RotationMatrix3d& outInverted) const
  {
    outInverted = *this;
    outInverted.Invert();
  }
  
  
  
} // namespace Anki
