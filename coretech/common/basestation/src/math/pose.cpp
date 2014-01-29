
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/matrix.h"

#include <stdexcept>

namespace Anki {
    
#pragma mark --- Pose2d Implementations ---
  Pose2d* Pose2d::World = NULL;
  
  Pose2d::Pose2d(const Radians &theta, const Point2f &t)
  : translation(t), angle(theta), planeNormal(0.f,0.f,1.f),
    parent(Pose2d::World)
  {
    
  }
  
  Pose2d::Pose2d(const Radians &theta, const float x, const float y)
  : translation(x,y), angle(theta), planeNormal(0.f,0.f,1.f),
    parent(Pose2d::World)
  {
    
  }
    
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
  
  Pose2d Pose2d::getInverse(void) const
  {
    Pose2d returnPose(*this);
    returnPose.Invert();
    return returnPose;
  }
  
  Pose2d& Pose2d::Invert(void)
  {
    this->angle *= -1.f;
    RotationMatrix2d R(this->angle);

    this->translation *= -1.f;
    this->translation = R * this->translation;
    
    return *this;
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
  
  Pose3d::Pose3d(const Radians angle, const Vec3f axis,
                 const Vec3f T, const Pose3d *parentPose)
  : rotationVector(angle, axis),
    rotationMatrix(rotationVector),
    translation(T),
    parent(parentPose)
  {
    
  } // Constructor: Pose3d(angle, axis, T)
  
  Pose3d::Pose3d(const Pose3d &otherPose)
  : Pose3d(otherPose.rotationMatrix, otherPose.translation, otherPose.parent)
  {
    
  }
  
  Pose3d::Pose3d(const Pose3d *otherPose)
  : Pose3d(*otherPose)
  {
    
  }
  
  Pose3d::Pose3d(const Pose2d &pose2d)
  : Pose3d(pose2d.get_angle(), {{0.f, 0.f, 0.f}},
           {{pose2d.get_x(), pose2d.get_y(), 0.f}})
  {
    // At this point, we have initialized a 3D pose corresponding
    // just to the 2D pose (i.e. w.r.t. the (0,0,0) origin and
    // with rotation around the Z axis).
    //
    // Next compute the 3D pose of the plane in which the 2D Pose
    // is embedded.  Then compose the two poses to get the final
    // 3D pose.
    
    const Vec3f Zaxis(0.f, 0.f, 1.f);
    
    const float dotProduct = dot(pose2d.get_planeNormal(), Zaxis);
    
    CORETECH_ASSERT(std::abs(dotProduct) <= 1.f);
    
    // We will rotate the pose2d's plane normal into the Z axis.
    // The dot product gives us the angle needed to do that, and
    // the cross product gives us the axis around which we will
    // rotate.
    Radians angle3d = std::acos(dotProduct);
    Vec3f   axis3d  = cross(Zaxis, pose2d.get_planeNormal());
    
    Pose3d planePose(angle3d, axis3d, pose2d.get_planeOrigin());
    this->preComposeWith(planePose);
    
  } // Constructor: Pose3d(Pose2d)
  
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
  
  Point3f Pose3d::operator*(const Point3f &pointIn) const
  {
    Point3f pointOut( this->rotationMatrix * pointIn );
    pointOut += this->translation;
    
    return pointOut;
  }
  
  void Pose3d::applyTo(const Quad3f &quadIn,
                       Quad3f &quadOut) const
  {
    using namespace Quad;
    quadOut[TopLeft]     = (*this) * quadIn[TopLeft];
    quadOut[TopRight]    = (*this) * quadIn[TopRight];
    quadOut[BottomLeft]  = (*this) * quadIn[BottomLeft];
    quadOut[BottomRight] = (*this) * quadIn[BottomRight];
  }
  
  void Pose3d::applyTo(const std::vector<Point3f> &pointsIn,
                       std::vector<Point3f>       &pointsOut) const
  {
    const size_t numPoints = pointsIn.size();
    
    if(pointsOut.size() == numPoints)
    {
      // The output vector already has the right number of points
      // in it.  No need to construct a new vector full of (0,0,0)
      // points with resize; just replace what's there.
      for(size_t i=0; i<numPoints; ++i)
      {
        pointsOut[i] = (*this) * pointsIn[i];
      }
      
    } else {
      // Clear the output vector, and use push_back to add newly-
      // constructed points. Again, this avoids first creating a
      // bunch of (0,0,0) points via resize and then immediately
      // overwriting them.
      pointsOut.clear();

      for(const Point3f& x : pointsIn)
      {
        pointsOut.emplace_back( (*this) * x );
      }
    }
  } // applyTo()
  
#pragma mark --- Member Methods ---
  Pose3d Pose3d::getInverse(void) const
  {
    Pose3d returnPose(*this);
    returnPose.Invert();
    return returnPose;
  }
  
  Pose3d& Pose3d::Invert(void)
  {
    this->rotationMatrix.Transpose();
    this->translation *= -1.f;
    this->translation = this->rotationMatrix * this->translation;
    
    return *this;
  }
  
  void Pose3d::rotateBy(const Radians& angleIn) {
    // Keep same rotation axis, but add the incoming angle
    RotationMatrix3d Rnew(angleIn, this->get_rotationAxis());
    this->translation = Rnew * this->translation;
    Rnew *= this->rotationMatrix;
    this->set_rotation(Rnew);
  }
  
  void Pose3d::rotateBy(const RotationVector3d& Rvec)
  {
    RotationMatrix3d Rnew(Rvec);
    this->translation = Rnew * this->translation;
    Rnew *= this->rotationMatrix;
    this->set_rotation(Rnew);
  }
  
  void Pose3d::rotateBy(const RotationMatrix3d& Rmat)
  {
    this->translation = Rmat * this->translation;
    this->rotationMatrix.preMultiplyBy(Rmat);
    this->set_rotation(this->rotationMatrix);
  }

  // Count number of steps to root ("World") node, by walking up
  // the chain of parents.
  template<class POSE>
  unsigned int getTreeDepthHelper(const POSE *P)
  {
    unsigned int treeDepth = 1;
    
    const POSE* current = P;
    while(current->parent != POSE::World)
    {
      ++treeDepth;
      current = current->parent;
    }
    
    return treeDepth;
  }
  
  unsigned int Pose2d::getTreeDepth(void) const
  {
    return getTreeDepthHelper<Pose2d>(this);
  } // getTreeDepth()

  unsigned int Pose3d::getTreeDepth(void) const
  {
    return getTreeDepthHelper<Pose3d>(this);
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
    return getWithRespectToHelper<Pose2d>(this, otherPose);
  }
  
  Pose3d Pose3d::getWithRespectTo(const Anki::Pose3d *otherPose) const
  {
    return getWithRespectToHelper<Pose3d>(this, otherPose);
  }
  
  bool Pose3d::IsSameAs(const Pose3d& otherPose,
                        const float distThreshold,
                        const Radians angleThreshold) const
  {
    bool isSame = false;
    if(computeDistanceBetween(this->translation, otherPose.translation) < distThreshold)
    {
      // TODO: use quaternion difference?  Might be cheaper than the matrix multiplication below
      //     UnitQuaternion qThis(this->rotationVector), qOther(otherPose.rotationVector);
      //
      //
      // const Radians angleDiff( 2.f*std::acos(std::abs(dot(qThis, qOther))) );
      //
      
      RotationMatrix3d R(this->rotationMatrix);
      R *= otherPose.rotationMatrix;
      
      const Radians angleDiff( 0.5f * std::acos(R.Trace() - 1.f) );
      if(angleDiff < angleThreshold) {
        isSame = true;
      }
    }
    
    return isSame;
  }
  
#pragma mark --- Global Functions ---
  
  float computeDistanceBetween(const Pose3d& pose1, const Pose3d& pose2)
  {
    // Compute distance between the two poses' translation vectors
    // TODO: take rotation into account?
    Vec3f distVec(pose1.get_translation());
    distVec -= pose2.get_translation();
    return distVec.length();
  }

  


  
} // namespace Anki
