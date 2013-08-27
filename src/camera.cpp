//
//  camera.cpp
//  CoreTech_Vision
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/vision/camera.h"

namespace Anki {
  
  Camera::Camera(void)
  {
    
  } // Constructor: Camera()
  
  Camera::Camera(const CameraCalibration &calib_in,
                 const Pose3d& pose_in)
  : calibration(calib_in), pose(pose_in)
  {
    
  } // Constructor: Camera(calibration, pose)
  
  
  Matrix<float> CameraCalibration::get_calibrationMatrix(void) const
  {
    Matrix<float> K(3,3);
    
    K(0,0) = this->focalLength_x;
    K(0,1) = this->focalLength_x * this->skew;
    K(0,2) = this->center_x;
    
    K(1,0) = 0.f;
    K(1,1) = this->focalLength_y;
    K(1,2) = this->center_y;
    
    K(2,0) = 0.f;
    K(2,1) = 0.f;
    K(2,2) = 1.f;
    
    return K;
  } // get_calibrationMatrix()
  
} // namespace Anki