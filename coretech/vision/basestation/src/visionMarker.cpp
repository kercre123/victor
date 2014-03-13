//
//  visionMarker.cpp
//  CoreTech_Vision_Basestation
//
//  Created by Andrew Stein on 1/22/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/visionMarker.h"

namespace Anki {
  namespace Vision{
  
    
    Marker::Marker(const Code& withCode)
    : code_(withCode)
    {
      
    }
    
    
    ObservedMarker::ObservedMarker(const Code& withCode, const Quad2f& corners, const Camera& seenBy)
    : Marker(withCode), imgCorners_(corners), seenBy_(seenBy)
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
      
      // Start with canonical 3d quad corners:
      corners3d_ = KnownMarker::canonicalCorners3d_;
      
      // Scale to this marker's physical size:
      corners3d_ *= size_;
      
      // Transform the canonical corners to this new pose
      pose_.applyTo(corners3d_, corners3d_);
      
    }
    
    
    Pose3d KnownMarker::EstimateObservedPose(const ObservedMarker& obsMarker) const
    {
      const Camera& camera = obsMarker.GetSeenBy();
      return camera.computeObjectPose(obsMarker.GetImageCorners(),
                                      this->corners3d_);
    }
    
    
  } // namespace Vision
} // namespace Anki