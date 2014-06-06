//
//  camera.h
//  CoreTech_Vision
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef __CoreTech_Vision__camera__
#define __CoreTech_Vision__camera__

#include "json/json.h"

//#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/vision/basestation/cameraCalibration.h"
#include "anki/vision/basestation/occluderList.h"

#include <array>
#include <vector>

namespace Anki {
  
  namespace Vision {
    
    class Camera
    {
    public:
      
      // Constructors:
      Camera();
      Camera(const CameraID_t cam_id, const CameraCalibration& calib, const Pose3d& pose);
      
      // Accessors:
      const CameraID_t          get_id()          const;
      const Pose3d&             get_pose()        const;
      const CameraCalibration&  get_calibration() const;
      
      void set_pose(const Pose3d& newPose);
      void set_calibration(const CameraCalibration& calib);
      
      bool IsCalibrated() const;
      
      //
      // Methods:
      //
      
      // Compute the 3D (6DoF) pose of a set of object points, given their
      // corresponding observed positions in the image.  The returned Pose will
      // be w.r.t. the camera's pose, unless the input camPose is non-NULL, in
      // which case the returned Pose will be w.r.t. that pose.
      Pose3d ComputeObjectPose(const std::vector<Point2f> &imgPoints,
                               const std::vector<Point3f> &objPoints) const;
      
      // Use three points of a quadrilateral and the P3P algorithm  to compute
      // possible camera poses, then use the fourth point to choose the valid
      // pose. Do this four times (once using each corner as the validation
      // point) and choose the best.  
      template<typename WORKING_PRECISION=double> // TODO: Make default float?
      Pose3d ComputeObjectPose(const Quad2f &imgPoints,
                               const Quad3f &objPoints) const;
      
      
      // Compute the projected image locations of 3D point(s). The points
      //  should already be in the camera's coordinate system (i.e. relative to
      //  its origin.
      // After projection, points' visibility can be checked using the helpers
      //  below.
      void Project3dPoint(const Point3f& objPoint, Point2f& imgPoint) const;
      
      void Project3dPoints(const std::vector<Point3f> &objPoints,
                           std::vector<Point2f>       &imgPoints) const;
      
      void Project3dPoints(const Quad3f &objPoints,
                           Quad2f       &imgPoints) const;
      
      template<size_t NumPoints>
      void Project3dPoints(const std::array<Point3f,NumPoints> &objPoints,
                           std::array<Point2f,NumPoints>       &imgPoints) const;

      // Register an object as an "occluder" for this camera
      void AddOccluder(const Vision::ObservableObject* object);
      void ClearOccluders();
      
      // Returns true when the point (computed by one of the projection functions
      // above) is BOTH in front of the camera AND within image dimensions.
      // Occlusion by registered occluders is not considered.
      bool IsWithinFieldOfView(const Point2f& projectedPoint) const;
      //bool IsVisible(const Quad2f& projectedQuad) const; // TODO: Implement

      // Returns true when a point in image, seen at the specified distance,
      // is occluded by a registered occluder.
      bool IsOccluded(const Point2f& projectedPoint, const f32 atDistance) const;
      
      // Returns true when there is a registered "occluder" behind the specified
      // point or quad at the given distance.
      bool IsAnythingBehind(const Point2f& projectedPoint, const f32 atDistance) const;
      bool IsAnythingBehind(const Quad2f&  projectedQuad,  const f32 atDistance) const;

      // TODO: Method to remove radial distortion from an image
      // (This requires an image class)
      
    protected:
      CameraID_t         camID;
      CameraCalibration  calibration;
      bool               isCalibrationSet;
      Pose3d             pose;
      
      OccluderList       occluderList;
      
      // TODO: Include const reference or pointer to a parent Robot object?
      void DistortCoordinate(const Point2f& ptIn, Point2f& ptDistorted);
      
      template<class PointContainer3d, class PointContainer2d>
      void Project3dPointHelper(const PointContainer3d& objPoints,
                                PointContainer2d& imgPoints) const;
      
#if ANKICORETECH_USE_OPENCV
      Pose3d ComputeObjectPoseHelper(const std::vector<cv::Point2f>& cvImagePoints,
                                     const std::vector<cv::Point3f>& cvObjPoints) const;
#endif
      
    }; // class Camera
    
    // Inline accessors:
    inline const CameraID_t Camera::get_id(void) const
    { return this->camID; }
    
    inline const Pose3d& Camera::get_pose(void) const
    { return this->pose; }
    
    inline const CameraCalibration& Camera::get_calibration(void) const
    { return this->calibration; }
    
    inline void Camera::set_calibration(const CameraCalibration &calib)
    { this->calibration = calib; this->isCalibrationSet = true; }
    
    inline void Camera::set_pose(const Pose3d& newPose)
    { this->pose = newPose; }
    
    inline bool Camera::IsCalibrated() const {
      return this->isCalibrationSet;
    }
    
    inline bool Camera::IsOccluded(const Point2f& projectedPoint, const f32 atDistance) const {
      return occluderList.IsOccluded(projectedPoint, atDistance);
    }
    
    inline bool Camera::IsAnythingBehind(const Point2f& projectedPoint, const f32 atDistance) const {
      return occluderList.IsAnythingBehind(projectedPoint, atDistance);
    }

    inline bool Camera::IsAnythingBehind(const Quad2f& projectedQuad, const f32 atDistance) const {
      return occluderList.IsAnythingBehind(projectedQuad, atDistance);
    }

    
    template<size_t NumPoints>
    void Camera::Project3dPoints(const std::array<Point3f,NumPoints> &objPoints,
                                 std::array<Point2f,NumPoints>       &imgPoints) const
    {
      for(size_t i = 0; i<NumPoints; ++i)
      {
        Project3dPoint(objPoints[i], imgPoints[i]);
      }
    } // // Project3dPoints(std::array)
    
    
  } // namesapce Vision
} // namespace Anki

#endif // __CoreTech_Vision__camera__
