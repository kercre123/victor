//
//  rotation.cpp
//  CoreTech_Math
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/math/rotation.h"

namespace Anki {
  
  
  RotationMatrix2d::RotationMatrix2d(const float angle)
  : Matrix<float>(2,2)
  {
    const float cosAngle = std::cos(angle);
    const float sinAngle = std::sin(angle);
    
    (*this)(0,0) =  cosAngle;
    (*this)(1,0) =  sinAngle;
    (*this)(0,1) = -sinAngle;
    (*this)(1,1) =  cosAngle;
  }
  
  
  RotationMatrix3d::RotationMatrix3d(const RotationVector3d &rotVec_in)
  : Matrix<float>(3,3), rotationVector(rotVec_in)
  {
    Rodrigues(this->rotationVector, *this);
  }
  
  RotationVector3d::RotationVector3d(const Vec3f &rvec)
  : angle(rvec.length()), axis(rvec)
  {
    this->axis *= 1.f/this->angle;
  }
  
  RotationVector3d::RotationVector3d(const RotationMatrix3d &Rmat)
  {
    Rodrigues(Rmat, *this);
  }
  
  
  Result Rodrigues(const RotationVector3d &Rvec_in,
                   RotationMatrix3d &Rmat_out)
  {
    Result returnValue = RESULT_OK;
    
#if defined(ANKICORETECH_USE_OPENCV)
    cv::Vec3f cvRvec(Rvec_in.get_CvPoint3_());
    cv::Rodrigues(cvRvec, Rmat_out);
    assert(Rmat_out.numRows() == 3 && Rmat_out.numCols() == 3);
    
    if(Rmat_out.empty()) {
      returnValue = RESULT_FAIL;
    }
    
#else
    
    assert(false);
    // TODO: implement our own version?
#endif
    
    return returnValue;
    
  } // Rodrigues(Rvec, Rmat)
  
  
  Result Rodrigues(const RotationMatrix3d &Rmat_in,
                   RotationVector3d &Rvec_out)
  {
    assert(Rmat_in.numRows()==3 && Rmat_in.numCols()==3);
    
    Result returnValue = RESULT_OK;
    
#if defined(ANKICORETECH_USE_OPENCV)
    cv::Vec3f cvRvec;
    cv::Rodrigues(Rmat_in, cvRvec);
    Rvec_out.x = cvRvec[0];
    Rvec_out.y = cvRvec[1];
    Rvec_out.z = cvRvec[2];
#else
    assert(false);
    
    // TODO: implement our own version?
    
#endif
    
    return returnValue;
    
  } // Rodrigues(Rmat, Rvec)
  
  
} // namespace Anki
