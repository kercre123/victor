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

namespace Anki {
  namespace Vision{
    
    // Forward declaration:
    class ObservableObject;
    class Camera;
    
    
    
    
   
    
    
    //
    // A Vision "Marker" is a square fiducial surrounding a binary code, with
    // a known physical size.
    //
    class Marker
    {
    public:
      class Code
      {
      public:
        static const u8 NUM_PROBES = 9;
        static const u8 NUM_BYTES = static_cast<u8>(static_cast<f32>(NUM_PROBES*NUM_PROBES+4)/8.f + 0.5f);
        
        Code();
        Code(const u8* bytes);
        Code(std::initializer_list<u8> args);
        
        const u8* GetBytes() const;
        
        bool operator==(const Code& otherCode) const;
        
        // Required comparison object to use a Marker::Code in a STL container,
        // e.g. as a key for an std::map
        bool operator<(const Code& otherCode) const;
        
      protected:
        
        // TODO: Consider storing this as a std::array<u8, NUM_BYTES> ?
        //u8 byteArray_[NUM_BYTES];
        std::array<u8,NUM_BYTES> byteArray_;
        
      }; // class Marker::Code

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
      ObservedMarker(const Code& withCode, const Quad2f& corners, const Camera& seenBy);
      
      // Accessors:
      inline const Quad2f& GetImageCorners() const;
      inline const Camera& GetSeenBy()       const;
      
    protected:
      Quad2f         imgCorners_;
      Camera const&  seenBy_;
      
    };
    
    class KnownMarker : public Marker
    {
    public:
      
      KnownMarker(const Code& withCode, const Pose3d& atPose, const f32 size_mm);

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
      
      // Accessors
      inline const Quad3f& Get3dCorners() const;
      inline const Pose3d& GetPose()      const;
      inline const f32&    GetSize()      const;
      
    protected:
      static const Quad3f canonicalCorners3d_;
      
      Pose3d pose_;
      f32    size_; // in mm
      Quad3f corners3d_;

    };
    
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
    
    inline Camera const& ObservedMarker::GetSeenBy() const {
      return seenBy_;
    }
    
    /*
    class MarkerLibrary
    {
    public:
      
    protected:
      std::map<Marker::Code, Marker> knownMarkers;
      
    }; // class MarkerLibrary
    */
    
  } // namespace Vision
} // namespace Anki

#endif // __CoreTech_Vision__visionMarker__