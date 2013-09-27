#include "anki/common/datastructures.h"

#include "anki/math/pose.h"
#include "anki/math/matrix.h"

#include <stdexcept>

namespace Anki {
    
#pragma mark --- Pose2d Implementations ---
  Pose2d* Pose2d::World = NULL;
  
  Pose2d::Pose2d(const Radians &theta, const Point2f &t)
  : translation(t), angle(theta)
  {
    
  }
  
  Pose2d::Pose2d(const Radians &theta, const float x, const float y)
  : translation(x,y), angle(theta)
  {
    
  }
  
  float Pose2d::get_x(void) const
  {
    return this->translation.x();
  }
  
  float Pose2d::get_y(void) const
  {
    return this->translation.y();
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
  
  void Pose2d::operator*=(const Pose2d &other)
  {
    this->angle += other.angle;
    
    this->translation += this->get_rotationMatrix() * other.translation;
    
  }
  
  Pose2d Pose2d::operator* (const Pose2d &other) const
  {
    Radians newAngle(this->angle + other.angle);
    RotationMatrix2d newRotation(newAngle);
    Point2f newTranslation(newRotation * other.translation);
    newTranslation += this->translation;
    
    return Pose2d(newAngle, newTranslation);
  }
  
  void Pose2d::preComposeWith(const Pose2d &other)
  {
    this->angle += other.angle;

    this->translation = other.get_rotationMatrix() * this->translation;
    this->translation += other.translation;
  }
  
#pragma mark --- Pose3d Implementations ---
  
  Pose3d* Pose3d::World = NULL;
  
  Pose3d::Pose3d()
  : translation(0.f, 0.f, 0.f), parent(Pose3d::World)
  {
    
  } // Constructor: Pose3d()  
  
  Pose3d::Pose3d(const RotationVector3d &Rvec_in, const Vec3f &T_in, const Pose3d *parentPose)
  : rotationVector(Rvec_in),
    rotationMatrix(rotationVector),
    translation(T_in),
    parent(parentPose)
  {

  } // Constructor: Pose3d(Rvec, T)
  
  /* TODO: Add this constructor once we're using covariance
  Pose3d::Pose3d(const RotationVector3d &Rvec_in, const Vec3f &T_in,
             const Matrix<float> &cov_in, const Pose3d *parentPose)
  : Pose3d(Rvec_in, T_in, parentPose)
  {
    covariance = cov_in;
  } // Constructor: Pose3d(Rvec, T, cov)
  */
  
  Pose3d::Pose3d(const RotationMatrix3d &Rmat_in, const Vec3f &T_in, const Pose3d *parentPose)
  : rotationVector(Rmat_in),
    rotationMatrix(Rmat_in),
    translation(T_in),
    parent(parentPose)
  {

  } // Constructor: Pose3d(Rmat, T)
  
  
#pragma mark --- Operator Overloads ---
  // Composition: this = this*other
  void Pose3d::operator*=(const Pose3d &other)
  {
   
    // this.T = this.R*other.T + this.T;
    Vec3f thisTranslation(this->translation); // temp storage
    this->translation = this->rotationMatrix * other.translation;
    this->translation += thisTranslation;

    // this.R = this.R * other.R
    // Note: must do this _after_ the translation update, since that uses this.R
    this->rotationMatrix *= other.rotationMatrix;

  } // operatore*=(Pose3d)
  
  // Composition: new = this * other
  Pose3d Pose3d::operator*(const Pose3d &other) const
  {
    Vec3f newTranslation = this->rotationMatrix * other.translation;
    newTranslation += this->translation;
    
    RotationMatrix3d newRotation(this->rotationMatrix);
    newRotation *= other.rotationMatrix;
    
    return Pose3d(newRotation, newTranslation);
  } // operator*(Pose3d)
  
  // Composition: this = other * this
  void Pose3d::preComposeWith(const Pose3d &other)
  {
    this->rotationMatrix.preMultiplyBy(other.rotationMatrix);
    this->translation = other.rotationMatrix * this->translation;
    this->translation += other.translation;
  }
  
  
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
  
  // Count number of steps to root ("World") node, by walking up
  // the chain of parents.
  unsigned int Pose3d::getTreeDepth(void) const
  {
    unsigned int treeDepth = 1;
    
    const Pose3d* current = this;
    while(current->parent != Pose3d::World)
    {
      ++treeDepth;
      current = current->parent;
    }
    
    return treeDepth;
    
  } // getTreeDepth()
  
  template<class POSE>
  POSE getWithRespectToHelper(const POSE *from, const POSE *to)
  {
    POSE P_from(*from);
    
    POSE *P_wrt_other = NULL;
    
    if(to == POSE::World) {
      
      // Special (but common!) case: get with respect to the
      // world pose.  Just chain together poses up to the root.
      
      while(from->parent != POSE::World) {
        P_from.preComposeWith( *(from->parent) );
        from = from->parent;
      }
      
      P_wrt_other = &P_from;
      
    } else {
      
      POSE P_to(*to);
      
      // First make sure we are pointing at two nodes of the same tree depth,
      // which is the only way they could possibly share the same parent.
      // Until that's true, walk the deeper node up until it is at the same
      // depth as the shallower node, keeping track of the total transformation
      // along the way. (NITE: Only one of the following two while loops should
      // run, depending on which node is deeper in the tree)
      
      while(from->getTreeDepth() > to->getTreeDepth())
      {
        CORETECH_ASSERT(from->parent != NULL);
        
        P_from.preComposeWith( *(from->parent) );
        from = from->parent;
        
      }
      
      while(to->getTreeDepth() > from->getTreeDepth())
      {
        CORETECH_ASSERT(to->parent != NULL);
        
        P_to.preComposeWith( *(to->parent) );
        to = to->parent;
      }
      
      // Treedepths should now match:
      CORETECH_ASSERT(to->getTreeDepth() == from->getTreeDepth());
      
      // Now that we are pointing to the nodes of the same depth, keep moving up
      // until those nodes have the same parent, totalling up the transformations
      // along the way
      while(to->parent != from->parent)
      {
        CORETECH_ASSERT(from->parent != NULL && to->parent != NULL);
        
        P_from.preComposeWith( *(from->parent) );
        P_to.preComposeWith( *(to->parent) );
        
        to = to->parent;
        from = from->parent;
      }
      
      // Now compute the total transformation from this pose, up the "from" path
      // in the tree, to the common ancestor, and back down the "to" side to the
      // final other pose.
      //     P_wrt_other = P_to.inv * P_from;
      P_to.Invert();
      P_to *= P_from;
      
      P_wrt_other = &P_to;
      
    } // IF/ELSE other is the World pose
    
    // The Pose we are about to return is w.r.t. the "other" pose provided (that
    // was the whole point of the exercise!), so set its parent accordingly:
    P_wrt_other->parent = to;
    
    CORETECH_ASSERT(P_wrt_other != NULL);
    
    return *P_wrt_other;

  } // getWithRespectToHelper()
  
  Pose2d Pose2d::getWithRespectTo(const Anki::Pose2d *otherPose) const
  {
    return getWithRespectToHelper(this, otherPose);
  }
  
  Pose3d Pose3d::getWithRespectTo(const Anki::Pose3d *otherPose) const
  {
    return getWithRespectToHelper(this, otherPose);
  }
  
  void testPoseInstantiation(void)
  {
    Pose2d p2(30.f, 10.f, -5.f);
    Pose3d p3;
  }
  
} // namespace Anki
