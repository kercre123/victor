//
//  camera.h
//  CoreTech_Vision
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef __CoreTech_Vision__camera__
#define __CoreTech_Vision__camera__

#include "anki/math/pose.h"

namespace Anki {
  
  class CameraCalibration
  {
  public:
    // Constructors:
    CameraCalibration();
    
    CameraCalibration(const float fx, const float fy,
                      const float center_x, const float center_y,
                      const float skew,
                      const std::vector<float> &distCoeffs);
    
    // Accessors:
    float get_focalLength_x() const;
    float get_focalLength_y() const;
    float get_center_x() const;
    float get_center_y() const;
    float get_skew() const;
    const std::vector<float>& get_distortionCoeffs() const;
    
    // Returns the 3x3 camera calibration matrix:
    // [fx   skew*fx   center_x;
    //   0      fy     center_y;
    //   0       0         1    ]
    Matrix<float> get_calibrationMatrix() const;
    
  protected:
    
    float focalLength_x, focalLength_y;
    float center_x, center_y;
    float skew;
    std::vector<float> distortionCoeffs; // radial distortion coefficients
    
  }; // class CameraCalibration
  
  
  class Camera
  {
  public:
    
    // Constructors:
    Camera();
    Camera(const CameraCalibration &calib, const Pose3d &pose);
    
    // Accessors:
    const Pose3d& get_pose() const;
    const CameraCalibration& get_calibration() const;
    
    //
    // Methods:
    //
    
    // Compute the 3D (6DoF) pose of a set of object points, given their
    // corresponding observed positions in the image:
    Pose3d computeObjectPose(const std::vector<Point2<float> > &imgPoints,
                             const std::vector<Point3<float> > &objPoints) const;
    
    // Compute the projected image locations of a set of 3D points:
    void project3dPoints(const std::vector<Point3f> &objPoints,
                         std::vector<Point2f> &imgPoints) const;
    
    // TODO: Method to remove radial distortion from an image
    // (This requires an image class)
    
    
  protected:
    
    CameraCalibrationData  calibration;
    Pose3d                 pose;
    
    
    // TODO: Include const reference or pointer to a parent Robot object?
    
  }; // class Camera
  
  
  
} // namespace Anki

#endif // __CoreTech_Vision__camera__
