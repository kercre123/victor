//
//  camera.h
//  CoreTech_Vision
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef __CoreTech_Vision__camera__
#define __CoreTech_Vision__camera__

#include <array>

#include "anki/common/basestation/math/pose.h"

namespace Anki {
  
  class CameraCalibration
  {
  public:
    static const int NumDistortionCoeffs = 5;
    typedef std::array<float,CameraCalibration::NumDistortionCoeffs> DistortionCoeffVector;
    
    // Constructors:
    CameraCalibration();
    
    CameraCalibration(const float fx, const float fy,
                      const float center_x, const float center_y,
                      const float skew,
                      const std::vector<float> &distCoeffs);
    
    // Accessors:
    float   get_focalLength_x() const;
    float   get_focalLength_y() const;
    float   get_center_x() const;
    float   get_center_y() const;
    Point2f get_center_pt() const;
    float   get_skew() const;
    const   DistortionCoeffVector& get_distortionCoeffs() const;
    
    // Returns the 3x3 camera calibration matrix:
    // [fx   skew*fx   center_x;
    //   0      fy     center_y;
    //   0       0         1    ]
    Matrix_3x3f get_calibrationMatrix() const;
    
  protected:
    
    float focalLength_x, focalLength_y;
    float center_x, center_y; 
    float skew;
    DistortionCoeffVector distortionCoeffs; // radial distortion coefficients
    
  }; // class CameraCalibration
  
  
  // Inline accessor definitions:
  inline float CameraCalibration::get_focalLength_x() const
  { return this->focalLength_x; }
  
  inline float CameraCalibration::get_focalLength_y() const
  { return this->focalLength_y; }
  
  inline float CameraCalibration::get_center_x() const
  { return this->center_x; }
  
  inline float CameraCalibration::get_center_y() const
  { return this->center_y; }
  
  inline Point2f CameraCalibration::get_center_pt() const
  { return Point2f(this->center_x, this->center_y); }
  
  inline float CameraCalibration::get_skew() const
  { return this->skew; }
  
  inline const std::array<float,5>& CameraCalibration::get_distortionCoeffs() const
  { return this->distortionCoeffs; }
  
  
  class Camera
  {
  public:
    
    // Constructors:
    Camera();
    Camera(const CameraCalibration &calib, const Pose3d &pose);
    
    // Accessors:
    inline const Pose3d&             get_pose()        const;
    inline const CameraCalibration&  get_calibration() const;
        
    //
    // Methods:
    //
    
    // Compute the 3D (6DoF) pose of a set of object points, given their
    // corresponding observed positions in the image:
    Pose3d computeObjectPose(const std::vector<Point2f> &imgPoints,
                             const std::vector<Point3f> &objPoints) const;
  
    Pose3d computeObjectPose(const Quad2f &imgPoints,
                             const Quad3f &objPoints) const;
    
    
    // Compute the projected image locations of a set of 3D points:
    void project3dPoints(const std::vector<Point3f> &objPoints,
                         std::vector<Point2f>       &imgPoints) const;

    void project3dPoints(const Quad3f &objPoints,
                         Quad2f       &imgPoints) const;
    
    
    // TODO: Method to remove radial distortion from an image
    // (This requires an image class)
    
    
  protected:
    
    CameraCalibration  calibration;
    Pose3d             pose;
    
    // TODO: Include const reference or pointer to a parent Robot object?
    
  }; // class Camera
  
  // Inline accessors:
  const Pose3d& Camera::get_pose(void) const
  { return this->pose; }
  
  const CameraCalibration& Camera::get_calibration(void) const
  { return this->calibration; }
  
  
} // namespace Anki

#endif // __CoreTech_Vision__camera__
