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
      //CoreTechPrint("Renormalizing a %dD rotation matrix.\n", DIM);
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
  RotationMatrixBase<DIM>& RotationMatrixBase<DIM>::PreMultiplyBy(const RotationMatrixBase<DIM>& R_other)
  {
    SmallSquareMatrix<DIM,float>::PreMultiplyBy(R_other);
    this->Renormalize();
    return *this;
  }
  
  
  template<MatDimType DIM>
  RotationMatrixBase<DIM>::RotationMatrixBase(const SmallSquareMatrix<DIM, float> &matrix)
  : SmallSquareMatrix<DIM, float>(matrix)
  {
    this->Renormalize();
  }
  
  template<MatDimType DIM>
  RotationMatrixBase<DIM>::RotationMatrixBase(std::initializer_list<float> initValues)
  : SmallSquareMatrix<DIM,float>(initValues)
  {
    this->Renormalize();
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
  , _axis(X_AXIS_3D())
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
      _axis = X_AXIS_3D();
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
    
    assert(NEAR(axis.LengthSq(), 1.f, 1e-5));
    
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
  
  Rotation3d::Rotation3d(const UnitQuaternion<float>& q)
  : _q(q)
  {
    
  }
  
  const Radians Rotation3d::GetAngle() const
  {
    return 2.f*std::acos(_q[0]);
  }
  
  Radians Rotation3d::GetAngleDiffFrom(const Rotation3d& otherRotation) const
  {
    const UnitQuaternion<float>& q_this = this->GetQuaternion();
    const UnitQuaternion<float>& q_other = otherRotation.GetQuaternion();
    
    const f32 innerProd = std::min(1.f, std::max(-1.f, (q_this.w()*q_other.w() +
                                                        q_this.x()*q_other.x() +
                                                        q_this.y()*q_other.y() +
                                                        q_this.z()*q_other.z())));
    
    Radians angleDiff(std::acos(2.f * innerProd*innerProd - 1.f));
    
    return angleDiff;
  }
  
  Radians Rotation3d::GetAngleAroundXaxis() const
  {
    // Apply the quaternion to the Y axis and see where its (y,z) coordinates
    // end up.
    const float w2 = _q.w()*_q.w();
    const float x2 = _q.x()*_q.x();
    const float y2 = _q.y()*_q.y();
    const float z2 = _q.z()*_q.z();
    const float yz = _q.y()*_q.z();
    const float wx = _q.w()*_q.x();
    
    const float Ynew = w2-x2+y2-z2;
    const float Znew = 2*yz+2*wx;
    
    return Radians(atan2f(Znew, Ynew));
  }
  
  Radians Rotation3d::GetAngleAroundYaxis() const
  {
    // Apply the matrix to the X axis and see where its (x,z) coordinates
    // end up.
    const float w2 = _q.w()*_q.w();
    const float x2 = _q.x()*_q.x();
    const float y2 = _q.y()*_q.y();
    const float z2 = _q.z()*_q.z();
    const float xz = _q.x()*_q.z();
    const float wy = _q.w()*_q.y();
    
    const float Xnew = w2+x2-y2-z2;
    const float Znew = 2*xz-2*wy;

    return Radians(atan2f(-Znew, Xnew));
  }
  

  
  Radians Rotation3d::GetAngleAroundZaxis() const
  {
    // Apply the quaternion to the X axis and see where its (x,y) coordinates
    // end up.
    const float w2 = _q.w()*_q.w();
    const float x2 = _q.x()*_q.x();
    const float y2 = _q.y()*_q.y();
    const float z2 = _q.z()*_q.z();
    const float xy = _q.x()*_q.y();
    const float wz = _q.w()*_q.z();
    
    const float Xnew = w2+x2-y2-z2;
    const float Ynew = 2*xy+2*wz;
    
    return Radians(atan2f(Ynew, Xnew));
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
  
  Rotation3d& Rotation3d::PreMultiplyBy(const Rotation3d& other)
  {
    _q = other._q * _q;
    return *this;
  }
  
  Point3<float> Rotation3d::operator*(const Point3<float>& p) const
  {
    return _q*p;
  }
  
  bool IsNearlyEqual(const Rotation3d& R1, const Rotation3d& R2, const f32 tolerance)
  {
    return IsNearlyEqual(R1.GetQuaternion(), R2.GetQuaternion(), tolerance);
  }
  
  Rotation3d& Rotation3d::Invert()
  {
    _q.Conj();
    return *this;
  }
  
  Rotation3d Rotation3d::GetInverse() const
  {
    Rotation3d R(*this);
    R._q.Conj();
    return R;
  }
  
  bool Rotation3d::operator==(const Rotation3d &other) const
  {
    return _q == other._q;
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
  UnitQuaternion<T>& UnitQuaternion<T>::Conj()
  {
    // (w stays same)
    x() = -x();
    y() = -y();
    z() = -z();
    return *this;
  }
  
  template<typename T>
  UnitQuaternion<T> UnitQuaternion<T>::GetConj() const
  {
    UnitQuaternion<T> q_new(*this);
    q_new.Conj();
    return q_new;
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
    
    Normalize();
    /*
     // TODO: Only normalize when things get outside some tolerance?
    const T newLength = Point<4,T>::Length();
    if(!NEAR(newLength, 1.f, UnitQuaternion<T>::NormalizationTolerance)) {
      Point<4,T>::operator/=(newLength);
    }
     */
    
    return *this;
  }
  
  template<typename T>
  Point3<T> UnitQuaternion<T>::operator*(const Point3<T>& p) const
  {
    const T w2 = w()*w();
    const T x2 = x()*x();
    const T y2 = y()*y();
    const T z2 = z()*z();
    const T xy = x()*y();
    const T xz = x()*z();
    const T yz = y()*z();
    const T wx = w()*x();
    const T wy = w()*y();
    const T wz = w()*z();
    
    Point3<T> p_out((w2+x2-y2-z2) * p.x() + (2*xy-2*wz)   * p.y() + (2*xz+2*wy)   * p.z(),
                    (2*xy+2*wz)   * p.x() + (w2-x2+y2-z2) * p.y() + (2*yz-2*wx)   * p.z(),
                    (2*xz-2*wy)   * p.x() + (2*yz+2*wx)   * p.y() + (w2-x2-y2+z2) * p.z());
  
    return p_out;
  }
  
  template<typename T>
  bool UnitQuaternion<T>::operator==(const UnitQuaternion<T>& other) const
  {
    return Point<4,T>::operator==(other);
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
  
  RotationMatrix3d::RotationMatrix3d(const Radians& angleX, const Radians& angleY, const Radians& angleZ)
  {
    // For reference: http://staff.city.ac.uk/~sbbh653/publications/euler.pdf
    
    const f32 cY = std::cos(angleY.ToFloat());
    const f32 sY = std::sin(angleY.ToFloat());
    const f32 cX = std::cos(angleX.ToFloat());
    const f32 sX = std::sin(angleX.ToFloat());
    const f32 cZ = std::cos(angleZ.ToFloat());
    const f32 sZ = std::sin(angleZ.ToFloat());
    
    // First row
    operator()(0,0) = cY*cZ;
    operator()(0,1) = sX*sY*cZ - cX*sZ;
    operator()(0,2) = cX*sY*cZ + sX*sZ;
    
    // Second row
    operator()(1,0) = cY*sZ;
    operator()(1,1) = sX*sY*sZ + cX*cZ;
    operator()(1,2) = cX*sY*sZ - sX*cZ;
    
    // Third row
    operator()(2,0) = -sY;
    operator()(2,1) = sX*cY;
    operator()(2,2) = cX*cY;
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
    R.PreMultiplyBy(*this);
    
    //const Radians angleDiff( std::acos(0.5f*(R.Trace() - 1.f)) );
    return R.GetAngle();
  }
  
  Radians RotationMatrix3d::GetAngle() const
  {
    return Radians( std::acos(0.5f*(this->Trace() - 1.f)) );
  }

  bool RotationMatrix3d::GetEulerAngles(Radians& angle_x,
                                        Radians& angle_y,
                                        Radians& angle_z,
                                        Radians* angle_x2,
                                        Radians* angle_y2,
                                        Radians* angle_z2) const
  {
    // For reference: http://staff.city.ac.uk/~sbbh653/publications/euler.pdf
    
    bool inGimbalLock = false;
    
    const f32 R20 = (*this)(2,0);
    
    if(FLT_NEAR(std::abs(R20), 1.f))
    {
      // In gimbal lock, angle Z is arbitrary. Choose 0 by convention.
      inGimbalLock = true;
      angle_z = 0.f;
      
      const f32 R01 = (*this)(0,1);
      const f32 R02 = (*this)(0,2);
      
      if(R20 > 0) { // R(2,0) = +1
        angle_y = -M_PI_2;
        angle_x = std::atan2f(-R01, -R02);
      } else { // R(2,0) = -1
        angle_y = M_PI_2;
        angle_x = std::atan2f(R01, R02);
      }
      
      // Alternate solution is same in this case
      if(angle_x2!=nullptr) {
        *angle_x2 = angle_x;
      }
      if(angle_y2!=nullptr) {
        *angle_y2 = angle_y;
      }
      if(angle_z2!=nullptr) {
        *angle_z2 = angle_z;
      }

    } else {
      angle_y = -std::asinf(R20);
      
      const f32 R21 = (*this)(2,1);
      const f32 R22 = (*this)(2,2);
      const f32 R10 = (*this)(1,0);
      const f32 R00 = (*this)(0,0);
      
      const f32 inv_cy = 1.f / std::cosf(angle_y.ToFloat());
      angle_x = std::atan2f(R21*inv_cy, R22*inv_cy);
      angle_z = std::atan2f(R10*inv_cy, R00*inv_cy);
      
      if(angle_x2 != nullptr) {
        // If any is requested non-null, all must be
        assert(angle_y2 != nullptr && angle_z2 != nullptr);
        *angle_y2 = M_PI - angle_y;
        
        const f32 inv_cy2 = 1.f / std::cos(angle_y2->ToFloat());
        *angle_x2 = std::atan2f(R21*inv_cy2, R22*inv_cy2);
        *angle_z2 = std::atan2f(R10*inv_cy2, R00*inv_cy2);
      }
    }
    
    return inGimbalLock;
  } // RotationMatrix3d::GetEulerAngles()


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
