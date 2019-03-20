//
//  visionMarker.h
//  CoreTech_Vision_Basestation
//
//  Created by Andrew Stein on 1/22/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#ifndef ANKI_CORETECH_VISION_MARKER_H
#define ANKI_CORETECH_VISION_MARKER_H

//#include <bitset>
#include <map>
#include <array>

#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/vision/engine/camera.h"

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
      using Code = s16; // TODO: this should be an Embedded::VisionMarkerType
      static const Code ANY_CODE;

      Marker(const Code& code);
      
      inline const Code& GetCode() const;
      
      const char* GetCodeName() const;
      static const char* GetNameForCode(Code code);
      
    protected:
      
      Code _code;
      
    }; // class Marker
    
    
    class ObservedMarker : public Marker
    {
    public:
      
      using UserHandle = int;
      static const UserHandle UnknownHandle = -1;
      
      ObservedMarker();
      
      // Instantiate a Marker from a given code, seen by a given camera with
      // the corners observed at the specified image coordinates.
      ObservedMarker(const TimeStamp_t t,
                     const Code& type,
                     const Quad2f& corners,
                     const Camera& seenBy,
                     UserHandle userHandle = UnknownHandle);
      
      // Const Accessors:
      inline const TimeStamp_t GetTimeStamp()     const;
      inline const Quad2f&     GetImageCorners()  const;
      inline const Camera&     GetSeenBy()        const;
      inline UserHandle        GetUserHandle()    const;
      
      inline void SetSeenBy(const Camera& camera);
      
      inline void SetImageCorners(const Quad2f& newCorners);
      
    protected:
      TimeStamp_t    _observationTime;
      Quad2f         _imgCorners;
      Camera         _seenByCam;
      int            _userHandle;
      
    }; // class ObservedMarker
    
    
    class KnownMarker : public Marker
    {
    public:
      
      KnownMarker(const Code& type, const Pose3d& atPose, const Point2f& size_mm);

      Result EstimateObservedPose(const ObservedMarker& obsMarker,
                                  Pose3d& pose) const;
      
      // Update this marker's pose and, in turn, its 3d corners' locations.
      //
      // Note that it is your responsibility to make sure the new pose has the
      // parent you intend! To preserve an existing parent, you may want to do
      // something like:
      //   newPose.SetParent(marker.GetParent());
      //   marker.SetPose(newPose);
      //
      void SetPose(const Pose3d& newPose);
      
      // Return true if all this marker's corners are visible from the given
      // camera, using current 3D poses of each. The marker must be within the
      // given angle tolerance of being front-parallel to the camera (i.e.
      // facing it) and have a diaonal image size at least the given image size
      // tolerance.
      bool IsVisibleFrom(const Camera& camera,
                         const f32     maxAngleRad,
                         const f32     minImageSize,
                         const bool    requireSomethingBehind,
                         const u16     xBorderPad = 0,
                         const u16     yBorderPad = 0) const;
      
      // Same as above, but also returns a reason code for why they marker was
      // not visible. Note that these are ordered, meaning if a given code is
      // returned, then none of the earlier ones are true. 
      enum class NotVisibleReason : u8 {
        IS_VISIBLE = 0, // if IsVisibleFrom() == true
        CAMERA_NOT_CALIBRATED,
        POSE_PROBLEM,
        BEHIND_CAMERA,
        NORMAL_NOT_ALIGNED,
        TOO_SMALL,
        OUTSIDE_FOV,
        OCCLUDED,
        NOTHING_BEHIND
      };
      
      bool IsVisibleFrom(const Camera& camera,
                         const f32     maxAngleRad,
                         const f32     minImageSize,
                         const bool    requireSomethingBehind,
                         const u16     xBorderPad,
                         const u16     yBorderPad,
                         NotVisibleReason& reason) const;
      
      // Accessors
      Quad3f  const& Get3dCorners() const; // at current pose
      Pose3d  const& GetPose()      const;
      Point2f const& GetSize()      const;
      
      Quad3f Get3dCorners(const Pose3d& atPose) const;
      
      // Return a Unit Normal vector to the face of the known marker
      Vec3f ComputeNormal() const; // at current pose
      Vec3f ComputeNormal(const Pose3d& atPose) const;
      
      void SetLastObservedTime(const TimeStamp_t atTime);
      TimeStamp_t GetLastObservedTime() const;
      
    protected:
      static const Quad3f _canonicalCorners3d;
      
      Pose3d       _pose;
      Point2f      _size; // in mm
      Quad3f       _corners3d;
      TimeStamp_t  _lastObservedTime;

    }; // class KnownMarker
    
    
    inline const TimeStamp_t ObservedMarker::GetTimeStamp() const {
      return _observationTime;
    }
    
    inline Quad2f const& ObservedMarker::GetImageCorners() const {
      return _imgCorners;
    }
    
    inline void ObservedMarker::SetImageCorners(const Quad2f& newCorners) {
      _imgCorners = newCorners;
    }
    
    inline ObservedMarker::UserHandle ObservedMarker::GetUserHandle() const {
      return _userHandle;
    }
    
    inline Quad3f const& KnownMarker::Get3dCorners() const {
      return _corners3d;
    }
    
    inline Pose3d const& KnownMarker::GetPose() const {
      return _pose;
    }
    
    inline Marker::Code const& Marker::GetCode() const {
      return _code;
    }
    
    inline Point2f const& KnownMarker::GetSize() const {
      return _size;
    }
    
    inline Camera const& ObservedMarker::GetSeenBy() const {
      return _seenByCam;
    }
    
    inline void ObservedMarker::SetSeenBy(const Camera& camera) {
      _seenByCam = camera;
    }
    
    inline void KnownMarker::SetLastObservedTime(const TimeStamp_t atTime) {
      _lastObservedTime = atTime;
    }
    
    inline TimeStamp_t KnownMarker::GetLastObservedTime() const {
      return _lastObservedTime;
    }

    inline bool KnownMarker::IsVisibleFrom(const Camera& camera,
                                           const f32     maxAngleRad,
                                           const f32     minImageSize,
                                           const bool    requireSomethingBehind,
                                           const u16     xBorderPad,
                                           const u16     yBorderPad) const
    {
      NotVisibleReason dummy;
      return IsVisibleFrom(camera, maxAngleRad, minImageSize, requireSomethingBehind, xBorderPad, yBorderPad, dummy);
    }

    const char* NotVisibleReasonToString(KnownMarker::NotVisibleReason reason);

  } // namespace Vision
} // namespace Anki

#endif // ANKI_CORETECH_VISION_MARKER_H
