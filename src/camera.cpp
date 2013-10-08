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
  
  
  Matrix_3x3f CameraCalibration::get_calibrationMatrix(void) const
  {
    float K_data[9] = {
      this->focalLength_x, this->focalLength_x*this->skew, this->center_x,
      0.f,                 this->focalLength_y,            this->center_y,
      0.f,                 0.f,                            1.f};
    
    Matrix_3x3f K(K_data);
    
    return K;
  } // get_calibrationMatrix()
  
} // namespace Anki