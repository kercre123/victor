//
//  camera.cpp
//  CoreTech_Vision
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include <algorithm>
#include <list>

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/shared/math/matrix_impl.h"
#include "coretech/common/engine/math/quad_impl.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/calib3d/calib3d.hpp"
#endif

#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/observableObject.h"
#include "coretech/vision/engine/perspectivePoseEstimation.h"


// Set to 1 to use OpenCV's iterative pose estimation for quads.
// Otherwise, the closed form P3P solution is used.
// NOTE: this currently only affects the ComputeObjectPose() that takes in quads.
#define USE_ITERATIVE_QUAD_POSE_ESTIMATION 1

namespace Anki {
  
  namespace Vision {
    
    Camera::Camera(void)
    : _camID(0)
    , _calibration(nullptr)
    {
      
    } // Constructor: Camera()
    
    Camera::Camera(const CameraID_t ID)
    : _camID(ID)
    , _calibration(nullptr)
    {
      
    }
    
    Camera::Camera(const Camera& other)
    : _camID(other._camID)
    , _calibration(other._calibration)
    , _pose(other._pose)
    , _occluderList(other._occluderList)
    {
      
    }
    
    Camera::~Camera()
    {
     
    }
    
    bool Camera::SetCalibration(std::shared_ptr<CameraCalibration> calib)
    {
      if(IsCalibrated() )
      {
        if(!ANKI_VERIFY(calib != nullptr, "Camera.SetCalibration.NullCalibration",
                        "Camera already calibrated. Cannot set with nullptr."))
        {
          return false;
        }
        
        // If Camera is already calibrated and the new one is the same, then nothing is actually changing.
        if(calib == _calibration)
        {
          return false;
        }
      }
      else if(calib == nullptr)
      {
        // The Camera isn't already calibrated and a null calibration is being passed in, nothing is actually changing.
        return false;
      }
      
      _calibration = calib;
      return true;
    }
    
#if ANKICORETECH_USE_OPENCV
    Pose3d Camera::ComputeObjectPoseHelper(const std::vector<cv::Point2f>& cvImagePoints,
                                           const std::vector<cv::Point3f>& cvObjPoints) const
    {
      CORETECH_THROW_IF(this->IsCalibrated() == false);
      
      cv::Vec3d cvRvec, cvTranslation;
      
      Matrix_3x3f calibMatrix(_calibration->GetCalibrationMatrix());
      
      const CameraCalibration::DistortionCoeffs& distCoeffs = _calibration->GetDistortionCoeffs();
      cv::Mat_<f32> distortionCoeffs(1, (s32)distCoeffs.size(), const_cast<f32*>(distCoeffs.data()));
      
      cv::solvePnP(cvObjPoints, cvImagePoints,
                   calibMatrix.get_CvMatx_(), distortionCoeffs,
                   cvRvec, cvTranslation,
                   false, cv::SOLVEPNP_ITERATIVE);
      
      RotationVector3d rvec(Vec3f(cvRvec[0], cvRvec[1], cvRvec[2]));
      Vec3f translation(cvTranslation[0], cvTranslation[1], cvTranslation[2]);
      
      // Return Pose object w.r.t. the camera's pose
      const Pose3d pose(rvec, translation, _pose);
      
      return pose;
      
    } // ComputeObjectPoseHelper()
    
#endif
    
    
    Pose3d Camera::ComputeObjectPose(const std::vector<Point2f>& imgPoints,
                                     const std::vector<Point3f>& objPoints) const
    {
      if(this->IsCalibrated() == false) {
        CORETECH_THROW("Camera::ComputeObjectPose() called before calibration set.");
      }
      
#if ANKICORETECH_USE_OPENCV
      std::vector<cv::Point2f> cvImagePoints;
      std::vector<cv::Point3f> cvObjPoints;
      
      for(const Point2f & imgPt : imgPoints) {
        cvImagePoints.emplace_back(imgPt.get_CvPoint_());
      }
      
      for(const Point3f & objPt : objPoints) {
        cvObjPoints.emplace_back(objPt.get_CvPoint3_());
      }
      
      return ComputeObjectPoseHelper(cvImagePoints, cvObjPoints);
      
#else
      // TODO: Implement!
      CORETECH_THROW("Unimplemented!")
      return Pose3d();
#endif
      
    } // ComputeObjectPose(from std::vectors)
    
    template<typename WORKING_PRECISION>
    Result Camera::ComputeObjectPose(const Quad2f& imgQuad,
                                     const Quad3f& worldQuad,
                                     Pose3d&       pose) const
    {
      if(this->IsCalibrated() == false) {
        CORETECH_THROW("Camera::ComputeObjectPose() called before calibration set.");
      }
      
#if USE_ITERATIVE_QUAD_POSE_ESTIMATION
      
      std::vector<cv::Point2f> cvImagePoints;
      std::vector<cv::Point3f> cvObjPoints;
      
      cvImagePoints.emplace_back(imgQuad[Quad::TopLeft].get_CvPoint_());
      cvImagePoints.emplace_back(imgQuad[Quad::BottomLeft].get_CvPoint_());
      cvImagePoints.emplace_back(imgQuad[Quad::TopRight].get_CvPoint_());
      cvImagePoints.emplace_back(imgQuad[Quad::BottomRight].get_CvPoint_());
      
      cvObjPoints.emplace_back(worldQuad[Quad::TopLeft].get_CvPoint3_());
      cvObjPoints.emplace_back(worldQuad[Quad::BottomLeft].get_CvPoint3_());
      cvObjPoints.emplace_back(worldQuad[Quad::TopRight].get_CvPoint3_());
      cvObjPoints.emplace_back(worldQuad[Quad::BottomRight].get_CvPoint3_());
      
      pose = ComputeObjectPoseHelper(cvImagePoints, cvObjPoints);
      return RESULT_OK;

#else
      
      // Turn the three image points into unit vectors corresponding to rays
      // in the direction of the image points
      const SmallSquareMatrix<3,WORKING_PRECISION> invK = this->GetCalibration().GetInvCalibrationMatrix<WORKING_PRECISION>();
      
      Quadrilateral<3, WORKING_PRECISION> imgRays, worldPoints;
      
      for(Quad::CornerName i_corner=Quad::FirstCorner; i_corner < Quad::NumCorners; ++i_corner)
      {
        // Get unit vector pointing along each image ray
        //   imgRay = K^(-1) * [u v 1]^T
        imgRays[i_corner].x() = static_cast<WORKING_PRECISION>(imgQuad[i_corner].x());
        imgRays[i_corner].y() = static_cast<WORKING_PRECISION>(imgQuad[i_corner].y());
        imgRays[i_corner].z() = WORKING_PRECISION(1);
        
        imgRays[i_corner] = invK * imgRays[i_corner];
        
        /*
        printf("point %d (%f, %f) became ray (%f, %f, %f) ",
               i_corner,
               imgQuad[i_corner].x(), imgQuad[i_corner].y(),
               imgRays[i_corner].x(), imgRays[i_corner].y(), imgRays[i_corner].z());
        */
        
        imgRays[i_corner].MakeUnitLength();
        
        //printf(" which normalized to (%f, %f, %f)\n",
        //       imgRays[i_corner].x(), imgRays[i_corner].y(), imgRays[i_corner].z());
        
        // cast each world quad into working precision quad
        worldPoints[i_corner].x() = static_cast<WORKING_PRECISION>(worldQuad[i_corner].x());
        worldPoints[i_corner].y() = static_cast<WORKING_PRECISION>(worldQuad[i_corner].y());
        worldPoints[i_corner].z() = static_cast<WORKING_PRECISION>(worldQuad[i_corner].z());
      }
      
      
      // Compute best pose from each subset of three corners, keeping the one
      // with the lowest error
      float minErrorOuter = std::numeric_limits<float>::max();
    
      std::array<Quad::CornerName,4> cornerList = {
        {Quad::TopLeft, Quad::BottomLeft, Quad::TopRight, Quad::BottomRight}
      };
  
      for(s32 i=0; i<4; ++i)
      {
        // Use the first corner in the current corner list as the validation
        // corner. Use the remaining three to estimate the pose.
        const Quad::CornerName i_validate = cornerList[0];
        
        //printf("Validating with %d, estimating with %d, %d, %d\n",
        //       i_validate, cornerList[1], cornerList[2], cornerList[3]);
        
        std::array<Pose3d,4> possiblePoses;
        Result lastResult = P3P::computePossiblePoses(worldPoints[cornerList[1]],
                                                      worldPoints[cornerList[2]],
                                                      worldPoints[cornerList[3]],
                                                      imgRays[cornerList[1]],
                                                      imgRays[cornerList[2]],
                                                      imgRays[cornerList[3]],
                                                      possiblePoses);
        if(lastResult != RESULT_OK) {
          return lastResult;
        }
        
        // Find the pose with the least reprojection error for the 4th
        // validation corner (which was not used in estimating the pose)
        s32 bestSolution = -1;
        float minErrorInner = std::numeric_limits<float>::max();
        
        for(s32 i_solution=0; i_solution<4; ++i_solution)
        {
          // First make sure this solution puts the object in front of the
          // camera (this should be true for two solutions)
          if(possiblePoses[i_solution].GetTranslation().z() > 0)
          {
            Point2f projectedPoint;
            this->Project3dPoint(possiblePoses[i_solution]*worldQuad[i_validate], projectedPoint);
            
            float error = (projectedPoint - imgQuad[i_validate]).Length();
            
            if(error < minErrorInner) {
              minErrorInner = error;
              bestSolution = i_solution;
            }
          } // if translation.z > 0
        } // for each solution
        
        CORETECH_ASSERT(bestSolution >= 0);
        
        // If the pose using this validation corner is better than the
        // best so far, keep it
        if(minErrorInner < minErrorOuter) {
          minErrorOuter = minErrorInner;
          pose = possiblePoses[bestSolution];
        }
        
        if(i<3) {
          // Rearrange corner list for next loop, to get a different
          // validation corner each time
          std::swap(cornerList[0], cornerList[i+1]);
        }
        
      } // for each validation corner
      
      // Make sure to make the returned pose w.r.t. the camera!
      pose.SetParent(&_pose);
      
      return RESULT_OK;
      
#endif // #if USE_ITERATIVE_QUAD_POSE_ESTIMATION
      
    } // ComputeObjectPose(from quads)
 
    
    // Explicit instantiation for single and double precision
    template Result Camera::ComputeObjectPose<float>(const Quad2f& imgQuad,
                                                     const Quad3f& worldQuad,
                                                     Pose3d& pose) const;

    template Result Camera::ComputeObjectPose<double>(const Quad2f& imgQuad,
                                                      const Quad3f& worldQuad,
                                                      Pose3d& pose) const;

    void Camera::AddOccluder(const std::vector<Point2f>& projectedPoints,
                             const f32 atDistance)
    {
      _occluderList.AddOccluder(projectedPoints, atDistance);
    }
    
    bool Camera::IsWithinFieldOfView(const Point2f &projectedPoint,
                                     const u16 xBorderPad,
                                     const u16 yBorderPad) const
    {
      if(!IsCalibrated())
      {
        // Can't check if within FOV if camera is not calibrated
        PRINT_NAMED_WARNING("Camera.IsWithinFieldOfView.NotCalibrated", "");
        return false;
      }
      
      return (not std::isnan(projectedPoint.x()) &&
              not std::isnan(projectedPoint.y()) &&
              projectedPoint.x() >= (0.f + static_cast<f32>(xBorderPad)) &&
              projectedPoint.y() >= (0.f + static_cast<f32>(yBorderPad)) &&
              projectedPoint.x() <  static_cast<f32>(_calibration->GetNcols() - xBorderPad) &&
              projectedPoint.y() <  static_cast<f32>(_calibration->GetNrows() - yBorderPad));
      
    } // Camera::IsVisible()

    
    bool Camera::Project3dPoint(const Point3f& objPoint,
                                Point2f&       imgPoint) const
    {
      const f32 BEHIND_OR_OCCLUDED = std::numeric_limits<f32>::quiet_NaN();
      
      if(this->IsCalibrated() == false) {
        PRINT_NAMED_WARNING("Camera.Project3dPoint.NoCalibration", "Called before calibration set");
        imgPoint = BEHIND_OR_OCCLUDED;
        return false;
      }
      
      if(objPoint.z() <= 0.f)
      {
        // Point not visible (not in front of camera)
        imgPoint = BEHIND_OR_OCCLUDED;
        return false;
      } else {
        // Point visible, distort and project it
        imgPoint.x() = objPoint.x() / objPoint.z();
        imgPoint.y() = objPoint.y() / objPoint.z();
        
        auto const& k = _calibration->GetDistortionCoeffs();
        if(!k.empty())
        {
          // Radial/tangental distortion
          // Uses OpenCV model here: http://docs.opencv.org/2.4/doc/tutorials/calib3d/camera_calibration/camera_calibration.html
          
          DEV_ASSERT_MSG(k.size() == 4 || k.size() == 5 || k.size() == 8,
                         "Camera.Project3dPoints.UnsupportedNumDistCoeffs", "N=%zu", k.size());
          
          const f32 x2 = imgPoint.x() * imgPoint.x();
          const f32 y2 = imgPoint.y() * imgPoint.y();
          const f32 r2 = x2 + y2;
          const f32 r4 = r2*r2;
          const f32 a1 = 2.f*imgPoint.x()*imgPoint.y();
          const f32 a2 = r2 + 2.f*x2;
          const f32 a3 = r2 + 2.f*y2;
          f32 cdist = 1.f + k[0]*r2 + k[1]*r4;
          if(k.size() >= 5)
          {
            const f32 r6 = r4*r2;
            cdist += k[4]*r6;
            
            if(k.size() == 8)
            {
              const f32 icdist2 = 1.f/(1.f + k[5]*r2 + k[6]*r4 + k[7]*r6);
              cdist *= icdist2;
            }
          }
          
          imgPoint.x() = imgPoint.x()*cdist + k[2]*a1 + k[3]*a2; // TODO: numCoeffs > 8: + k[8]*r2+k[9]*r4;
          imgPoint.y() = imgPoint.y()*cdist + k[2]*a3 + k[3]*a1; // TODO: numCoeffs > 8: + k[10]*r2+k[11]*r4;
          
        }
        
        imgPoint.x() *= _calibration->GetFocalLength_x();
        imgPoint.y() *= _calibration->GetFocalLength_y();
        
        imgPoint += _calibration->GetCenter();
        return true;
      }
      
    } // Project3dPoint()
    
    
    /* This doesn't work but it would be nice to have a "generic"
     wrapper here for looping over various containers of points and projecting
     them.
     
    template<template<class PointType> class PointContainer>
    void Project3dPointContainer(const PointContainer<Point3f>& objPoints,
                                 PointContainer<Point2f>& imgPoints)
    {
      CORETECH_ASSERT(objPoints.size() == imgPoints.size());
      
      auto objPointIter = objPoints.begin();
      auto imgPointIter = imgPoints.begin();
      
      while(objPointIter != objPoints.end()) {
        
        Project3dPoint(*objPointIter, *imgPointIter);
        
        ++objPointIter;
        ++imgPointIter;
      }
    }
     */
    
    template<class PointContainer3d, class PointContainer2d>
    void Camera::Project3dPointHelper(const PointContainer3d& objPoints,
                                      PointContainer2d& imgPoints) const
    {
      CORETECH_ASSERT(objPoints.size() == imgPoints.size());
      
      auto objPointIter = objPoints.begin();
      auto imgPointIter = imgPoints.begin();
      
      while(objPointIter != objPoints.end()) {
        
        Project3dPoint(*objPointIter, *imgPointIter);
        
        ++objPointIter;
        ++imgPointIter;
      }
    } // Camera::Project3dPointHelper()
    
    
    // Compute the projected image locations of a set of 3D points:
    void Camera::Project3dPoints(const std::vector<Point3f>& objPoints,
                                 std::vector<Point2f>&       imgPoints) const
    {
      imgPoints.resize(objPoints.size());
      Project3dPointHelper(objPoints, imgPoints);
    } // Project3dPoints(std::vectors)
    
    void Camera::Project3dPoints(const Quad3f& objPoints,
                                 Quad2f&       imgPoints) const
    {
      for(Quad::CornerName i_corner=Quad::FirstCorner;
          i_corner < Quad::NumCorners; ++i_corner)
      {
        Project3dPoint(objPoints[i_corner], imgPoints[i_corner]);
      }
    } // Project3dPoints(Quads)
    
    
    void Camera::ClearOccluders()
    {
      _occluderList.Clear();
    }
    
    bool Camera::ProjectObject(const ObservableObject&  object,
                               std::vector<Point2f>&    projectedCorners,
                               f32&                     distanceFromCamera) const
    {
      Pose3d objectPoseWrtCamera;
      if(object.GetPose().GetWithRespectTo(_pose, objectPoseWrtCamera) == false) {
        PRINT_NAMED_ERROR("Camera.AddOccluder.ObjectDoesNotShareOrigin",
                          "Object must be in the same pose tree as the camera to add it as an occluder");
        return false;
      } else {
        std::vector<Point3f> cornersAtPose;
        
        // Project the objects's corners into the image and create an occluding
        // bounding rectangle from that
        object.GetCorners(objectPoseWrtCamera, cornersAtPose);
        Project3dPoints(cornersAtPose, projectedCorners);
        distanceFromCamera = objectPoseWrtCamera.GetTranslation().z();
        return true;
      }
    } // ProjectObject()
    
    
    void Camera::AddOccluder(const ObservableObject& object)
    {
      
      std::vector<Point2f> projectedCorners;
      f32 atDistance;
      if(ProjectObject(object, projectedCorners, atDistance))
      {
        AddOccluder(projectedCorners, atDistance);
      }
    } // AddOccluder(ObservableObject)
    
    
    void Camera::AddOccluder(const KnownMarker& marker)
    {
      Pose3d markerPoseWrtCamera;
      if(marker.GetPose().GetWithRespectTo(_pose, markerPoseWrtCamera) == false) {
        PRINT_NAMED_ERROR("Camera.AddOccluder.MarkerDoesNotShareOrigin",
                          "Marker must be in the same pose tree as the camera to add it as an occluder");
      } else {
        
        const Quad3f markerCorners = marker.Get3dCorners(markerPoseWrtCamera);
        
        Quad2f imgCorners;
        Project3dPoints(markerCorners, imgCorners);
        
        // Use closest point as the distance to the quad
        auto cornerIter = markerCorners.begin();
        f32 atDistance = cornerIter->z();
        ++cornerIter;
        while(cornerIter != markerCorners.end()) {
          if(cornerIter->z() < atDistance) {
            atDistance = cornerIter->z();
          }
          ++cornerIter;
        }
        
        _occluderList.AddOccluder(imgCorners, atDistance);
      }

    } // AddOccluder(Quad3f)
   
    void Camera::ComputePanAndTiltAngles(const Point2f& imgPoint, Radians& relPanAngle, Radians& relTiltAngle) const
    {

      // centering the image point
      const Point2f centeredPoint = imgPoint - _calibration->GetCenter();

      relTiltAngle = std::atan(-centeredPoint.y() / _calibration->GetFocalLength_y());
      relPanAngle  = std::atan(-centeredPoint.x() / _calibration->GetFocalLength_x());
    }
    
  } // namespace Vision
} // namespace Anki
