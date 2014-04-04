/**
 * File: rotation.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 8/23/2013
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description: Implements objects for storing rotation operations, in two and
 *              three dimensions.
 *
 *              RotationMatrix2d and RotationMatrix3d are subclasses of
 *              SmallSquareMatrix for storing 2x2 and 3x3 rotation matrices, 
 *              respectively.
 *
 *              RotationVector3d stores an angle+axis rotation vector.
 *
 *              Rodrigues functions are provided to convert between 
 *              RotationMatrix3d and RotationVector3d containers.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef __CoreTech_Math__rotation__
#define __CoreTech_Math__rotation__

#include "anki/common/shared/radians.h"
#include "matrix.h"
#include "point.h"

namespace Anki {
  
  class RotationMatrix2d : public Matrix_2x2f
  {
  public:
    RotationMatrix2d(const Radians angle = 0.f);
    
  }; // class RotationMatrix
  
  // Forward declaratin:
  class RotationMatrix3d;
  
  
  class RotationVector3d
  {
  public:
    RotationVector3d(); // no rotation around z axis
    RotationVector3d(const Radians angle, const Vec3f &axis);
    RotationVector3d(const Vec3f &rvec);
    RotationVector3d(const RotationMatrix3d &rmat);
    
    // Accessors for angle and axis.  Note that it is more efficient
    // to request both simultaneously if you need both, because the
    // angle must be computed to get the axis anyway.
    Radians       get_angle() const;
    const Vec3f&  get_axis()  const;
    void          get_angleAndAxis(Radians &angle, Vec3f &axis) const;
    
  private:
    Radians angle;
    Vec3f axis; // unit vector
    
  }; // class RotationVector3d
  
  
  class UnitQuaternion : public Point<4, float>
  {
  public:
    UnitQuaternion(const RotationVector3d& Rvec);
    
  }; // class UnitQuaternion
  
  
  class RotationMatrix3d : public Matrix_3x3f
  {
  public:
    RotationMatrix3d(); // 3x3 identity matrix (no rotation)
    RotationMatrix3d(const RotationVector3d &rotationVector);
    RotationMatrix3d(const Matrix_3x3f &matrix3x3);
    RotationMatrix3d(std::initializer_list<float> initVals);
    RotationMatrix3d(const Radians angle, const Vec3f &axis);
  
    //Radians  get_angle() const;
    //Vec3f    get_axis()  const;
    
    // Return total angular rotation from the identity (no rotation)
    Radians GetAngle() const;
    
    // Return angular rotation difference from another rotation matrix
    Radians GetAngleDiffFrom(const RotationMatrix3d &other) const;
    
    /*
    // Overload math operations to keep rotation vector updated:
    void operator*=(const RotationMatrix3d &other);
    void preMultiplyBy(const RotationMatrix3d &other);
    */
    
    // Matrix inversion and transpose just negate the change the sign of the
    // rotation angle
    RotationMatrix3d& Transpose(void);
    RotationMatrix3d  getTranspose() const;
    void Invert(void); // same as transpose
    RotationMatrix3d getInverse(void) const;
    
    
  protected:
    //RotationVector3d rotationVector;
    
  }; // class RotationMatrix3d
  
  // Rodrigues' formula for converting between angle+axis representation and 3x3
  // matrix representation.
  void Rodrigues(const RotationVector3d &Rvec_in,
                       RotationMatrix3d &Rmat_out);
  
  void Rodrigues(const RotationMatrix3d &Rmat_in,
                       RotationVector3d &Rvec_out);

  
#pragma mark --- Inline Implementations ---
  /*
  inline Radians RotationMatrix3d::get_angle(void) const
  {
    return this->rotationVector.get_angle();
  }
  
  inline Vec3f RotationMatrix3d::get_axis(void) const
  {
    return this->rotationVector.get_axis();
  }
   */
  
  inline Radians RotationVector3d::get_angle(void) const
  {
    return this->angle;
  }
  
  inline const Vec3f& RotationVector3d::get_axis(void) const
  {
    return this->axis;
  }
  
  inline void RotationVector3d::get_angleAndAxis(Radians &angle,
                                                 Vec3f   &axis) const
  {
    axis = this->axis;
    angle = this->angle;
  }
  


} // namespace Anki

#endif /* defined(__CoreTech_Math__rotation__) */
