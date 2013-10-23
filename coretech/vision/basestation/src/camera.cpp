//
//  camera.cpp
//  CoreTech_Vision
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include <algorithm>

#include "anki/vision/basestation/camera.h"

namespace Anki {
  
  Camera::Camera(void)
  {
    
  } // Constructor: Camera()
  
  Camera::Camera(const CameraCalibration &calib_in,
                 const Pose3d& pose_in)
  : calibration(calib_in), pose(pose_in)
  {
    
  } // Constructor: Camera(calibration, pose)
  
  
  CameraCalibration::CameraCalibration()
  : focalLength_x(1.f), focalLength_y(1.f),
    center_x(0.f), center_y(0.f),
    skew(0.f)
  {
    /*
    std::fill(this->distortionCoeffs.begin(),
              this->distortionCoeffs.end(),
              0.f);
     */
  }
  
  CameraCalibration::CameraCalibration(const float fx,   const float fy,
                                       const float cenx, const float ceny,
                                       const float skew_in)
  : focalLength_x(fx), focalLength_y(fy),
    center_x(cenx), center_y(ceny), skew(skew_in)
  {
    /*
    std::fill(this->distortionCoeffs.begin(),
              this->distortionCoeffs.end(),
              0.f);
     */
  }
  
  
  Matrix_3x3f CameraCalibration::get_calibrationMatrix(void) const
  {
    float K_data[9] = {
      this->focalLength_x, this->focalLength_x*this->skew, this->center_x,
      0.f,                 this->focalLength_y,            this->center_y,
      0.f,                 0.f,                            1.f};
    
    Matrix_3x3f K(K_data);
    
    return K;
  } // get_calibrationMatrix()
  
  
  Pose3d Camera::computeObjectPose(const std::vector<Point2f> &imgPoints,
                           const std::vector<Point3f> &objPoints) const
  {
    // TODO: Implement!
    CORETECH_THROW("Unimplemented!")
    return Pose3d();
  }
  
  Pose3d Camera::computeObjectPose(const Quad2f &imgPoints,
                           const Quad3f &objPoints) const
  {
    // TODO: Implement!
    CORETECH_THROW("Unimplemented!")
    return Pose3d();
  }
  
  
  // Compute the projected image locations of a set of 3D points:
  void Camera::project3dPoints(const std::vector<Point3f> &objPoints,
                       std::vector<Point2f>       &imgPoints) const
  {
    
  }
  
  void Camera::project3dPoints(const Quad3f &objPoints,
                       Quad2f       &imgPoints) const
  {
    
  }

  
} // namespace Anki