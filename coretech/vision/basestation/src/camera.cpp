//
//  camera.cpp
//  CoreTech_Vision
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include <algorithm>

#if ANKICORETECH_USE_OPENCV
#include "opencv2/calib3d/calib3d.hpp"
#endif

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
    
#if ANKICORETECH_USE_OPENCV
    
#endif
    
    return Pose3d();
  }
  
  Pose3d Camera::computeObjectPose(const Quad2f &imgPoints,
                           const Quad3f &objPoints) const
  {
#if ANKICORETECH_USE_OPENCV
    cv::Vec3d cvRvec, cvTranslation;
    std::vector<cv::Point2f> cvImagePoints;
    std::vector<cv::Point3f> cvObjPoints;
    
    cvImagePoints.emplace_back(imgPoints[Quad2f::TopLeft].get_CvPoint_());
    cvImagePoints.emplace_back(imgPoints[Quad2f::TopRight].get_CvPoint_());
    cvImagePoints.emplace_back(imgPoints[Quad2f::BottomLeft].get_CvPoint_());
    cvImagePoints.emplace_back(imgPoints[Quad2f::BottomRight].get_CvPoint_());
    
    cvObjPoints.emplace_back(objPoints[Quad3f::TopLeft].get_CvPoint3_());
    cvObjPoints.emplace_back(objPoints[Quad3f::TopRight].get_CvPoint3_());
    cvObjPoints.emplace_back(objPoints[Quad3f::BottomLeft].get_CvPoint3_());
    cvObjPoints.emplace_back(objPoints[Quad3f::BottomRight].get_CvPoint3_());
    
    Matrix_3x3f calibMatrix(this->calibration.get_calibrationMatrix());
    
    cv::Mat distortionCoeffs; // TODO: currently empty, use radial distoration?
    cv::solvePnP(cvObjPoints, cvImagePoints,
                 calibMatrix.get_CvMatx_(), distortionCoeffs,
                 cvRvec, cvTranslation,
                 false, CV_ITERATIVE);
    
    Vec3f rvec(cvRvec[0], cvRvec[1], cvRvec[2]);
    Vec3f translation(cvTranslation[0], cvTranslation[1], cvTranslation[2]);

    // Return Pose object w.r.t. the camera's pose
    return Pose3d(rvec, translation, &(this->pose));
    
#else
    // TODO: Implement our own non-opencv version?
    CORETECH_THROW("Unimplemented!")
    return Pose3d();
#endif

  } // computeObjectPose(from quads)
  
  
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