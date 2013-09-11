#include "anki/common/datastructures.h"

#include "anki/math/pose.h"
#include "anki/math/matrix.h"

#include <stdexcept>

namespace Anki {
  
#pragma mark --- Pose2d Implementations ---
  
  float Pose2d::get_x(void) const
  {
    return this->x;
  }
  
  float Pose2d::get_y(void) const
  {
    return this->y;
  }
  
  Point2f Pose2d::get_xy(void) const
  {
    return Point2f(this->x, this->y);
  }
  
  Radians Pose2d::get_angle(void) const
  {
    return this->angle;
  }
  
  RotationMatrix2d Pose2d::get_rotationMatrix(void) const
  {

    RotationMatrix2d rotMat(this->angle);
    
    return rotMat;
    
  } // Pose2d::get_rotationMatrix()
  
#pragma mark --- Pose3d Implementations ---
  
  Pose3d* Pose3d::World = NULL;
  
  Pose3d::Pose3d()
  : translation(0.f, 0.f, 0.f), parent(NULL), treeDepth(0)
  {
    
  } // Constructor: Pose3d()  
  
  Pose3d::Pose3d(const RotationVector3d &Rvec_in, const Vec3f &T_in, const Pose3d *parentPose)
  : rotationVector(Rvec_in), translation(T_in),
    rotationMatrix(rotationVector), parent(parentPose)
  {
    if(parentPose != NULL) {
      treeDepth = parentPose->treeDepth + 1;
    }
  } // Constructor: Pose3d(Rvec, T)
  
  Pose3d::Pose3d(const RotationVector3d &Rvec_in, const Vec3f &T_in,
             const Matrix<float> &cov_in, const Pose3d *parentPose)
  : Pose3d(Rvec_in, T_in, parentPose)
  {
    covariance = cov_in;
  } // Constructor: Pose3d(Rvec, T, cov)
  
  Pose3d::Pose3d(const RotationMatrix3d &Rmat_in, const Vec3f &T_in, const Pose3d *parentPose)
  : rotationMatrix(Rmat_in), translation(T_in), rotationVector(rotationMatrix), parent(parentPose)
  {
    if(parentPose != NULL) {
      treeDepth = parentPose->treeDepth + 1;
    }
  } // Constructor: Pose3d(Rmat, T)
  
  
#pragma mark --- Operator Overloads ---
  void Pose3d::operator*=(const Pose3d &other)
  {
    // TODO: Implement [this.Rmat this.t] * [other.Rmat other.t]
    
  } // operatore*=(Pose3d)
  
  Pose3d Pose3d::operator*(const Pose3d &other) const
  {
    // TODO: Implement [this.Rmat this.t] * [other.Rmat other.t]

  } // operator*(Pose3d)
  
  
#pragma mark --- Member Methods ---
  Pose3d Pose3d::getInverse(void) const
  {
    // TODO: Return inverse(this)
    Pose3d returnPose(*this);
    returnPose.Invert();
    return returnPose;
  }
  
  void Pose3d::Invert(void)
  {
    this->rotationMatrix.getTranspose();
    this->translation *= -1.f;
    this->translation = this->rotationMatrix * this->translation;
  }
  
  Pose3d Pose3d::getWithRespectTo(const Anki::Pose3d *otherPose) const
  {
    const Pose3d *current = this->parent;
    while(current != otherPose) {
      
      // TODO: Chain pose inversions together here as we work our way to parent
      
      if(current->parent == NULL) {
        // TODO: replace this with some "approved" error handling method
        throw std::runtime_error("Parent pose not found.");
      }
    }
    
  } // Pose3d::withRespectTo()
  
} // namespace Anki
