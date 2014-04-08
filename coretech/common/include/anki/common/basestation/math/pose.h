/**
 * File: pose.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 9/10/2013
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description: Implements two classes for storing Pose, meaning a rotation 
 *              translation.  
 *
 *                Pose2d stores 2D pose representing rotation
 *                in a plane with a 2D translation vector. Thus it has three
 *                degrees of freedom: x, y, and angle.
 *
 *                Pose3d stores 3D pose representing a 3D rotation and
 *                a 3D translation vector.  Thus it has six degrees of
 *                freedom: three rotation angles and x, y, z translation.
 *               
 *                Each has the ability to represent a kinematic tree using
 *                a "parent" pointer. Thus, a pose "with respect to" another
 *                pose can be requested using the getWithRespectTo() method.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef _ANKICORETECH_MATH_POSE_H_
#define _ANKICORETECH_MATH_POSE_H_

#include "anki/common/basestation/math/matrix.h"
#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/quad.h"
#include "anki/common/basestation/math/rotation.h"
#include "anki/common/shared/radians.h"

namespace Anki {

  // Forward declarations of types used below:
  //  typedef Point3f Vec3f;
  //template<typename T> class Matrix;
  

  // TODO: Have Pose2d and Pose3d inherit from an (abstract?) base class?
  //  Then the base class could define the common elements like the parent
  //  pointer, the getTreeDepth() method, and the getWithRespectTo method.
  
  //
  // Pose2d Class
  //
  // Stores 2d translation and orienation in a plane.
  //
  class Pose2d
  {
  public:
    static Pose2d* World;
    
    // Constructors:
    Pose2d(const Radians &angle, const Point2f &translation);
    Pose2d(const Radians &angle, const float x, const float y);

    /* TODO: Add constructor to take in covariance
    Pose2d(const float x, const float y, const Radians angle,
           const Matrix<float> &cov);
     */
    
    // Accessors:
    float   get_x()     const;
    float   get_y()     const;
    Radians get_angle() const;
    
    const Point2f& get_translation() const;
    const Point3f& get_planeOrigin() const;
    const Vec3f&   get_planeNormal() const;

    void  set_planeOrigin(const Point3f &origin);
    void  set_planeNormal(const Vec3f   &normal);
    
    // Note that this Rotation Matrix is not a member but is computed
    // on-the-fly from the Pose's angle.
    RotationMatrix2d get_rotationMatrix() const;
    
    void set_parent(const Pose2d* otherPose);
    const Pose2d* get_parent() const;
    
    // Composition with another Pose2d:
    void   operator*=(const Pose2d &other); // in place
    Pose2d operator* (const Pose2d &other) const;
    void preComposeWith(const Pose2d &other);
    
    // "Apply" Pose to a 2D point (i.e. transform that point by this Pose)
    Point2f operator*(const Point2f &point) const;
    
    Pose2d  getInverse(void) const; // return new Pose
    Pose2d& Invert(void); // in place
    
    // Get this pose with respect to another pose.  Other pose
    // must be an ancestor of this pose, otherwise an error
    // will be generated.  Use Pose2d::World to get the pose with
    // respect to the root / world pose.
    Pose2d getWithRespectTo(const Pose2d *otherPose) const;

  protected:
    Point2f translation;
    Radians angle;
    
    // Stores the orientation of this 2D plane in 3D space
    Point3f planeOrigin;
    Vec3f   planeNormal;
    
    /* TODO: Add and use 3DOF covariance
    Matrix<float> covariance;
     */
    
    // "Parent" for defining linkages or sequences of poses, so we can get
    // a pose "with respect to" a parent or to root (world) pose.
    const Pose2d *parent;
    unsigned int getTreeDepth(void) const;
    
    template<class POSE>
    friend POSE getWithRespectToHelper(const POSE *from, const POSE *to);

    template<class POSE>
    friend unsigned int getTreeDepthHelper(const POSE *P);
    
  }; // class Pose2d
  
  
  // TODO: Add thin wrapper or typedef for "Transform2d/3d" objects?
  
  
  //
  // Pose3d Class
  //
  // Stores a 6DOF (Degree Of Freedom) pose: 3D translation plus
  //   3-axis rotation.
  //
  class Pose3d {
    
  public:
    static Pose3d* World;
        
    // Default pose: no rotation, no translation, world as parent
    Pose3d();
    
    // Construct from rotation vector and translation vector
    Pose3d(const RotationVector3d &Rvec, const Vec3f &T,
           const Pose3d *parentPose = Pose3d::World);
    
    /* TODO: Add constructor that takes in covariance
    Pose3d(const RotationVector3d &Rvec, const Vec3f &T, const Matrix<float> &cov,
           const Pose3d *parentPose = Pose3d::World);
     */
    
    // Construct from rotation matrix and translation vector
    // TODO: do we want a version that takes in covariance too?
    Pose3d(const RotationMatrix3d &Rmat, const Vec3f &T,
           const Pose3d *parentPose = Pose3d::World);
    
    // Construct from an angle, axis, and translation vector
    Pose3d(const Radians angle, const Vec3f axis,
           const Vec3f translation,
           const Pose3d *parentPose = Pose3d::World);
    
    // Construct a Pose3d from a Pose2d (using the plane information)
    Pose3d(const Pose2d &pose2d);
    
    // TODO: Copy constructor?
    Pose3d(const Pose3d &otherPose);
    Pose3d(const Pose3d *otherPose);

    // Accessors:
    const RotationMatrix3d& get_rotationMatrix() const;
    const RotationVector3d& get_rotationVector() const;
    const Vec3f&            get_translation()    const;
    Vec3f                   get_rotationAxis()   const;
    Radians                 get_rotationAngle()  const;
    
    void set_rotation(const RotationMatrix3d &Rmat);
    void set_rotation(const RotationVector3d &Rvec);
    void set_rotation(const Radians angle, const Vec3f &axis);
    void set_translation(const Vec3f &T);
    
    void set_parent(const Pose3d* parent);
    const Pose3d* get_parent() const;
    
    // Composition with another Pose
    void   operator*=(const Pose3d &other); // in place
    Pose3d operator*(const Pose3d &other) const;
    void preComposeWith(const Pose3d &other);
    
    void rotateBy(const Radians& angle); // around existing axis
    void rotateBy(const RotationVector3d& Rvec);
    void rotateBy(const RotationMatrix3d& Rmat);
    
    // "Apply" Pose to 3D point(s) (i.e. transform that point by this Pose)
    template<typename T>
    Point<3,T> operator*(const Point<3,T> &point) const;
    
    template<typename T>
    void applyTo(const std::vector<Point<3,T> > &pointsIn,
                 std::vector<Point<3,T> >       &pointsOut) const;

    template<typename T>
    void applyTo(const Quadrilateral<3,T> &quadIn,
                 Quadrilateral<3,T>       &quadOut) const;
    
    Pose3d  getInverse(void) const;
    Pose3d& Invert(void); // in place?
    
    // Get this pose with respect to another pose.  Other pose
    // must be an ancestor of this pose, otherwise an error
    // will be generated.  Use Pose3d::World to get the pose with
    // respect to the root / world pose.
    Pose3d getWithRespectTo(const Pose3d *otherPose) const;
    
    // Check to see if two poses are the same.  Return true if so.
    // If requested, P_diff will contain the transformation from this pose to
    // P_other.
    bool IsSameAs(const Pose3d& P_other,
                  const float   distThreshold,
                  const Radians angleThreshold) const;
    
    bool IsSameAs(const Pose3d& P_other,
                  const float   distThreshold,
                  const Radians angleThreshold,
                  Pose3d& P_diff) const;
    
    // Same as above, but with a list of rotational ambiguities to ignore.
    // True will be returned so long as the difference from P_other is
    // one of the given rotations.
    bool IsSameAs_WithAmbiguity(const Pose3d& P_other,
                                const std::vector<RotationMatrix3d>& R_ambiguities,
                                const float   distThreshold,
                                const Radians angleThreshold,
                                const bool    useAbsRotation,
                                Pose3d& P_diff) const;
    
  protected:
    
    RotationVector3d  rotationVector;
    RotationMatrix3d  rotationMatrix;
    Vec3f             translation;
    
    /* TODO: Add and use 6DOF covariance.
    // 6x6 covariance matrix (upper-left 3x3 block corresponds to rotation
    // vector elements, lower right 3x3 block corresponds to translation)
    Matrix<float> covariance;
     */
    
    // "Parent" for defining linkages or sequences of poses, so we can get
    // a pose "with respect to" a parent or to root (world) pose.
    const Pose3d *parent;
    unsigned int getTreeDepth(void) const;
    
    template<class POSE>
    friend POSE getWithRespectToHelper(const POSE *from, const POSE *to);
    
    template<class POSE>
    friend unsigned int getTreeDepthHelper(const POSE *P);
    
  }; // class Pose3d
  
  // Compute distance between the two poses' translation vectors
  // TODO: take rotation into account?
  float computeDistanceBetween(const Pose3d& pose1, const Pose3d& pose2);
  
  //
  // Inline accessors:
  //
  
  // Pose2d
  
  inline float Pose2d::get_x(void) const
  { return this->translation.x(); }
  
  inline float Pose2d::get_y(void) const
  { return this->translation.y(); }
  
  inline Radians Pose2d::get_angle(void) const
  { return this->angle; }
  
  inline RotationMatrix2d Pose2d::get_rotationMatrix(void) const
  { return RotationMatrix2d(this->angle); }
  
  inline const Point2f& Pose2d::get_translation(void) const
  { return this->translation; }
  
  inline void Pose2d::set_planeNormal(const Vec3f &normal)
  {
    this->planeNormal = normal;
    this->planeNormal.makeUnitLength();
  }
  
  inline const Vec3f& Pose2d::get_planeNormal() const
  { return this->planeNormal; }
  
  inline const Point3f& Pose2d::get_planeOrigin() const
  { return this->planeOrigin; }
  
  inline void Pose2d::set_parent(const Anki::Pose2d *otherPose)
  { this->parent = otherPose; }
  
  inline const Pose2d* Pose2d::get_parent(void) const
  { return this->parent; }
  
  
  // Pose3d
  
  inline const RotationMatrix3d& Pose3d::get_rotationMatrix() const
  { return this->rotationMatrix; }
  
  inline const RotationVector3d& Pose3d::get_rotationVector() const
  { return this->rotationVector; }
  
  inline const Vec3f& Pose3d::get_translation() const
  { return this->translation; }
  
  inline Vec3f Pose3d::get_rotationAxis() const
  { return this->rotationVector.get_axis(); }
  
  inline Radians Pose3d::get_rotationAngle() const
  { return this->rotationVector.get_angle(); }
  
  inline void Pose3d::set_rotation(const RotationMatrix3d &Rmat)
  {
    if(&(this->rotationMatrix) != &Rmat) {
      this->rotationMatrix = Rmat;
    }
    this->rotationVector = RotationVector3d(Rmat);
  }
  
  inline void Pose3d::set_rotation(const RotationVector3d &Rvec)
  {
    if(&(this->rotationVector) != &Rvec) {
      this->rotationVector = Rvec;
    }
    this->rotationMatrix = RotationMatrix3d(Rvec);
  }
  
  inline void Pose3d::set_rotation(const Radians angle, const Vec3f &axis)
  {
    this->rotationVector = RotationVector3d(angle, axis);
    this->rotationMatrix = RotationMatrix3d(this->rotationVector);
  }
  
  inline void Pose3d::set_translation(const Vec3f &T)
  {
    this->translation = T;
  }
  
  inline void Pose3d::set_parent(const Pose3d* parent)
  { this->parent = parent; }
  
  inline const Pose3d* Pose3d::get_parent() const
  { return this->parent; }

  inline bool Pose3d::IsSameAs(const Pose3d& P_other,
                       const float   distThreshold,
                       const Radians angleThreshold) const
  {
    Pose3d P_diff_temp;
    return this->IsSameAs(P_other, distThreshold, angleThreshold,
                          P_diff_temp);
  }
  
  template<typename T>
  Point<3,T> Pose3d::operator*(const Point<3,T> &pointIn) const
  {
    Point3f pointOut( this->rotationMatrix * pointIn );
    pointOut += this->translation;
    
    return pointOut;
  }
  
  template<typename T>
  void Pose3d::applyTo(const Quadrilateral<3,T> &quadIn,
                       Quadrilateral<3,T>       &quadOut) const
  {
    using namespace Quad;
    quadOut[TopLeft]     = (*this) * quadIn[TopLeft];
    quadOut[TopRight]    = (*this) * quadIn[TopRight];
    quadOut[BottomLeft]  = (*this) * quadIn[BottomLeft];
    quadOut[BottomRight] = (*this) * quadIn[BottomRight];
  }
  
  template<typename T>
  void Pose3d::applyTo(const std::vector<Point<3,T> > &pointsIn,
                       std::vector<Point<3,T> >       &pointsOut) const
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
  
} // namespace Anki

#endif // _ANKICORETECH_MATH_POSE_H_