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
  
  class RotationMatrix2d : public Matrix<float>
  {
  public:
    RotationMatrix2d(const float angle);
    
  }; // class RotationMatrix
  
  // Forward declaratin:
  class RotationMatrix3d;
  
  class RotationVector3d : public Vec3f
  {
  public:
    RotationVector3d(const float angle, const float axis);
    RotationVector3d(const Vec3f &rvec);
    RotationVector3d(const RotationMatrix3d &rmat);
    
    float get_angle() const;
    Vec3f get_axis()  const;
    
  protected:
    float angle;
    Vec3f axis;
    
  }; // class RotationVector3d
  
  
  class RotationMatrix3d : public Matrix<float>
  {
  public:
    RotationMatrix3d(const RotationVector3d &rotationVector);
    RotationMatrix3d(const Matrix<float> &matrix3x3);
  
  protected:
    RotationVector3d rotationVector;
    
  }; // class RotationMatrix3d

  
  // Rodrigues' formula for converting between angle+axis representation and 3x3
  // matrix representation.
  Result Rodrigues(const RotationVector3d &Rvec_in,
                         RotationMatrix3d &Rmat_out);
  
  Result Rodrigues(const RotationMatrix3d &Rmat_in,
                         RotationVector3d &Rvec_out);


} // namespace Anki

#endif /* defined(__CoreTech_Math__rotation__) */
