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

#include "json/json.h"

//#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/vision/basestation/occluderList.h"

#ifdef SIMULATOR
#include <webots/Camera.hpp>
#endif

namespace Anki {
  
  namespace Vision {
    
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
      
      // Construct from a Json node
      CameraCalibration(const Json::Value& jsonNode);
      
#ifdef SIMULATOR
      CameraCalibration(const webots::Camera* camera);
#endif
      
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
      template<typename PRECISION = float>
      SmallSquareMatrix<3,PRECISION> get_calibrationMatrix() const;
      
      // Returns the inverse calibration matrix (e.g. for computing
      // image rays)
      template<typename PRECISION = float>
      SmallSquareMatrix<3,PRECISION> get_invCalibrationMatrix() const;
      
      void CreateJson(Json::Value& jsonNode) const;
      
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
    
    template<typename PRECISION>
    SmallSquareMatrix<3,PRECISION> CameraCalibration::get_calibrationMatrix() const
    {
      const PRECISION K_data[9] = {
        static_cast<PRECISION>(focalLength_x), static_cast<PRECISION>(focalLength_x*skew), static_cast<PRECISION>(center.x()),
        PRECISION(0),                          static_cast<PRECISION>(focalLength_y),      static_cast<PRECISION>(center.y()),
        PRECISION(0),                          PRECISION(0),                               PRECISION(1)};
      
      return SmallSquareMatrix<3,PRECISION>(K_data);
    } // get_calibrationMatrix()
    
    template<typename PRECISION>
    SmallSquareMatrix<3,PRECISION> CameraCalibration::get_invCalibrationMatrix() const
    {
      const PRECISION invK_data[9] = {
        static_cast<PRECISION>(1.f/focalLength_x),
        static_cast<PRECISION>(-skew/focalLength_y),
        static_cast<PRECISION>(center.y()*skew/focalLength_y - center.x()/focalLength_x),
        PRECISION(0),    static_cast<PRECISION>(1.f/focalLength_y),    static_cast<PRECISION>(-center.y()/focalLength_y),
        PRECISION(0),    PRECISION(0),                                 PRECISION(1)
      };
      
      return SmallSquareMatrix<3,PRECISION>(invK_data);
    }
    
    /*
     inline const std::array<float,5>& CameraCalibration::get_distortionCoeffs() const
     { return this->distortionCoeffs; }
     */
    
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
