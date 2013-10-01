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

#include "anki/common/datastructures.h"

#include "anki/math/radians.h"
#include "anki/math/matrix.h"
#include "anki/math/point.h"

namespace Anki {
  
  class RotationMatrix2d : public Matrix_2x2f
  {
  public:
    RotationMatrix2d(const Radians angle = 0.f);
    
  }; // class RotationMatrix
  
  // Forward declaratin:
  class RotationMatrix3d;
  
  
  class RotationVector3d : public Vec3f
  {
  public:
    RotationVector3d(); // no rotation around z axis
    RotationVector3d(const Radians angle, const Vec3f &axis);
    RotationVector3d(const Vec3f &rvec);
    RotationVector3d(const RotationMatrix3d &rmat);
       
    Radians  get_angle() const;
    Vec3f    get_axis()  const;
    
#if defined(ANKICORETECH_USE_OPENCV)
    using Vec3f::get_CvPoint_;
    using Vec3f::x;
    using Vec3f::y;
    using Vec3f::z;
#endif
    
  }; // class RotationVector3d
  
  
  class RotationMatrix3d : public Matrix_3x3f
  {
  public:
    RotationMatrix3d(); // 3x3 identity matrix (no rotation)
    RotationMatrix3d(const RotationVector3d &rotationVector);
    RotationMatrix3d(const Matrix<float> &matrix3x3);
    RotationMatrix3d(const Radians angle, const Vec3f &axis);
  
    Radians  get_angle() const;
    Vec3f    get_axis()  const;
    
  protected:
    RotationVector3d rotationVector;
    
  }; // class RotationMatrix3d

 
  // Rodrigues' formula for converting between angle+axis representation and 3x3
  // matrix representation.
  void Rodrigues(const RotationVector3d &Rvec_in,
                       RotationMatrix3d &Rmat_out);
  
  void Rodrigues(const RotationMatrix3d &Rmat_in,
                       RotationVector3d &Rvec_out);

  
#pragma mark --- Inline Implementations ---
  
  inline Radians RotationMatrix3d::get_angle(void) const
  {
    return this->rotationVector.get_angle();
  }
  
  inline Vec3f RotationMatrix3d::get_axis(void) const
  {
    return this->rotationVector.get_axis();
  }
  
  inline Radians RotationVector3d::get_angle(void) const
  {
    return this->length();
  }
  
  inline Vec3f RotationVector3d::get_axis(void) const
  {
    Vec3f axis(*this);
    axis.makeUnitLength();
    return axis;
  }

} // namespace Anki

#endif /* defined(__CoreTech_Math__rotation__) */
