//
//  rotation.cpp
//  CoreTech_Math
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/common/basestation/math/rotation.h"

#include "anki/common/basestation/math/matrix_impl.h"
#include "anki/common/basestation/math/point_impl.h"

#include "anki/common/shared/utilities_shared.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/calib3d/calib3d.hpp"
#endif

namespace Anki {
  
#if 0
#pragma mark --- RotationMatrixBase ----
#endif
  
  template<MatDimType DIM>
  RotationMatrixBase<DIM>::RotationMatrixBase(void)
  {
    for(s32 i=0; i<DIM; ++i) {
      for(s32 j=0; j<DIM; ++j) {
        (*this)(i,j) = static_cast<float>(i==j);
      }
    }
  } // Constructor: RotationMatrixBase()
  
  void RenormalizeHelper(RotationMatrixBase<2>& R) {
    // TODO: implement
    assert(false);
  }
  
  void RenormalizeHelper(RotationMatrixBase<3>& R)
  {
    // Convert to and from a rotation vector to get a valid orthogonal matrix
    // TODO: consider using Quaternions instead?
#if ANKICORETECH_USE_OPENCV
    cv::Vec3f cvRvec;
    cv::Rodrigues(R.get_CvMatx_(), cvRvec); // to rotation vector
    cv::Rodrigues(cvRvec, R.get_CvMatx_()); // from rotation vector
#else
    assert(false);
#endif
  }
  
  template<MatDimType DIM>
  void RotationMatrixBase<DIM>::Renormalize()
  {
   
    bool needsRenormalization = false;
    
    for(s32 i=0; i<DIM; ++i) {
      
      const f32 rowNorm = this->GetRow(i).Length();
      
      // Check if the row norm is crazy
      // TODO: Somehow throw an error?
      if(!NEAR(rowNorm, 1.f, RotationMatrixBase<DIM>::OrthogonalityToleranceHigh)) {
        CoreTechPrint("Norm of row %d = %f! (Expecting near 1.0) Row = [ ", i, rowNorm);
        for(s32 j=0; j<DIM; ++j) {
          CoreTechPrint("%d ", this->operator()(i,j));
        }
        CoreTechPrint("]\n");
        needsRenormalization = true;
        break;
      }
      
      // If the row norm isn't crazy, but has gotten outside our tolerances
      // then renormalize the matrix
      if(!NEAR(rowNorm, 1.f, RotationMatrixBase<DIM>::OrthogonalityToleranceLow)) {
        needsRenormalization = true;
        break;
      }
      
    }
    
    if(needsRenormalization) {
      CoreTechPrint("Renormalizing a %dD rotation matrix.\n", DIM);
      RenormalizeHelper(*this);
    }
  }
  
  template<MatDimType DIM>
  bool RotationMatrixBase<DIM>::IsValid(const float tolerance) const
  {
    bool isValid = DIM > 0;
    for(s32 i=0; isValid && i<DIM; ++i) {
      isValid &= NEAR(this->GetColumn(i).Length(), 1.f, tolerance);
    }
    
    return isValid;
  }
  
  template<MatDimType DIM>
  RotationMatrixBase<DIM> RotationMatrixBase<DIM>::operator* (const RotationMatrixBase<DIM>& R_other) const
  {
    RotationMatrixBase<DIM> Rnew( SmallSquareMatrix<DIM,float>::operator*(R_other) );
    return Rnew;
  }
  
  template<MatDimType DIM>
  RotationMatrixBase<DIM>& RotationMatrixBase<DIM>::operator*=(const RotationMatrixBase<DIM>& R_other)
  {
    SmallSquareMatrix<DIM,float>::operator*=(R_other);
    this->Renormalize();
    return *this;
  }
  
  template<MatDimType DIM>
  RotationMatrixBase<DIM>& RotationMatrixBase<DIM>::preMultiplyBy(const RotationMatrixBase<DIM>& R_other)
  {
    SmallSquareMatrix<DIM,float>::preMultiplyBy(R_other);
    this->Renormalize();
    return *this;
  }
  
  
  template<MatDimType DIM>
  RotationMatrixBase<DIM>::RotationMatrixBase(const SmallSquareMatrix<DIM, float> &matrix)
  : SmallSquareMatrix<DIM, float>(matrix)
  {
    CORETECH_ASSERT(this->IsValid());
    this->Renormalize();
  }
  
  template<MatDimType DIM>
  RotationMatrixBase<DIM>::RotationMatrixBase(std::initializer_list<float> initValues)
  : SmallSquareMatrix<DIM,float>(initValues)
  {
    CORETECH_ASSERT(this->IsValid());
  }
  
  template<MatDimType DIM>
  RotationMatrixBase<DIM>& RotationMatrixBase<DIM>::Transpose(void)
  {
    SmallSquareMatrix<DIM, float>::Transpose();
    return *this;
  } // Transpose()
  
  template<MatDimType DIM>
  void RotationMatrixBase<DIM>::GetTranspose(RotationMatrixBase<DIM>& outTransposed) const
  {
    outTransposed = *this;
    outTransposed.Transpose();
  }
  
  template<MatDimType DIM>
  RotationMatrixBase<DIM>& RotationMatrixBase<DIM>::Invert(void)
  {
    this->Transpose();
    return *this;
  }
  
  template<MatDimType DIM>
  void RotationMatrixBase<DIM>::GetInverse(RotationMatrixBase<DIM>& outInverted) const
  {
    outInverted = *this;
    outInverted.Invert();
  }

  
  // Explicit instantiation of 2D and 3D base class
  template class RotationMatrixBase<2>;
  template class RotationMatrixBase<3>;
  
#if 0
#pragma mark --- RotationMatrix2d ----
#endif
  
  RotationMatrix2d::RotationMatrix2d()
  : RotationMatrixBase<2>()
  {
    
  }
  
  RotationMatrix2d::RotationMatrix2d(std::initializer_list<float> initVals)
  : RotationMatrixBase<2>(initVals)
  {
    
  }
  
  RotationMatrix2d::RotationMatrix2d(const Radians angle)
  {
    const float cosAngle = std::cos(angle.ToFloat());
    const float sinAngle = std::sin(angle.ToFloat());
    
    (*this)(0,0) =  cosAngle;
    (*this)(1,0) =  sinAngle;
    (*this)(0,1) = -sinAngle;
    (*this)(1,1) =  cosAngle;
  }

  RotationMatrix2d::RotationMatrix2d(const Matrix_2x2f &matrix2x2)
  : RotationMatrixBase<2>(matrix2x2)
  {
    
  }
  

#if 0
#pragma mark --- RotationVector3d ---
#endif
  
  RotationVector3d::RotationVector3d(void)
  : _angle(0.f)
  , _axis(X_AXIS_3D)
  {
    
  }
  
  RotationVector3d::RotationVector3d(const Radians angleIn, const Vec3f &axisIn)
  : _angle(angleIn)
  , _axis(axisIn)
  {
    f32 axisLength = _axis.MakeUnitLength();
    if(axisLength != 1.f) {
      // TODO: Issue a warning that axis provided was not unit length?
    }
  }
  
  RotationVector3d::RotationVector3d(const Vec3f &rvec)
  : _axis(rvec)
  {
    _angle = _axis.MakeUnitLength();
    if(_angle == Radians(0)) {
      // If the specified vector was all zero, then the axis is ambiguous,
      // so default to the X axis
      _axis = X_AXIS_3D;
    }
  }
  
  RotationVector3d::RotationVector3d(const RotationMatrix3d &Rmat)
  : RotationVector3d()
  {
    Rodrigues(Rmat, *this);
  }

  bool RotationVector3d::operator==(const RotationVector3d &other) const
  {
    return ( (_axis == other._axis && _angle == other._angle)
            || (-_axis == other._axis && -_angle == other._angle)
            || (_angle == other._angle == 0));
  }
  
  
#if 0
#pragma mark --- Rotation3d ---
#endif
  
  Rotation3d::Rotation3d(const Radians& angle, const Vec3f& axis)
  {
    const f32 halfAngle = angle.ToFloat() * 0.5f;
    const f32 q1 = std::cos(halfAngle);
    
    const f32 sinHalfAngle = std::sin(halfAngle);
    const f32 q2 = sinHalfAngle * axis[0];
    const f32 q3 = sinHalfAngle * axis[1];
    const f32 q4 = sinHalfAngle * axis[2];
    
    _q = {q1, q2, q3, q4};
  }
  
  Rotation3d::Rotation3d(const RotationVector3d& Rvec)
  : Rotation3d(Rvec.GetAngle(), Rvec.GetAxis())
  {
    
  }

  Rotation3d::Rotation3d(const RotationMatrix3d& Rmat)
  : Rotation3d(RotationVector3d(Rmat))
  {
    
  }
  
  const Radians Rotation3d::GetAngle() const
  {
    return 2.f*std::acos(_q[0]);
  }
  
  const Vec3f Rotation3d::GetAxis() const
  {
    Vec3f axis(_q[1], _q[2], _q[3]);
    axis.MakeUnitLength();
    return axis;
  }
  
  const RotationMatrix3d Rotation3d::GetRotationMatrix() const
  {
    return RotationMatrix3d(GetAngle(), GetAxis());
  }
  
  const RotationVector3d Rotation3d::GetRotationVector() const
  {
    return RotationVector3d(GetAngle(), GetAxis());
  }
  
  Rotation3d& Rotation3d::operator*=(const Rotation3d& other)
  {
    _q *= other._q;
    return *this;
  }
  
  Rotation3d Rotation3d::operator*(const Rotation3d& other) const
  {
    Rotation3d retVal(*this);
    retVal *= other;
    return retVal;
  }
  
  bool IsNearlyEqual(const Rotation3d& R1, const Rotation3d& R2, const f32 tolerance)
  {
    return IsNearlyEqual(R1.GetQuaternion(), R2.GetQuaternion(), tolerance);
  }
  
#if 0
#pragma mark --- UnitQuaternion ----
#endif
  
  template<typename T>
  T UnitQuaternion<T>::NormalizationTolerance = 1000.f * std::numeric_limits<T>::epsilon();
  
  template<typename T>
  UnitQuaternion<T>::UnitQuaternion()
  : Point<4,T>(1,0,0,0)
  {
    
  }

  template<typename T>
  UnitQuaternion<T>::UnitQuaternion(const UnitQuaternion& other)
  : Point<4, T>(other)
  {
    
  }
  
  template<typename T>
  UnitQuaternion<T>::UnitQuaternion(const T w, const T x, const T y, const T z)
  : Point<4,T>(w,x,y,z)
  {
    this->Normalize();
  }
  
  template<typename T>
  UnitQuaternion<T>& UnitQuaternion<T>::Normalize()
  {
    if(Point<4,T>::MakeUnitLength() == 0) {
      CORETECH_THROW("Tried to normalize an all-zero UnitQuaternion.\n");
    }
    
    return *this;
  }
  
  template<typename T>
  UnitQuaternion<T> UnitQuaternion<T>::operator*(const UnitQuaternion<T>& other) const
  {
    UnitQuaternion output(*this);
    output *= other;
    return output;
  }
  
  template<typename T>
  UnitQuaternion<T>& UnitQuaternion<T>::operator*=(const UnitQuaternion<T>& other)
  {
    const T wNew = (this->w()*other.w()) - (this->x()*other.x()) - (this->y()*other.y()) - (this->z()*other.z());
    const T xNew = (this->w()*other.x()) + (this->x()*other.w()) + (this->y()*other.z()) - (this->z()*other.y());
    const T yNew = (this->w()*other.y()) - (this->x()*other.z()) + (this->y()*other.w()) + (this->z()*other.x());
    const T zNew = (this->w()*other.z()) + (this->x()*other.y()) - (this->y()*other.x()) + (this->z()*other.w());
    
    this->w() = wNew;
    this->x() = xNew;
    this->y() = yNew;
    this->z() = zNew;
    
    const T newLength = Point<4,T>::Length();
    if(!NEAR(newLength, 1.f, UnitQuaternion<T>::NormalizationTolerance)) {
      Point<4,T>::operator/=(newLength);
    }
    
    return *this;
  }
  
  // Explicit instantiation for single and double precision
  template class UnitQuaternion<float_t>;
  template class UnitQuaternion<double_t>;
  
#if 0
#pragma mark --- RotationMatrix 3d ----
#endif
  
  
  RotationMatrix3d::RotationMatrix3d(const Radians angle, const Vec3f &axis)
  //: rotationVector(angle, axis)
  {
    RotationVector3d Rvec(angle, axis);
    Rodrigues(Rvec, *this);
  }

  
  RotationMatrix3d::RotationMatrix3d(const RotationVector3d &rotVec_in)
  {
    Rodrigues(rotVec_in, *this);
  }
  
  RotationMatrix3d::RotationMatrix3d()
  : RotationMatrixBase<3>()
  {
    
  }
  
  RotationMatrix3d::RotationMatrix3d(const Matrix_3x3f &matrix3x3)
  : RotationMatrixBase<3>(matrix3x3)
  {
    
  }
  
  RotationMatrix3d::RotationMatrix3d(std::initializer_list<float> initVals)
  : RotationMatrixBase<3>(initVals)
  {
    
  }
  
  
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
  
  
#if 0
#pragma mark --- Rodrigues Functions ---
#endif
  
  void Rodrigues(const RotationVector3d &Rvec_in,
                 RotationMatrix3d &Rmat_out)
  {
#if ANKICORETECH_USE_OPENCV
    cv::Vec3f cvRvec(Rvec_in.GetAxis().get_CvPoint3_());
    cvRvec *= Rvec_in.GetAngle().ToFloat();
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
  
  
} // namespace Anki
