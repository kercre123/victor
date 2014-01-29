//
//  rotation.cpp
//  CoreTech_Math
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/common/basestation/math/rotation.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/calib3d/calib3d.hpp"
#endif

namespace Anki {
  
  
  RotationMatrix2d::RotationMatrix2d(const Radians angle)
  {
    const float cosAngle = std::cos(angle.ToFloat());
    const float sinAngle = std::sin(angle.ToFloat());
    
    (*this)(0,0) =  cosAngle;
    (*this)(1,0) =  sinAngle;
    (*this)(0,1) = -sinAngle;
    (*this)(1,1) =  cosAngle;
  }

  
  RotationVector3d::RotationVector3d(void)
  : angle(0.f), axis(X_AXIS_3D)
  {
    
  }
  
  RotationVector3d::RotationVector3d(const Radians angleIn, const Vec3f &axisIn)
  : angle(angleIn), axis(axisIn)
  {
    f32 axisLength = axis.makeUnitLength();
    if(axisLength != 1.f) {
      // TODO: Issue a warning that axis provided was not unit length?
    }
  }
  
  RotationVector3d::RotationVector3d(const Vec3f &rvec)
  : axis(rvec)
  {
    this->angle = axis.makeUnitLength();
    if(this->angle == Radians(0)) {
      // If the specified vector was all zero, then the axis is ambiguous,
      // so default to the X axis
      axis = X_AXIS_3D;
    }
  }
  
  RotationVector3d::RotationVector3d(const RotationMatrix3d &Rmat)
  : RotationVector3d()
  {
    Rodrigues(Rmat, *this);
  }
  
  
  UnitQuaternion::UnitQuaternion(const RotationVector3d& Rvec)
  {
    const float halfAngle = Rvec.get_angle().ToFloat() * 0.5f;
    this->data[0] = std::cos(halfAngle);
    
    const float sinHalfAngle = std::sin(halfAngle);
    this->data[1] = Rvec.get_axis().x() * sinHalfAngle;
    this->data[2] = Rvec.get_axis().y() * sinHalfAngle;
    this->data[3] = Rvec.get_axis().z() * sinHalfAngle;
  }
  
  RotationMatrix3d::RotationMatrix3d(void)
  {
    (*this)(0,0) = 1.f;
    (*this)(0,1) = 0.f;
    (*this)(0,2) = 0.f;
    
    (*this)(1,0) = 0.f;
    (*this)(1,1) = 1.f;
    (*this)(1,2) = 0.f;
  
    (*this)(2,0) = 0.f;
    (*this)(2,1) = 0.f;
    (*this)(2,2) = 1.f;
    
  } // Constructor: RotationMatrix3d()
  
  RotationMatrix3d::RotationMatrix3d(const RotationVector3d &rotVec_in)
  : rotationVector(rotVec_in)
  {
    Rodrigues(this->rotationVector, *this);
  }
  
  RotationMatrix3d::RotationMatrix3d(const Radians angle, const Vec3f &axis)
  : rotationVector(angle, axis)
  {
    Rodrigues(this->rotationVector, *this);
  }
  
  /* Isn't this inherited from the base class now?
  Point3f RotationMatrix3d::operator*(const Point3f &p) const
  {
#if ANKICORETECH_USE_OPENCV
    // TODO: pretty sure there's a better way to do this:
    cv::Matx<float,3,1> vec(p.x, p.y, p.z);
    cv::Matx<float,3,1> out(this->get_CvMatx_() * vec);
    Point3f rotatedPoint(out(0,0), out(1,0), out(2,0));
#else
    assert(false);
#endif
    
    return rotatedPoint;
    
  } // RotationMatrix3d::operator*(Point3f)
  */
  
  void Rodrigues(const RotationVector3d &Rvec_in,
                 RotationMatrix3d &Rmat_out)
  {
#if ANKICORETECH_USE_OPENCV
    cv::Vec3f cvRvec(Rvec_in.get_axis().get_CvPoint3_());
    cvRvec *= Rvec_in.get_angle().ToFloat();
    cv::Rodrigues(cvRvec, Rmat_out.get_CvMatx_());    
#else
    
    assert(false);
    // TODO: implement our own version?
#endif
    
  } // Rodrigues(Rvec, Rmat)
  
  
  void Rodrigues(const RotationMatrix3d &Rmat_in,
                 RotationVector3d &Rvec_out)
  {
   
#if ANKICORETECH_USE_OPENCV
    cv::Vec3f cvRvec;
    cv::Rodrigues(Rmat_in.get_CvMatx_(), cvRvec);
    Anki::Vec3f tempVec(cvRvec);
    Rvec_out = RotationVector3d(tempVec);
#else
    assert(false);
    
    // TODO: implement our own version?
    
#endif
    
  } // Rodrigues(Rmat, Rvec)
  
  
} // namespace Anki
