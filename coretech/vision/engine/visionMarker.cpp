//
//  visionMarker.cpp
//  CoreTech_Vision_Basestation
//
//  Created by Andrew Stein on 1/22/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/visionMarker.h"

#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/matrix_impl.h"

#include "util/logging/logging.h"

#include "coretech/vision/shared/MarkerCodeDefinitions.h"

#include <random>

namespace Anki {
  namespace Vision{
  
    const Marker::Code Marker::ANY_CODE = std::numeric_limits<s16>::max();
    
    
    Marker::Marker(const Code& withCode)
    : _code(withCode)
    {
      
    }
    
    const char* Marker::GetNameForCode(Marker::Code code) {
      if (code >= 0 && code <= NUM_MARKER_TYPES) {
        return MarkerTypeStrings[code];
      } else if (code == Marker::ANY_CODE) {
        return "MARKER_ANY";
      } else {
        PRINT_NAMED_ERROR("Marker.GetNameForCode", "Could not look up name for code=%d", code);
        return "UNKNOWN";
      }
    }
    
    const char * Marker::GetCodeName() const
    {
      return GetNameForCode(_code);
    }
    
    ObservedMarker::ObservedMarker()
    : Marker(MARKER_UNKNOWN)
    , _userHandle(UnknownHandle)
    {
    
    }
    
    ObservedMarker::ObservedMarker(const TimeStamp_t t, const Code& withCode,
                                   const Quad2f& corners, const Camera& seenBy,
                                   UserHandle userHandle)
    : Marker(withCode)
    , _observationTime(t)
    , _imgCorners(corners)
    , _seenByCam(seenBy)
    , _userHandle(userHandle)
    {

    }
    
    
    KnownMarker::KnownMarker(const Code& withCode, const Pose3d& atPose, const Point2f& size_mm)
    : Marker(withCode)
    , _size(size_mm)
    , _lastObservedTime(0)
    {
      SetPose(atPose);
    }
    
    /*
    // Canonical 3D corners in camera coordinate system (where Z is depth)
    const Quad3f KnownMarker::canonicalCorners3d_ = {
      {-.5f, -.5f, 0.f},
      {-.5f,  .5f, 0.f},
      { .5f, -.5f, 0.f},
      { .5f,  .5f, 0.f}
    };
    */
    
    /*
    // Canonical corners in 3D are in the Y-Z plane, with X pointed away
    // (like the robot coordinate system: so if a robot is at the origin,
    //  pointed along the +x axis, then this marker would appear parallel
    //  to a forward-facing camera)
    const Quad3f KnownMarker::canonicalCorners3d_ = {
      { 0.f,  .5f,  .5f},  // TopLeft
      { 0.f,  .5f, -.5f},  // BottomLeft
      { 0.f, -.5f,  .5f},  // TopRight
      { 0.f, -.5f, -.5f}   // BottomRight
    };
    */
    
    // Canonical corners in X-Z plane
    const Quad3f KnownMarker::_canonicalCorners3d = {
      {-.5f, 0.f, .5f},
      {-.5f, 0.f,-.5f},
      { .5f, 0.f, .5f},
      { .5f, 0.f,-.5f}
    };

    void KnownMarker::SetPose(const Pose3d& newPose)
    {
      CORETECH_ASSERT(_size > 0.f);
      
      // Update the pose
      _pose = newPose;
      
      //
      // And also update the 3d corners accordingly...
      //
      _corners3d = Get3dCorners(_pose);
    }
    
    
    Quad3f KnownMarker::Get3dCorners(const Pose3d& atPose) const
    {
      // Start with canonical 3d quad corners:
      Quad3f corners3dAtPose(KnownMarker::_canonicalCorners3d);
      
      // Scale to this marker's physical size:
      for(auto & corner : corners3dAtPose) {
        // NOTE: canonical corners in X-Z plane, so the "y" size corresponds to
        // the canonical z axis
        corner.x() *= _size.x();
        corner.z() *= _size.y();
      }
      
      // Transform the canonical corners to this new pose
      atPose.ApplyTo(corners3dAtPose, corners3dAtPose);
      
      return corners3dAtPose;
      
    } // KnownMarker::Get3dCorners(atPose)
    
    // TODO: Make this part of Quadrilateral class?
    // NOTE: It assumes the quad is planar, which is not generally true for 3d quads
    inline Vec3f ComputeNormalHelper(const Quad3f& corners)
    {
      Vec3f edge1(corners[Quad::TopRight]);
      edge1 -= corners[Quad::TopLeft];
      
      Vec3f edge2(corners[Quad::BottomLeft]);
      edge2 -= corners[Quad::TopLeft];
      
      Vec3f normal( CrossProduct(edge2, edge1) );
      
      CORETECH_ASSERT(normal.MakeUnitLength() > 0.f);
      
      return normal;
    } // ComputeNormalHelper()
    
    Vec3f KnownMarker::ComputeNormal() const
    {
      return ComputeNormalHelper(Get3dCorners());
    } // ComputeNormal()
    
    
    Vec3f KnownMarker::ComputeNormal(const Pose3d& atPose) const
    {
      return ComputeNormalHelper(Get3dCorners(atPose));
    } // ComputeNormal(atPose)
                                    
                                
    bool KnownMarker::IsVisibleFrom(const Camera& camera,
                                    const f32     maxAngleRad,
                                    const f32     minImageSize,
                                    const bool    requireSomethingBehind,
                                    const u16     xBorderPad,
                                    const u16     yBorderPad,
                                    NotVisibleReason& reason) const
    {
      using namespace Quad;
      
      reason = NotVisibleReason::IS_VISIBLE;
      
      if(!camera.IsCalibrated())
      {
        // Can't check visibility with an uncalibrated camera
        PRINT_NAMED_WARNING("KnownMarker.IsVisibleFrom.CameraNotCalibrated", "");
        reason = NotVisibleReason::CAMERA_NOT_CALIBRATED;
        return false;
      }
      
      // Get the marker's pose relative to the camera
      Pose3d markerPoseWrtCamera;
      if(_pose.GetWithRespectTo(camera.GetPose(), markerPoseWrtCamera) == false) {
        PRINT_NAMED_WARNING("KnownMarker.IsVisibleFrom.NotInCameraPoseTree",
                            "Marker must be in the same pose tree as the camera to check its visibility");
        reason = NotVisibleReason::POSE_PROBLEM;
        return false;
      }
      
      // Make sure the marker is at least in front of the camera!
      if(markerPoseWrtCamera.GetTranslation().z() <= 0.f) {
        reason = NotVisibleReason::BEHIND_CAMERA;
        return false;
      }
      
      // Get the 3D positions of the marker's corners relative to the camera
      Quad3f markerCornersWrtCamera( Get3dCorners(markerPoseWrtCamera) );
      
      // Make sure the face of the marker is pointed towards the camera
      // Use "TopLeft" canonical corner as the local origin for computing the
      // face normal
      Vec3f topLine(markerCornersWrtCamera[TopRight]);
      topLine -= markerCornersWrtCamera[TopLeft];
      topLine.MakeUnitLength();
      
      Vec3f sideLine(markerCornersWrtCamera[BottomLeft]);
      sideLine -= markerCornersWrtCamera[TopLeft];
      sideLine.MakeUnitLength();
      
      const Point3f faceNormal( CrossProduct(sideLine, topLine) );
      const f32 dotProd = DotProduct(faceNormal, Z_AXIS_3D()); // TODO: Optimize to just: faceNormal.z()?
      if(dotProd > 0.f || acos(-dotProd) > maxAngleRad) { // TODO: Optimize to simple dotProd comparison
        reason = NotVisibleReason::NORMAL_NOT_ALIGNED;
        return false;
      }
      
      // Project the marker corners into the image
      Quad2f imgCorners;
      camera.Project3dPoints(markerCornersWrtCamera, imgCorners);
      
      // Make sure the projected marker size is big enough
      if((imgCorners[BottomRight] - imgCorners[TopLeft]).Length() < minImageSize ||
         (imgCorners[BottomLeft]  - imgCorners[TopRight]).Length() < minImageSize)
      {
        reason = NotVisibleReason::TOO_SMALL;
        return false;
      }
      
      // Make sure the projected corners are within the camera's field of view
      if(not camera.IsWithinFieldOfView(imgCorners[TopLeft], xBorderPad, yBorderPad)    ||
         not camera.IsWithinFieldOfView(imgCorners[TopRight], xBorderPad, yBorderPad)   ||
         not camera.IsWithinFieldOfView(imgCorners[BottomLeft], xBorderPad, yBorderPad) ||
         not camera.IsWithinFieldOfView(imgCorners[BottomRight], xBorderPad, yBorderPad))
      {
        reason = NotVisibleReason::OUTSIDE_FOV;
        return false;
      }
      
      // Make sure none of the projected corners are occluded
      // NOTE: occluder list uses closest point as the distance to the quad, so use that here as well
      f32 atDistance = std::numeric_limits<float>::max();
      for (const auto& corner : markerCornersWrtCamera) {
        atDistance = std::fmin(atDistance, corner.z());
      }

      if(camera.IsOccluded(imgCorners[TopLeft]    , atDistance) ||
         camera.IsOccluded(imgCorners[TopRight]   , atDistance) ||
         camera.IsOccluded(imgCorners[BottomLeft] , atDistance) ||
         camera.IsOccluded(imgCorners[BottomRight], atDistance))
      {
        reason = NotVisibleReason::OCCLUDED;
        return false;
      }
      
      if(requireSomethingBehind) {
        // Make sure there was something visible behind the marker's quad.
        // Otherwise, we don't know if we should have seen
        // it or if detection failed for some reason
        
        if(not camera.IsAnythingBehind(imgCorners, atDistance))
        {
          reason = NotVisibleReason::NOTHING_BEHIND;
          return false;
        }
      }
      
      // We passed all the checks, so the marker is visible
      return true;
      
    } // KnownMarker::IsVisibleFrom()
    
#   define NUM_AVERAGE_POSES 0
    
#   if NUM_AVERAGE_POSES > 1
    // TODO: Move these elsewhere?
    static std::random_device rd;
    static std::mt19937 rng(rd());
    static std::normal_distribution<> normalDist(0,0.05);
#   endif
    
    Result KnownMarker::EstimateObservedPose(const ObservedMarker& obsMarker,
                                             Pose3d& pose) const
    {
      const Camera& camera = obsMarker.GetSeenBy();

#     if NUM_AVERAGE_POSES > 1
      SmallMatrix<4, NUM_AVERAGE_POSES, f32> Q;
      Vec3f T = 0.f;
      
      std::vector<Pose3d> posesToAverage(NUM_AVERAGE_POSES);
      for(s32 i=0; i<NUM_AVERAGE_POSES; ++i)
      {
        Quad2f perturbedCorners(obsMarker.GetImageCorners());
        for(auto & corner : perturbedCorners) {
          corner.x() += normalDist(rng);
          corner.y() += normalDist(rng);
        }
        
        Result result = camera.ComputeObjectPose(perturbedCorners, _corners3d, pose);
        if(RESULT_OK != result) {
          return result;
        }
        
        T += pose.GetTranslation();
        
        Q.SetColumn(i, pose.GetRotation().GetQuaternion());
      }
      
      // Compute average translation
      T /= static_cast<f32>(NUM_AVERAGE_POSES);
      
      // Compute average rotation, which is the eigenvector corresponding to the
      // largest eigenvalue of QQ^T according to one of the responses in:
      // http://stackoverflow.com/questions/12374087/average-of-multiple-quaternions
      // which references: http://www.acsu.buffalo.edu/~johnc/ave_quat07.pdf
      // TODO: Make pose averaging a method of Pose3d
      cv::Mat_<f32> eigenvalues;
      cv::Mat_<f32> cvEigenvectors;
      cv::eigen(Q.get_CvMatx_() * Q.get_CvMatx_().t(), eigenvalues, cvEigenvectors);
      
      SmallMatrix<4,4,f32> eigenvectors(cvEigenvectors);
      pose.SetRotation(UnitQuaternion(eigenvectors.GetRow(0)));
      pose.SetTranslation(T);
      
      return RESULT_OK;
      
#     else 
      
      return camera.ComputeObjectPose(obsMarker.GetImageCorners(), _corners3d, pose);
      
#     endif // if NUM_AVERAGE_POSES > 1
      
    } // EstimateObservedPose()

    const char* NotVisibleReasonToString(KnownMarker::NotVisibleReason reason)
    {
      switch(reason) {
        case KnownMarker::NotVisibleReason::IS_VISIBLE: return "IS_VISIBLE";
        case KnownMarker::NotVisibleReason::CAMERA_NOT_CALIBRATED: return "CAMERA_NOT_CALIBRATED";
        case KnownMarker::NotVisibleReason::POSE_PROBLEM: return "POSE_PROBLEM";
        case KnownMarker::NotVisibleReason::BEHIND_CAMERA: return "BEHIND_CAMERA";
        case KnownMarker::NotVisibleReason::NORMAL_NOT_ALIGNED: return "NORMAL_NOT_ALIGNED";
        case KnownMarker::NotVisibleReason::TOO_SMALL: return "TOO_SMALL";
        case KnownMarker::NotVisibleReason::OUTSIDE_FOV: return "OUTSIDE_FOV";
        case KnownMarker::NotVisibleReason::OCCLUDED: return "OCCLUDED";
        case KnownMarker::NotVisibleReason::NOTHING_BEHIND: return "NOTHING_BEHIND";
      }
    }
    
  } // namespace Vision
} // namespace Anki
