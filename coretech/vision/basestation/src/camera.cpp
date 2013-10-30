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
  : nrows(480), ncols(640), focalLength_x(1.f), focalLength_y(1.f),
    center(0.f,0.f),
    skew(0.f)
  {
    /*
    std::fill(this->distortionCoeffs.begin(),
              this->distortionCoeffs.end(),
              0.f);
     */
  }
  
  CameraCalibration::CameraCalibration(const u16 nrowsIn, const u16 ncolsIn,
                                       const f32 fx,    const f32 fy,
                                       const f32 cenx,  const f32 ceny,
                                       const f32 skew_in)
  : nrows(nrowsIn), ncols(ncolsIn),
    focalLength_x(fx), focalLength_y(fy),
    center(cenx, ceny), skew(skew_in)
  {
    /*
    std::fill(this->distortionCoeffs.begin(),
              this->distortionCoeffs.end(),
              0.f);
     */
  }
  
  
  Matrix_3x3f CameraCalibration::get_calibrationMatrix(void) const
  {
    f32 K_data[9] = {
      this->focalLength_x, this->focalLength_x*this->skew, this->center.x(),
      0.f,                 this->focalLength_y,            this->center.y(),
      0.f,                 0.f,                            1.f};
    
    Matrix_3x3f K(K_data);
    
    return K;
  } // get_calibrationMatrix()
  
#if ANKICORETECH_USE_OPENCV
  Pose3d Camera::computeObjectPoseHelper(const std::vector<cv::Point2f>& cvImagePoints,
                                         const std::vector<cv::Point3f>& cvObjPoints) const
  {
    cv::Vec3d cvRvec, cvTranslation;
    
    Matrix_3x3f calibMatrix(this->calibration.get_calibrationMatrix());
    
    cv::Mat distortionCoeffs; // TODO: currently empty, use radial distoration?
    cv::solvePnP(cvObjPoints, cvImagePoints,
                 calibMatrix.get_CvMatx_(), distortionCoeffs,
                 cvRvec, cvTranslation,
                 false, CV_ITERATIVE);
    
    RotationVector3d rvec(Vec3f(cvRvec[0], cvRvec[1], cvRvec[2]));
    Vec3f translation(cvTranslation[0], cvTranslation[1], cvTranslation[2]);
    
    // Return Pose object w.r.t. the camera's pose
    return Pose3d(rvec, translation, &(this->pose));
    
  } // computeObjectPoseHelper()
  
#endif
  
  
  Pose3d Camera::computeObjectPose(const std::vector<Point2f> &imgPoints,
                           const std::vector<Point3f> &objPoints) const
  {
    
#if ANKICORETECH_USE_OPENCV
    std::vector<cv::Point2f> cvImagePoints;
    std::vector<cv::Point3f> cvObjPoints;
    
    for(const Point2f & imgPt : imgPoints) {
      cvImagePoints.emplace_back(imgPt.get_CvPoint_());
    }
    
    for(const Point3f & objPt : objPoints) {
      cvObjPoints.emplace_back(objPt.get_CvPoint3_());
    }
    
    return computeObjectPoseHelper(cvImagePoints, cvObjPoints);
    
#else
    // TODO: Implement!
    CORETECH_THROW("Unimplemented!")
    return Pose3d();
#endif
    
  } // computeObjectPose(from std::vectors)
  
  Pose3d Camera::computeObjectPose(const Quad2f &imgPoints,
                           const Quad3f &objPoints) const
  {
#if ANKICORETECH_USE_OPENCV
    std::vector<cv::Point2f> cvImagePoints;
    std::vector<cv::Point3f> cvObjPoints;
    
    cvImagePoints.emplace_back(imgPoints[Quad::TopLeft].get_CvPoint_());
    cvImagePoints.emplace_back(imgPoints[Quad::TopRight].get_CvPoint_());
    cvImagePoints.emplace_back(imgPoints[Quad::BottomLeft].get_CvPoint_());
    cvImagePoints.emplace_back(imgPoints[Quad::BottomRight].get_CvPoint_());
    
    cvObjPoints.emplace_back(objPoints[Quad::TopLeft].get_CvPoint3_());
    cvObjPoints.emplace_back(objPoints[Quad::TopRight].get_CvPoint3_());
    cvObjPoints.emplace_back(objPoints[Quad::BottomLeft].get_CvPoint3_());
    cvObjPoints.emplace_back(objPoints[Quad::BottomRight].get_CvPoint3_());
    
    return computeObjectPoseHelper(cvImagePoints, cvObjPoints);
    
#else
    // TODO: Implement our own non-opencv version?
    CORETECH_THROW("Unimplemented!")
    return Pose3d();
#endif

  } // computeObjectPose(from quads)
  
  void  Camera::project3dPoint(const Point3f& objPoint,
                               Point2f&       imgPoint) const
  {
    const f32 BEHIND_CAM = std::numeric_limits<f32>::quiet_NaN();
    
    if(objPoint.z() <= 0.f)
    {
      // Point not visible (not in front of camera)
      imgPoint = BEHIND_CAM;
      
    } else {
      // Point visible, project it
      imgPoint.x() = (objPoint.x() / objPoint.z());
      imgPoint.y() = (objPoint.y() / objPoint.z());
      
      // TODO: Add radial distortion here
      //distortCoordinate(imgPoints[i_corner], imgPoints[i_corner]);
      
      imgPoint.x() *= this->calibration.get_focalLength_x();
      imgPoint.y() *= this->calibration.get_focalLength_y();
      
      imgPoint += this->calibration.get_center();
    }
    
  } // project3dPoint()
  
  // Compute the projected image locations of a set of 3D points:
  void Camera::project3dPoints(const std::vector<Point3f> &objPoints,
                       std::vector<Point2f>       &imgPoints) const
  {
    imgPoints.resize(objPoints.size());
    for(size_t i = 0; i<objPoints.size(); ++i)
    {
      project3dPoint(objPoints[i], imgPoints[i]);
    }
  } // project3dPoints(std::vectors)
  
  void Camera::project3dPoints(const Quad3f &objPoints,
                       Quad2f       &imgPoints) const
  {
    for(Quad::CornerName i_corner=Quad::FirstCorner;
        i_corner < Quad::NumCorners; ++i_corner)
    {
      project3dPoint(objPoints[i_corner], imgPoints[i_corner]);
    }
  } // project3dPoints(Quads)

  
} // namespace Anki