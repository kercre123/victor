#ifndef _ANKICORETECH_MATH_POSE_H_
#define _ANKICORETECH_MATH_POSE_H_

#include "anki/math/config.h"
#include "anki/math/matrix.h"
#include "anki/math/rotation.h"

#include "anki/common/point.h"

namespace Anki {

  // Forward declarations of types used below:
  typedef Point3f Vec3f;
  template<typename T> class Matrix;
  
  
  class Pose2d
  {
  public:
    // Constructors:
    Pose2d(const float x, const float y, const float angle);
    Pose2d(const float x, const float y, const float angle,
           const Matrix<float> &cov);
    
    // Accessors:
    inline float   get_x()     const;
    inline float   get_y()     const;
    inline Point2f get_xy()    const;
    inline float   get_angle() const;
    
    Vec3f get_normal(void) const;
    void  set_normal(const Vec3f &normal_new);
    
    RotationMatrix2d get_rotationMatrix() const;
    
    // Composition with another Pose2d:
    void   operator*=(const Pose2d &other); // in place
    Pose2d operator* (const Pose2d &other) const;
    
    // "Apply" Pose to a 2D point (i.e. transform that point by this Pose)
    Point2f operator*(const Point2f &point) const;
    
    Pose2d getInverse(void); const // return new Pose
    void Invert(void); // in place
    
  protected:
    float x, y, angle;
    
    // Stores the orientation of this 2D plane in 3D space
    Vec3f normal;
    
    Matrix<float> covariance;
    
  }; // class Pose2d
  

  // TODO: Add thin wrapper or typedef for "Transform2d/3d" objects?
  
  
  // A class for encapsulating 6DOF pose (3D translation plus 3-axis rotation).
  class Pose3d {
    
  public:
    static Pose3d* World;
    
    // Default pose: no rotation, no translation, world as parent
    Pose3d();
    
    // Construct from rotation vector and translation vector
    Pose3d(const RotationVector3d &Rvec, const Vec3f &T,
           const Pose3d *parentPose = Pose3d::World);
    
    Pose3d(const RotationVector3d &Rvec, const Vec3f &T, const Matrix<float> &cov,
           const Pose3d *parentPose = Pose3d::World);
    
    // Construct from rotation matrix and translation vector
    // TODO: do we want a version that takes in covariance too?
    Pose3d(const RotationMatrix3d &Rmat, const Vec3f &T,
           const Pose3d *parentPose = Pose3d::World);
    
    
    // TODO: Copy constructor?
    
    
    // Composition with another Pose
    void   operator*=(const Pose3d &other); // in place
    Pose3d operator*(const Pose3d &other) const;
    
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
    
    // 6x6 covariance matrix (upper-left 3x3 block corresponds to rotation
    // vector elements, lower right 3x3 block corresponds to translation)
    Matrix<float> covariance;
    
    // "Parent" for defining linkages or sequences of poses, so we can get
    // a pose "with respect to" a parent or to root (world) pose.
    const Pose3d *parent;
    
  }; // class Pose3d
  
  
} // namespace Anki

#endif // _ANKICORETECH_MATH_POSE_H_