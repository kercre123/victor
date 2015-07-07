//
//  visionMarker.cpp
//  CoreTech_Vision_Basestation
//
//  Created by Andrew Stein on 1/22/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/visionMarker.h"

#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "util/logging/logging.h"

#include "anki/vision/MarkerCodeDefinitions.h"

namespace Anki {
  namespace Vision{
  
    const Marker::Code Marker::ANY_CODE = s16_MAX;
    const Marker::Code Marker::FACE_CODE = s16_MIN;
    
    Marker::Marker(const Code& withCode)
    : _code(withCode)
    {
      
    }
    
    const char* Marker::GetCodeName() const
    {
      if(_code >= 0 && _code < NUM_MARKER_TYPES) {
        return MarkerTypeStrings[_code];
      }
      else if(_code == FACE_CODE) {
        return "HUMAN_FACE_MARKER";
      }
      else {
        PRINT_NAMED_ERROR("Marker.GetCodeName", "Could not look up name for code=%d.\n", _code);
        return MarkerTypeStrings[MARKER_UNKNOWN];
      }
    }
    
    ObservedMarker::ObservedMarker(const TimeStamp_t t, const Code& withCode, const Quad2f& corners, const Camera& seenBy)
    : Marker(withCode)
    , _observationTime(t)
    , _imgCorners(corners)
    , _seenByCam(seenBy)
    , _used(false)
    {

    }
    
    
    KnownMarker::KnownMarker(const Code& withCode, const Pose3d& atPose, const f32 size_mm)
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
      corners3dAtPose *= _size;
      
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
      
      const f32 normalLength = normal.MakeUnitLength();
      CORETECH_ASSERT(normalLength > 0.f);
      
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
      
      // Get the marker's pose relative to the camera
      Pose3d markerPoseWrtCamera;
      if(_pose.GetWithRespectTo(camera.GetPose(), markerPoseWrtCamera) == false) {
        PRINT_NAMED_WARNING("KnownMarker.IsVisibleFrom.NotInCameraPoseTree",
                            "Marker must be in the same pose tree as the camera to check its visibility.\n");
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
      if(camera.IsOccluded(imgCorners[TopLeft]    , markerCornersWrtCamera[TopLeft].z())    ||
         camera.IsOccluded(imgCorners[TopRight]   , markerCornersWrtCamera[TopRight].z())   ||
         camera.IsOccluded(imgCorners[BottomLeft] , markerCornersWrtCamera[BottomLeft].z()) ||
         camera.IsOccluded(imgCorners[BottomRight], markerCornersWrtCamera[BottomRight].z()))
      {
        reason = NotVisibleReason::OCCLUDED;
        return false;
      }
      
      if(requireSomethingBehind) {
        // Make sure there was something visible behind the marker's quad.
        // Otherwise, we don't know if we should have seen
        // it or if detection failed for some reason
        const f32 atDistance = 0.25f*(markerCornersWrtCamera[TopLeft].z() +
                                      markerCornersWrtCamera[TopRight].z() +
                                      markerCornersWrtCamera[BottomLeft].z() +
                                      markerCornersWrtCamera[BottomRight].z());
        
        if(not camera.IsAnythingBehind(imgCorners, atDistance))
        {
          reason = NotVisibleReason::NOTHING_BEHIND;
          return false;
        }
      }
      
      // We passed all the checks, so the marker is visible
      return true;
      
    } // KnownMarker::IsVisibleFrom()
    
    
    Result KnownMarker::EstimateObservedPose(const ObservedMarker& obsMarker,
                                             Pose3d& pose) const
    {
      const Camera& camera = obsMarker.GetSeenBy();
      return camera.ComputeObjectPose(obsMarker.GetImageCorners(), _corners3d, pose);
    }
    
    
  } // namespace Vision
} // namespace Anki
