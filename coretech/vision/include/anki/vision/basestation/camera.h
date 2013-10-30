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

//#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"

namespace Anki {
  
  class CameraCalibration
  {
  public:
    /*
    static const int NumDistortionCoeffs = 5;
    typedef std::array<float,CameraCalibration::NumDistortionCoeffs> DistortionCoeffVector;
    */
    
    // Constructors:
    CameraCalibration();
    
    CameraCalibration(const u16 nrows,    const u16 ncols,
                      const f32 fx,       const f32 fy,
                      const f32 center_x, const f32 center_y,
                      const f32 skew = 0.f);
    
    CameraCalibration(const u16 nrows,    const u16 ncols,
                      const f32 fx,       const f32 fy,
                      const f32 center_x, const f32 center_y,
                      const f32 skew,
                      const std::vector<float> &distCoeffs);
    
    // Accessors:
    u16     get_nrows()         const;
    u16     get_ncols()         const;
    f32     get_focalLength_x() const;
    f32     get_focalLength_y() const;
    f32     get_center_x()      const;
    f32     get_center_y()      const;
    f32     get_skew()          const;
    const Point2f& get_center() const;
    //const   DistortionCoeffVector& get_distortionCoeffs() const;
    
    // Returns the 3x3 camera calibration matrix:
    // [fx   skew*fx   center_x;
    //   0      fy     center_y;
    //   0       0         1    ]
    Matrix_3x3f get_calibrationMatrix() const;
    
  protected:
    
    u16  nrows, ncols;
    f32  focalLength_x, focalLength_y;
    Point2f center;
    f32  skew;
    //DistortionCoeffVector distortionCoeffs; // radial distortion coefficients
    
  }; // class CameraCalibration
  
  
  // Inline accessor definitions:
  inline u16 CameraCalibration::get_nrows() const
  { return this->nrows; }
  
  inline u16 CameraCalibration::get_ncols() const
  { return this->ncols; }
  
  inline f32 CameraCalibration::get_focalLength_x() const
  { return this->focalLength_x; }
  
  inline f32 CameraCalibration::get_focalLength_y() const
  { return this->focalLength_y; }
  
  inline f32 CameraCalibration::get_center_x() const
  { return this->center.x(); }
  
  inline f32 CameraCalibration::get_center_y() const
  { return this->center.y(); }
  
  inline const Point2f& CameraCalibration::get_center() const
  { return this->center; }
  
  inline f32  CameraCalibration::get_skew() const
  { return this->skew; }
  
  /*
  inline const std::array<float,5>& CameraCalibration::get_distortionCoeffs() const
  { return this->distortionCoeffs; }
  */
  
  class Camera
  {
  public:
    
    // Constructors:
    Camera();
    Camera(const CameraCalibration &calib, const Pose3d &pose);
    
    // Accessors:
    const Pose3d&             get_pose()        const;
    const CameraCalibration&  get_calibration() const;
    
    void set_pose(const Pose3d& newPose);
    void set_calibration(const CameraCalibration& calib);
    
    //
    // Methods:
    //
    
    // Compute the 3D (6DoF) pose of a set of object points, given their
    // corresponding observed positions in the image.  The returned Pose will
    // be w.r.t. the camera's pose, unless the input camPose is non-NULL, in
    // which case the returned Pose will be w.r.t. that pose.
    Pose3d computeObjectPose(const std::vector<Point2f> &imgPoints,
                             const std::vector<Point3f> &objPoints) const;
  
    Pose3d computeObjectPose(const Quad2f &imgPoints,
                             const Quad3f &objPoints) const;
    
    
    // Compute the projected image locations of 3D point(s):
    // (Resulting projected image points can be tested for being behind the
    //  camera or visible using the functions below.)
    void project3dPoint(const Point3f& objPoint, Point2f& imgPoint) const;
    
    void project3dPoints(const std::vector<Point3f> &objPoints,
                         std::vector<Point2f>       &imgPoints) const;

    void project3dPoints(const Quad3f &objPoints,
                         Quad2f       &imgPoints) const;
    
    // Returns true when the point (computed by one of the projection functions
    // above) is behind the camera.  Otherwise it is false -- even if the point
    // is outside image dimensions.
    bool isBehind(const Point2f& projectedPoint) const;
    
    // Returns true when the point (computed by one of the projection functions
    // above) is BOTH in front of the camera AND within image dimensions.
    bool isVisible(const Point2f& projectedPoint) const;
    
    // TODO: Method to remove radial distortion from an image
    // (This requires an image class)
    
    
  protected:
    CameraCalibration  calibration;
    Pose3d             pose;
    
    // TODO: Include const reference or pointer to a parent Robot object?
    void distortCoordinate(const Point2f& ptIn, Point2f& ptDistorted);
    
  }; // class Camera
  
  // Inline accessors:
  inline const Pose3d& Camera::get_pose(void) const
  { return this->pose; }
  
  inline const CameraCalibration& Camera::get_calibration(void) const
  { return this->calibration; }
  
  inline void Camera::set_calibration(const Anki::CameraCalibration &calib)
  { this->calibration = calib; }
  
  inline void Camera::set_pose(const Pose3d& newPose)
  { this->pose = newPose; }
  
  inline bool Camera::isBehind(const Point2f &projectedPoint) const
  {
    return (std::isnan(projectedPoint.x()) || std::isnan(projectedPoint.y()));
  }
  
  inline bool Camera::isVisible(const Point2f &projectedPoint) const
  {
    return (not isBehind(projectedPoint) &&
            projectedPoint.x() >= 0.f && projectedPoint.y() >= 0.f &&
            projectedPoint.x() < this->calibration.get_ncols() &&
            projectedPoint.y() < this->calibration.get_nrows());
  }
} // namespace Anki

#endif // __CoreTech_Vision__camera__
