#ifndef _ANKICORETECH_MATH_POSE_H_
#define _ANKICORETECH_MATH_POSE_H_

#include "anki/math/config.h"

namespace Anki {

  // Forward declarations of types used below:
  class Vec3f;
  class Point3f;
  class Mat3x3;
  class Mat6x6;
 
  
  // A class for encapsulating 6DOF pose (3D translation plus 3-axis rotation).
  class Pose { // TODO: rename to Pose6DOF?
    
  public:
    // Construct from rotation vector and translation vector
    Pose(const Vec3f  &Rvec, const Vec3f &T);
    
    // Construct from rotation matrix and translation vector
    Pose(const Mat3x3 &Rmat, const Vec3f &T);
    
    // Construct from rotation angle, rotation axis, and translation vector
    Pose(const float angle, const Vec3f &axis, const Vec3f &T);
    
    // Destructor
    ~Pose();
    
    // Composition with another Pose
    void operator*=(const Pose &other); // in place
    Pose operator*(const Pose &other) const;
    
    // "Apply" Pose to a 3D point (i.e. transform that point by this Pose)
    Point3f operator*(const Point3f &point) const;
    
  protected:
    Vec3f  Rvector; // angle*axis vector
    Mat3x3 Rmatrix;
    Vec3f  translation;
    
    // 6x6 covariance matrix (upper-left 3x3 block corresponds to rotation,
    // lower right 3x3 block corresponds to translation)
    Mat6x6 covariance;
    
  }; // class Pose
  
  
} // namespace Anki

#endif // _ANKICORETECH_MATH_POSE_H_