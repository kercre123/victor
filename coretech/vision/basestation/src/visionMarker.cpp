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

namespace Anki {
  namespace Vision{
  
    
    Marker::Marker(const Code& withCode)
    : code_(withCode)
    {
      
    }
    
    
    ObservedMarker::ObservedMarker(const TimeStamp_t t, const Code& withCode, const Quad2f& corners, const Camera& seenBy)
    : t_(t), Marker(withCode), imgCorners_(corners), seenByCam_(seenBy)
    {

    }
    
    
    KnownMarker::KnownMarker(const Code& withCode, const Pose3d& atPose, const f32 size_mm)
    : Marker(withCode), size_(size_mm)
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
    const Quad3f KnownMarker::canonicalCorners3d_ = {
      {-.5f, 0.f, .5f},
      {-.5f, 0.f,-.5f},
      { .5f, 0.f, .5f},
      { .5f, 0.f,-.5f}
    };

    void KnownMarker::SetPose(const Pose3d& newPose)
    {
      CORETECH_ASSERT(size_ > 0.f);
      
      // Update the pose
      pose_ = newPose;
      
      //
      // And also update the 3d corners accordingly...
      //
      corners3d_ = Get3dCorners(pose_);
    }
    
    
    Quad3f KnownMarker::Get3dCorners(const Pose3d& atPose) const
    {
      // Start with canonical 3d quad corners:
      Quad3f corners3dAtPose(KnownMarker::canonicalCorners3d_);
      
      // Scale to this marker's physical size:
      corners3dAtPose *= size_;
      
      // Transform the canonical corners to this new pose
      atPose.applyTo(corners3dAtPose, corners3dAtPose);
      
      return corners3dAtPose;
      
    } // KnownMarker::Get3dCorners(atPose)
    
    
    bool KnownMarker::IsVisibleFrom(Camera& camera,
                                    const f32 maxAngleRad,
                                    const f32 minImageSize) const
    {
      using namespace Quad;
      
      // Get the marker's pose relative to the camera
      Pose3d markerPoseWrtCamera( pose_.getWithRespectTo(&camera.get_pose()) );
      
      // Make sure the marker is at least in front of the camera!
      if(markerPoseWrtCamera.get_translation().z() <= 0.f) {
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
      const f32 dotProd = DotProduct(faceNormal, Z_AXIS_3D); // TODO: Optimize to just: faceNormal.z()?
      if(dotProd > 0.f || acos(-dotProd) > maxAngleRad) {
        return false;
      }
      
      // Make sure the projected corners are within the image
      // TODO: add some border padding?
      Quad2f imgCorners;
      camera.Project3dPoints(markerCornersWrtCamera, imgCorners);
      if(not camera.IsVisible(imgCorners[TopLeft]) ||
         not camera.IsVisible(imgCorners[TopRight]) ||
         not camera.IsVisible(imgCorners[BottomLeft]) ||
         not camera.IsVisible(imgCorners[BottomRight]))
      {
        return false;
      }
      
      // Make sure the projected marker size is big enough
      if((imgCorners[BottomRight] - imgCorners[TopLeft]).Length() < minImageSize ||
         (imgCorners[BottomLeft]  - imgCorners[TopRight]).Length() < minImageSize)
      {
        return false;
      }
      
      // We passed all the checks, so the marker is visible
      return true;
      
    } // KnownMarker::IsVisibleFrom()
    
    
    Pose3d KnownMarker::EstimateObservedPose(const ObservedMarker& obsMarker) const
    {
      const Camera& camera = obsMarker.GetSeenBy();
      return camera.ComputeObjectPose(obsMarker.GetImageCorners(),
                                      this->corners3d_);
    }
    
    
  } // namespace Vision
} // namespace Anki