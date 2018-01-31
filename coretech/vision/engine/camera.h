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

//#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/pose.h"

#include "coretech/vision/engine/cameraCalibration.h"
#include "coretech/vision/engine/occluderList.h"

#include "util/helpers/templateHelpers.h"

#include <array>
#include <vector>

namespace Anki {
  
  namespace Vision {
    
    // Forward declarations:
    class ObservableObject;
    
    class KnownMarker;
    
    // For now, this is always assumed to be a calibrated camera.  If we want
    // a more generic camera that does operations that do not require calibration
    // we can make a base Camera class and a derived CalibratedCamera class.
    class Camera
    {
    public:
      
      // Constructors:
      Camera();
      Camera(const CameraID_t ID);
      Camera(const Camera& other);
      
      ~Camera();
      
      // Accessors:
      const CameraID_t          GetID()          const;
      const Pose3d&             GetPose()        const;
      const std::shared_ptr<CameraCalibration> GetCalibration() const; // nullptr if not calibrated
      std::shared_ptr<CameraCalibration>       GetCalibration();       //   "

      void SetID(const CameraID_t ID);
      void SetPose(const Pose3d& newPose);
      
      // Set the calibration of the camera. Multiple Camera instantiations can share
      // the same underlying CameraCalibration object.
      // Returns true if the calibration actually changed, false if this was the same calibration as before.
      // NOTE: Once a calibration is set, calling SetCalibration(nullptr) will generate an error, return false,
      //       and not "un-set" it.
      bool SetCalibration(std::shared_ptr<CameraCalibration> calib);
      
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
      
      // Use three points of a quadrilateral and the P3P algorithm to compute
      // possible camera poses, then use the fourth point to choose the valid
      // pose. Do this four times (once using each corner as the validation
      // point) and choose the best.  
      template<typename WORKING_PRECISION=double> // TODO: Make default float?
      Result ComputeObjectPose(const Quad2f &imgPoints,
                               const Quad3f &objPoints,
                               Pose3d       &objPose) const;
      
      
      // Compute the projected image locations of 3D point(s). The points
      //  should already be in the camera's coordinate system (i.e. relative to
      //  its origin.
      // After projection, points' visibility can be checked using the helpers
      //  below.
      // returns true if the point is in camera view, false otherwise
      bool Project3dPoint(const Point3f& objPoint, Point2f& imgPoint) const;
      
      void Project3dPoints(const std::vector<Point3f> &objPoints,
                           std::vector<Point2f>       &imgPoints) const;
      
      void Project3dPoints(const Quad3f &objPoints,
                           Quad2f       &imgPoints) const;
      
      template<size_t NumPoints>
      void Project3dPoints(const std::array<Point3f,NumPoints> &objPoints,
                           std::array<Point2f,NumPoints>       &imgPoints) const;
      
      // Project an object's corners into the camera and also return the object's
      // distance from the camera's origin
      // Returns whether or not the projection was successful
      bool ProjectObject(const ObservableObject&  object,
                         std::vector<Point2f>&    projectedCorners,
                         f32&                     distanceFromCamera) const;

      // Register an object as an "occluder" for this camera
      void AddOccluder(const ObservableObject& object);
      
      // Register a KnownMarker as an "occluder" for this camera
      void AddOccluder(const KnownMarker& marker);
      
      // Add arbitrary projected points as an occluder (their bounding box
      // will form the actual occluder)
      void AddOccluder(const std::vector<Point2f>& projectedPoints,
                       const f32 atDistance);
      
      // Return all the occluders for this camera
      void GetAllOccluders(std::vector<Rectangle<f32> >& occluders) const;
      
      // Clear all occluders known to this camera
      void ClearOccluders();
      
      // Returns true when the point (computed by one of the projection functions
      // above) is BOTH in front of the camera AND within image dimensions.
      // Occlusion by registered occluders is not considered.
      // Inset padding, in pixels, can also be specified.
      bool IsWithinFieldOfView(const Point2f& projectedPoint,
                               const u16 xBorderPad = 0,
                               const u16 yBorderPad = 0) const;
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
      
      // Compute the pan and tilt angles for putting the given image point at the
      // center of the image.
      void ComputePanAndTiltAngles(const Point2f& imgPoint, Radians& relPanAngle, Radians& relTiltAngle) const;

    protected:
      CameraID_t                         _camID;
      std::shared_ptr<CameraCalibration> _calibration;
      Pose3d                             _pose;
      
      OccluderList                       _occluderList;
      
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
    
#pragma mark -
#pragma mark Inlined Accessors
    
    // Inline accessors:
    inline const CameraID_t Camera::GetID(void) const
    { return _camID; }
    
    inline const Pose3d& Camera::GetPose(void) const
    { return _pose; }
    
    inline std::shared_ptr<CameraCalibration> Camera::GetCalibration(void)
    { return _calibration; }
    
    inline const std::shared_ptr<CameraCalibration> Camera::GetCalibration(void) const
    { return _calibration; }
    
    inline void Camera::SetID(const CameraID_t ID)
    { _camID = ID; }
    
    inline void Camera::SetPose(const Pose3d& newPose)
    { _pose = newPose; }
    
    inline bool Camera::IsCalibrated() const {
      return (_calibration != nullptr);
    }
        
    inline bool Camera::IsOccluded(const Point2f& projectedPoint, const f32 atDistance) const {
      return _occluderList.IsOccluded(projectedPoint, atDistance);
    }
    
    inline bool Camera::IsAnythingBehind(const Point2f& projectedPoint, const f32 atDistance) const {
      return _occluderList.IsAnythingBehind(projectedPoint, atDistance);
    }

    inline bool Camera::IsAnythingBehind(const Quad2f& projectedQuad, const f32 atDistance) const {
      return _occluderList.IsAnythingBehind(projectedQuad, atDistance);
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
    
    inline void Camera::GetAllOccluders(std::vector<Rectangle<f32> >& occluders) const
    {
      _occluderList.GetAllOccluders(occluders);
    }
    
  } // namespace Vision
} // namespace Anki

#endif // __CoreTech_Vision__camera__
