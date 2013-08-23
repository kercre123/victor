#include "anki/common/datastructures.h"

#include "anki/math/basicMath.h"
#include "anki/math/pose.h"
#include "anki/math/matrix.h"

namespace Anki {
  
#pragma mark --- Constructors ---
  
  Pose::Pose(const Vec3f  &Rvec_in, const Vec3f &T_in)
  : Rvector(Rvec_in), translation(T_in)
  {
    if (Rodrigues(this->Rvector, this->Rmatrix) != RESULT_OK) {
      // TODO: what to do if this fails? Throw exception??
    }
  } // Constructor: Pose(Rvec, T) 
  
  Pose::Pose(const Vec3f  &Rvec_in, const Vec3f &T_in,
             const Matrix<float> &cov_in)
  : Pose(Rvec_in, T_in)
  {
    covariance = cov_in;
  } // Constructor: Pose(Rvec, T, cov)
  
  Pose::Pose(const Matrix<float> &Rmat_in, const Vec3f &T_in)
  : Rmatrix(Rmat_in), translation(T_in)
  {
    if (Rodrigues(this->Rmatrix, this->Rvector) != RESULT_OK) {
      // TODO: what to do if this fails? Throw exception??
    }
  } // Constructor: Pose(Rmat, T)
  
  Pose::Pose(const float angle, const Vec3f &axis, const Vec3f &T)
  : Rvector(axis)
  {
    // TODO: should this be an error / exception?
    assert(axis.length()==1.f); //"Axis should be a unit vector.");
    
    // Rvector already contains the axis, multiply in the angle
    this->Rvector *= angle;
    
    if (Rodrigues(this->Rvector, this->Rmatrix) != RESULT_OK) {
      // TODO: what to do if this fails? Throw exception??
    }
    
  } // Constructor: Pose(angle, axis, T)
  
  Pose::Pose(const float angle, const Vec3f &axis, const Vec3f &T,
             const Matrix<float> &cov_in)
  : Pose(angle, axis, T)
  {
    covariance = cov_in;
  } // Constructor: Pose(angle, axis, T, cov)
  
#pragma mark --- Operator Overloads ---
  void Pose::operator*=(const Pose &other)
  {
    // TODO: Implement [this.Rmat this.t] * [other.Rmat other.t]
    
  } // operatore*=(Pose)
  
  Pose Pose::operator*(const Pose &other) const
  {
    // TODO: Implement [this.Rmat this.t] * [other.Rmat other.t]

  } // operator*(Pose)
  
  
#pragma mark --- Member Methods ---
  Pose Pose::inverse(void) const
  {
    // TODO: Return inverse(this)
    Pose returnPose(*this);
    returnPose.inverse();
    return returnPose;
  }
  
  void Pose::inverse(void)
  {
     // TODO: Set this to inverse(this)
  }
  
} // namespace Anki
