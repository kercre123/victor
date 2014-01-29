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
    void Marker::Init()
    {
      // If this is a known code, set the size and name
      auto knownInfo = Marker::library_.find(code_);
      
      isKnown_ = knownInfo != Marker::library_.end();
      if(isKnown_) {
        name_ = knownInfo->second.name;
        size_ = knownInfo->second.size;
      }
    }
     */
    
    /*
    std::map<Marker::Code, Marker::MarkerInfo> Marker::InitLibrary()
    {
      // TODO: Read this from config somehow?
      std::map<Code, MarkerInfo> lib;
     
      Code code({0,0,0,0,0,0,0,0,0,0,0});
      lib[code] = {.size = 32.f, .name = "UNKNOWN"};
      
      return lib;
    }
    
    std::map<Marker::Code, Marker::MarkerInfo> Marker::library_ = Marker::InitLibrary();
    */
    
    /*
    std::map<Marker::Code, Marker::MarkerInfo> Marker::library_ = {
      {{0,0,0,0,0,0,0,0,0,0,0}, {.size = 32.f, .name = "UNKNOWN"}} // each entry can be added like this
    };
    */
    
    
    /*
    // Canonical markers in X-Y (e.g. image) plane
    const Quad3f KnownMarker::canonicalCorners3d_ = Quad3f(Point3f(-.5f, -.5f, 0.f),
                                                      Point3f(-.5f,  .5f, 0.f),
                                                      Point3f( .5f, -.5f, 0.f),
                                                      Point3f( .5f,  .5f, 0.f));
     */
    
    // Canonical corners in 3D are in the X-Z plane
    const Quad3f KnownMarker::canonicalCorners3d_ = {
      {-.5f, 0.f,  .5f},  // TopLeft
      {-.5f, 0.f, -.5f},  // BottomLeft
      { .5f, 0.f,  .5f},  // TopRight
      { .5f, 0.f, -.5f}   // BottomRight
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
    
    
    bool Marker::Code::operator==(const Code& otherCode) const
    {
      bool isSame = otherCode.byteArray_[0] == this->byteArray_[0];
      for(u8 i=0; isSame && i<NUM_BYTES; ++i) {
        isSame = otherCode.byteArray_[i] == this->byteArray_[i];
      }
      
      return isSame;
    }
    
    bool Marker::Code::operator<(const Code& otherCode) const
    {
      bool isLess = otherCode.byteArray_[0] < this->byteArray_[0];
      for(u8 i=0; isLess && i<NUM_BYTES; ++i) {
        isLess = otherCode.byteArray_[i] < this->byteArray_[i];
      }
      
      return isLess;
    }
    

#pragma mark --- Static functions for working with a known marker library ---
  
    /*
    void Marker::DefineKnown(const Code &code, const f32 size_mm,
                             const std::string &name)
    {
      library_[code] = {.size = size_mm, .name = name};
    }
     */
    
    /*
    bool Marker::IsKnown(const Code &code)
    {
      return library_.count(code) > 0;
    }
     */
    
    
  } // namespace Vision
} // namespace Anki