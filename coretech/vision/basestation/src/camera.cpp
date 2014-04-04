//
//  camera.cpp
//  CoreTech_Vision
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include <algorithm>
#include <list>

#include "anki/common/basestation/jsonTools.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/calib3d/calib3d.hpp"
#endif

#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/perspectivePoseEstimation.h"

// Set to 1 to use OpenCV's iterative pose estimation for quads.
// Otherwise, the closed form P3P solution is used.
// NOTE: this currently only affects the computeObjectPose() that takes in quads.
#define USE_ITERATIVE_QUAD_POSE_ESTIMATION 0

namespace Anki {
  
  namespace Vision {
    
    Camera::Camera(void)
    : isCalibrationSet(false)
    {
      
    } // Constructor: Camera()
    
    Camera::Camera(const CameraCalibration &calib_in,
                   const Pose3d& pose_in)
    : calibration(calib_in), isCalibrationSet(true), pose(pose_in)
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
    
    CameraCalibration::CameraCalibration(const Json::Value &jsonNode)
    {
      CORETECH_ASSERT(jsonNode.isMember("nrows"));
      nrows = JsonTools::GetValue<u16>(jsonNode["nrows"]);
      
      CORETECH_ASSERT(jsonNode.isMember("ncols"));
      ncols = JsonTools::GetValue<u16>(jsonNode["ncols"]);
      
      CORETECH_ASSERT(jsonNode.isMember("focalLength_x"));
      focalLength_x = JsonTools::GetValue<f32>(jsonNode["focalLength_x"]);
      
      CORETECH_ASSERT(jsonNode.isMember("focalLength_y"))
      focalLength_y = JsonTools::GetValue<f32>(jsonNode["focalLength_y"]);
      
      CORETECH_ASSERT(jsonNode.isMember("center_x"))
      center.x() = JsonTools::GetValue<f32>(jsonNode["center_x"]);
      
      CORETECH_ASSERT(jsonNode.isMember("center_y"))
      center.y() = JsonTools::GetValue<f32>(jsonNode["center_y"]);
      
      CORETECH_ASSERT(jsonNode.isMember("skew"))
      skew = JsonTools::GetValue<f32>(jsonNode["skew"]);
      
      // TODO: Add distortion coefficients
    }

    
    void CameraCalibration::CreateJson(Json::Value& jsonNode) const
    {
      jsonNode["nrows"] = nrows;
      jsonNode["ncols"] = ncols;
      jsonNode["focalLength_x"] = focalLength_x;
      jsonNode["focalLength_y"] = focalLength_y;
      jsonNode["center_x"] = center.x();
      jsonNode["center_y"] = center.y();
      jsonNode["skew"] = skew;
    }
    
    
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
    
    
    Pose3d Camera::computeObjectPose(const std::vector<Point2f>& imgPoints,
                                     const std::vector<Point3f>& objPoints) const
    {
      if(not isCalibrationSet) {
        CORETECH_THROW("Camera::computeObjectPose() called before calibration set.");
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
      
      return computeObjectPoseHelper(cvImagePoints, cvObjPoints);
      
#else
      // TODO: Implement!
      CORETECH_THROW("Unimplemented!")
      return Pose3d();
#endif
      
    } // computeObjectPose(from std::vectors)
    
    template<typename WORKING_PRECISION>
    Pose3d Camera::computeObjectPose(const Quad2f& imgQuad,
                                     const Quad3f& worldQuad) const
    {
      if(not isCalibrationSet) {
        CORETECH_THROW("Camera::computeObjectPose() called before calibration set.");
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
      
      return computeObjectPoseHelper(cvImagePoints, cvObjPoints);

#else
      
      Pose3d pose;
      
      
      // Turn the three image points into unit vectors corresponding to rays
      // in the direction of the image points
      const SmallSquareMatrix<3,WORKING_PRECISION> invK = this->get_calibration().get_invCalibrationMatrix<WORKING_PRECISION>();
      
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
        
        imgRays[i_corner].makeUnitLength();
        
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
        P3P::computePossiblePoses(worldPoints[cornerList[1]],
                                  worldPoints[cornerList[2]],
                                  worldPoints[cornerList[3]],
                                  imgRays[cornerList[1]],
                                  imgRays[cornerList[2]],
                                  imgRays[cornerList[3]],
                                  possiblePoses);
        
        // Find the pose with the least reprojection error for the 4th
        // validation corner (which was not used in estimating the pose)
        s32 bestSolution = -1;
        float minErrorInner = std::numeric_limits<float>::max();
        
        for(s32 i_solution=0; i_solution<4; ++i_solution)
        {
          Point2f projectedPoint;
          this->project3dPoint(possiblePoses[i_solution]*worldQuad[i_validate], projectedPoint);
          
          float error = (projectedPoint - imgQuad[i_validate]).length();
          
          if(error < minErrorInner) {
            minErrorInner = error;
            bestSolution = i_solution;
          }
          
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
      pose.set_parent(&this->pose);
      
      return pose;
      
#endif // #if USE_ITERATIVE_QUAD_POSE_ESTIMATION
      
    } // computeObjectPose(from quads)
 
    
    // Explicit instantiation for single and double precision
    template Pose3d Camera::computeObjectPose<float>(const Quad2f& imgQuad,
                                                     const Quad3f& worldQuad) const;

    template Pose3d Camera::computeObjectPose<double>(const Quad2f& imgQuad,
                                                      const Quad3f& worldQuad) const;

    
    void  Camera::project3dPoint(const Point3f& objPoint,
                                 Point2f&       imgPoint) const
    {
      if(not isCalibrationSet) {
        CORETECH_THROW("Camera::project3dPoint() called before calibration set.");
      }
      
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
    void Camera::project3dPoints(const std::vector<Point3f>& objPoints,
                                 std::vector<Point2f>&       imgPoints) const
    {
      imgPoints.resize(objPoints.size());
      for(size_t i = 0; i<objPoints.size(); ++i)
      {
        project3dPoint(objPoints[i], imgPoints[i]);
      }
    } // project3dPoints(std::vectors)
    
    void Camera::project3dPoints(const Quad3f& objPoints,
                                 Quad2f&       imgPoints) const
    {
      for(Quad::CornerName i_corner=Quad::FirstCorner;
          i_corner < Quad::NumCorners; ++i_corner)
      {
        project3dPoint(objPoints[i_corner], imgPoints[i_corner]);
      }
    } // project3dPoints(Quads)
    
  } // namespace Vision
} // namespace Anki