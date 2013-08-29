//
//  rotation.h
//  CoreTech_Math
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef __CoreTech_Math__rotation__
#define __CoreTech_Math__rotation__

#include "anki/common/datastructures.h"
#include "anki/common/point.h"

#include "anki/math/matrix.h"

namespace Anki {
  
  class RotationMatrix2d : public Matrix_2x2f
  {
  public:
    RotationMatrix2d(const float angle = 0.f);
    
  }; // class RotationMatrix
  
  // Forward declaratin:
  class RotationMatrix3d;
  
  class RotationVector3d : public Vec3f
  {
  public:
    RotationVector3d(); // no rotation around z axis
    RotationVector3d(const float angle, const float axis);
    RotationVector3d(const Vec3f &rvec);
    RotationVector3d(const RotationMatrix3d &rmat);
       
    inline float        get_angle() const;
    inline const Vec3f& get_axis()  const;
    
  protected:
    float angle;
    Vec3f axis;
    
  }; // class RotationVector3d
  
  // Inline accessors:
  float RotationVector3d::get_angle(void) const
  { return this->angle; }
  
  const Vec3f& RotationVector3d::get_axis(void) const
  { return this->axis; }
  
  
  class RotationMatrix3d : public Matrix_3x3f
  {
  public:
    RotationMatrix3d(); // 3x3 identity matrix (no rotation)
    RotationMatrix3d(const RotationVector3d &rotationVector);
    RotationMatrix3d(const Matrix<float> &matrix3x3);
  
    Point3f operator*(const Point3f &p) const;
    
    inline float        get_angle() const;
    inline const Vec3f& get_axis() const;
    
  protected:
    RotationVector3d rotationVector;
    
  }; // class RotationMatrix3d

  // Inline accessors:
  float RotationMatrix3d::get_angle(void) const
  { return this->rotationVector.get_angle(); }
  
  const Vec3f& RotationMatrix3d::get_axis(void) const
  { return this->rotationVector.get_axis(); }

  
  
  // Rodrigues' formula for converting between angle+axis representation and 3x3
  // matrix representation.
  void Rodrigues(const RotationVector3d &Rvec_in,
                       RotationMatrix3d &Rmat_out);
  
  void Rodrigues(const RotationMatrix3d &Rmat_in,
                       RotationVector3d &Rvec_out);


} // namespace Anki

#endif /* defined(__CoreTech_Math__rotation__) */
