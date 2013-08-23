#ifndef _ANKICORETECH_MATH_POSE_H_
#define _ANKICORETECH_MATH_POSE_H_

#include "anki/math/config.h"
#include "anki/math/matrix.h"

#include "anki/common/point.h"

namespace Anki {

  // Forward declarations of types used below:
  typedef Point3f Vec3f;
  template<typename T> class Matrix;
  
  // A class for encapsulating 6DOF pose (3D translation plus 3-axis rotation).
  class Pose { // TODO: rename to Pose6DOF?
    
  public:
    // Construct from rotation vector and translation vector
    Pose(const Vec3f  &Rvec, const Vec3f &T);
    Pose(const Vec3f  &Rvec, const Vec3f &T, const Matrix<float> &cov);
    
    // Construct from rotation matrix and translation vector
    // TODO: do we want a version that takes in covariance too?
    Pose(const Matrix<float> &Rmat, const Vec3f &T);
    
    // Construct from rotation angle, rotation axis, and translation vector
    // (angle in radians)
    Pose(const float angle, const Vec3f &axis, const Vec3f &T);
    Pose(const float angle, const Vec3f &axis, const Vec3f &T,
         const Matrix<float> &covariance);
    
    // TODO: Copy constructor?
    
    // Destructor
    ~Pose();
    
    // Composition with another Pose
    void operator*=(const Pose &other); // in place
    Pose operator*(const Pose &other) const;
    
    // "Apply" Pose to a 3D point (i.e. transform that point by this Pose)
    Point3f operator*(const Point3f &point) const;
    
    Pose inverse(void) const;
    void inverse(void); // in place?
    
  protected:
    // TODO: Explicitly store both angle/axis/vector/matrix info all the time,
    // or compute-on-demand (and store?) when first requested?
    
    //float  angle;
    //Vec3f  axis;
    Vec3f  Rvector; // angle*axis vector
    Matrix<float> Rmatrix;
    Vec3f  translation;
    
    // 6x6 covariance matrix (upper-left 3x3 block corresponds to rotation
    // vector elements, lower right 3x3 block corresponds to translation)
    Matrix<float> covariance;
    
  }; // class Pose
  
  
} // namespace Anki

#endif // _ANKICORETECH_MATH_POSE_H_