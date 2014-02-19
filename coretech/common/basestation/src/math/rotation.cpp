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
    f32 axisLength = axis.makeUnitLength();
    if(axisLength != 1.f) {
      // TODO: Issue a warning that axis provided was not unit length?
    }
  }
  
  RotationVector3d::RotationVector3d(const Vec3f &rvec)
  : axis(rvec)
  {
    this->angle = axis.makeUnitLength();
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
    CORETECH_ASSERT(std::abs(this->getColumn(0).length() - 1.f) < 1e-6f &&
                    std::abs(this->getColumn(1).length() - 1.f) < 1e-6f &&
                    std::abs(this->getColumn(2).length() - 1.f) < 1e-6f);
  }
  
  
  RotationMatrix3d::RotationMatrix3d(std::initializer_list<float> initValues)
  : Matrix_3x3f(initValues)
  {
    CORETECH_ASSERT(std::abs(this->getColumn(0).length() - 1.f) < 1e-6f &&
                    std::abs(this->getColumn(1).length() - 1.f) < 1e-6f &&
                    std::abs(this->getColumn(2).length() - 1.f) < 1e-6f);
    
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
    // const Radians angleDiff( 2.f*std::acos(std::abs(dot(qThis, qOther))) );
    //
    
    // R = R_this * R_other^T
    RotationMatrix3d R(other.getTranspose());
    R.preMultiplyBy(*this);
    
    const Radians angleDiff( std::acos(0.5f*(R.Trace() - 1.f)) );
    
    return angleDiff;
  }
  
  
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
  
  RotationMatrix3d  RotationMatrix3d::getTranspose() const
  {
    RotationMatrix3d Rt(*this);
    Rt.Transpose();
    return Rt;
  }
  
  void RotationMatrix3d::Invert(void)
  {
    this->Transpose();
  }
  
  RotationMatrix3d RotationMatrix3d::getInverse(void) const
  {
    return this->getTranspose();
  }
  
  
  
} // namespace Anki
