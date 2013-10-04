//
//  rotation.cpp
//  CoreTech_Math
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/math/rotation.h"

#if defined(ANKICORETECH_USE_OPENCV)
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
  : Vec3f(0.f, 0.f, 0.f)
  {
    
  }
  
  RotationVector3d::RotationVector3d(const Radians angle, const Vec3f &axis)
  : Vec3f(axis)
  {
    this->makeUnitLength();
    *this *= angle.ToFloat();
  }
  
  RotationVector3d::RotationVector3d(const Vec3f &rvec)
  : Vec3f(rvec)
  {

  }
  
  RotationVector3d::RotationVector3d(const RotationMatrix3d &Rmat)
  : Vec3f(0.f, 0.f, 0.f)
  {
    Rodrigues(Rmat, *this);
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
#if defined(ANKICORETECH_USE_OPENCV)
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
#if defined(ANKICORETECH_USE_OPENCV)
    cv::Vec3f cvRvec(Rvec_in.get_CvPoint3_());
    cv::Rodrigues(cvRvec, Rmat_out.get_CvMatx_());    
#else
    
    assert(false);
    // TODO: implement our own version?
#endif
    
  } // Rodrigues(Rvec, Rmat)
  
  
  void Rodrigues(const RotationMatrix3d &Rmat_in,
                 RotationVector3d &Rvec_out)
  {
   
#if defined(ANKICORETECH_USE_OPENCV)
    cv::Vec3f cvRvec;
    cv::Rodrigues(Rmat_in.get_CvMatx_(), cvRvec);
    Rvec_out.x() = cvRvec[0];
    Rvec_out.y() = cvRvec[1];
    Rvec_out.z() = cvRvec[2];
#else
    assert(false);
    
    // TODO: implement our own version?
    
#endif
    
  } // Rodrigues(Rmat, Rvec)
  
  
} // namespace Anki
