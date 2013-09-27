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

#include "anki/math/matrix.h"
#include "anki/math/rotation.h"
#include "anki/math/radians.h"

#include "anki/math/point.h"

namespace Anki {

  // Forward declarations of types used below:
  typedef Point3f Vec3f;
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
    inline float   get_x()        const;
    inline float   get_y()        const;
    inline Radians get_angle()    const;
    
    Vec3f get_normal(void) const;
    void  set_normal(const Vec3f &normal_new);
    
    RotationMatrix2d get_rotationMatrix() const;
    
    // Composition with another Pose2d:
    void   operator*=(const Pose2d &other); // in place
    Pose2d operator* (const Pose2d &other) const;
    void preComposeWith(const Pose2d &other);
    
    // "Apply" Pose to a 2D point (i.e. transform that point by this Pose)
    Point2f operator*(const Point2f &point) const;
    
    Pose2d getInverse(void); const // return new Pose
    void Invert(void); // in place
    
    // Get this pose with respect to another pose.  Other pose
    // must be an ancestor of this pose, otherwise an error
    // will be generated.  Use Pose2d::World to get the pose with
    // respect to the root / world pose.
    Pose2d getWithRespectTo(const Pose2d *otherPose) const;

  protected:
    Point2f translation;
    Radians angle;
    
    // Stores the orientation of this 2D plane in 3D space
    Vec3f normal;
    
    /* TODO: Add and use 3DOF covariance
    Matrix<float> covariance;
     */
    
    // "Parent" for defining linkages or sequences of poses, so we can get
    // a pose "with respect to" a parent or to root (world) pose.
    const Pose2d *parent;
    //unsigned int treeDepth; // helps find common ancestor with other poses
    unsigned int getTreeDepth(void) const;
    
    friend Pose2d getWithRespectToHelper(const Pose2d *from,
                                         const Pose2d *to);

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
    
    void update(const RotationMatrix3d &Rmat, const Vec3f &T);
    void update(const RotationVector3d &Rvec, const Vec3f &T);

    // TODO: Copy constructor?
    Pose3d(const Pose3d &otherPose);
    Pose3d(const Pose3d *otherPose);
    
    // Composition with another Pose
    void   operator*=(const Pose3d &other); // in place
    Pose3d operator*(const Pose3d &other) const;
    void preComposeWith(const Pose3d &other);
    
    // "Apply" Pose to a 3D point (i.e. transform that point by this Pose)
    Point3f operator*(const Point3f &point) const;
    
    Pose3d getInverse(void) const;
    void   Invert(void); // in place?
    
    // Get this pose with respect to another pose.  Other pose
    // must be an ancestor of this pose, otherwise an error
    // will be generated.  Use Pose3d::World to get the pose with
    // respect to the root / world pose.
    Pose3d getWithRespectTo(const Pose3d *otherPose) const;
    
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
    //unsigned int treeDepth; // helps find common ancestor with other poses
    unsigned int getTreeDepth(void) const;
    
    friend Pose3d getWithRespectToHelper(const Pose3d *from,
                                         const Pose3d *to);
    
  }; // class Pose3d
  
  
} // namespace Anki

#endif // _ANKICORETECH_MATH_POSE_H_