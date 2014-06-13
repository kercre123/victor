//
//  visionMarker.h
//  CoreTech_Vision_Basestation
//
//  Created by Andrew Stein on 1/22/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#ifndef __CoreTech_Vision__visionMarker__
#define __CoreTech_Vision__visionMarker__

//#include <bitset>
#include <map>
#include <array>

#include "anki/common/basestation/math/quad.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/vision/basestation/camera.h"

namespace Anki {
  namespace Vision{
    
    // Forward declaration:
    class ObservableObject;
    class Camera;
    
    //
    // A Vision "Marker" is a square fiducial surrounding a binary code
    //
    class Marker
    {
    public:
      typedef u16 Code; // TODO: this should be an Embedded::VisionMarkerType

      Marker(const Code& code);
      
      inline const Code& GetCode() const;
      
    protected:
      
      Code code_;
      
    }; // class Marker
    
    
    class ObservedMarker : public Marker
    {
    public:
      // Instantiate a Marker from a given code, seen by a given camera with
      // the corners observed at the specified image coordinates.
      ObservedMarker(const TimeStamp_t t,
                     const Code& type, const Quad2f& corners,
                     const Camera& seenBy);
      
      // Accessors:
      inline const TimeStamp_t GetTimeStamp() const;
      inline const Quad2f& GetImageCorners() const;
      inline const Camera& GetSeenBy()       const;
      
      inline void MarkUsed(bool used);
      inline bool IsUsed();
    protected:
      TimeStamp_t    observationTime_;
      Quad2f         imgCorners_;
      Camera         seenByCam_;
      
      bool           used_;
    }; // class ObservedMarker
    
    
    class KnownMarker : public Marker
    {
    public:
      
      KnownMarker(const Code& type, const Pose3d& atPose, const f32 size_mm);

      Pose3d EstimateObservedPose(const ObservedMarker& obsMarker) const;
      
      // Update this marker's pose and, in turn, its 3d corners' locations.
      //
      // Note that it is your responsibility to make sure the new pose has the
      // parent you intend! To preserve an existing parent, you may want to do
      // something like:
      //   newPose.set_parent(marker.get_parent());
      //   marker.SetPose(newPose);
      //
      void SetPose(const Pose3d& newPose);
      
      // Return true if all this marker's corners are visible from the given
      // camera, using current 3D poses of each. The marker must be within the
      // given angle tolerance of being front-parallel to the camera (i.e.
      // facing it) and have a diaonal image size at least the given image size
      // tolerance.
      bool IsVisibleFrom(const Camera& camera,
                         const f32 maxAngleRad,
                         const f32 minImageSize,
                         const bool requireSomethingBehind) const;
      
      // Accessors
      Quad3f const& Get3dCorners() const; // at current pose
      Pose3d const& GetPose()      const;
      f32    const& GetSize()      const;
      
      Quad3f Get3dCorners(const Pose3d& atPose) const;
      
      // Return a Unit Normal vector to the face of the known marker
      Vec3f ComputeNormal() const; // at current pose
      Vec3f ComputeNormal(const Pose3d& atPose) const;
      
      void SetLastObservedTime(const TimeStamp_t atTime);
      TimeStamp_t GetLastObservedTime() const;
      
    protected:
      static const Quad3f canonicalCorners3d_;
      
      Pose3d pose_;
      f32    size_; // in mm
      Quad3f corners3d_;
      //bool   wasObserved_;
      TimeStamp_t lastObservedTime_;

    }; // class KnownMarker
    
    
    inline const TimeStamp_t ObservedMarker::GetTimeStamp() const {
      return observationTime_;
    }
    
    inline Quad2f const& ObservedMarker::GetImageCorners() const {
      return imgCorners_;
    }
    
    inline Quad3f const& KnownMarker::Get3dCorners() const {
      return corners3d_;
    }
    
    inline Pose3d const& KnownMarker::GetPose() const {
      return pose_;
    }
    
    inline Marker::Code const& Marker::GetCode() const {
      return code_;
    }
    
    inline f32 const& KnownMarker::GetSize() const {
      return size_;
    }
    
    inline Camera const& ObservedMarker::GetSeenBy() const {
      return seenByCam_;
    }
    
    inline void ObservedMarker::MarkUsed(bool used) {
      used_ = used;
    }
    
    inline bool ObservedMarker::IsUsed() {
      return used_;
    }
    
    inline void KnownMarker::SetLastObservedTime(const TimeStamp_t atTime) {
      lastObservedTime_ = atTime;
    }
    
    inline TimeStamp_t KnownMarker::GetLastObservedTime() const {
      return lastObservedTime_;
    }

  } // namespace Vision
} // namespace Anki

#endif // __CoreTech_Vision__visionMarker__
