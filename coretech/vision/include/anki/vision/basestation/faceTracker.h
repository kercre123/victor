/**
 * File: faceTracker.h
 *
 * Author: Andrew Stein
 * Date:   8/18/2015
 *
 * Description: Wrapper for underlying face detection and tracking mechanism, 
 *              the details of which are hidden inside a private implementation.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Anki_Vision_FaceTracker_H__
#define __Anki_Vision_FaceTracker_H__

#include "anki/common/types.h"
#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/rect.h"

#include <opencv2/core/core.hpp>

namespace Anki {  
namespace Vision {
  
  class TrackedFace
  {
  public:
    
    class Impl;
    
    // Constructor:
    TrackedFace();
    TrackedFace(const Impl& impl);

    // Copy / move:
    TrackedFace(const TrackedFace&);
    TrackedFace(TrackedFace&&);

    // Assignment:
    TrackedFace& operator=(const TrackedFace&);
    TrackedFace& operator=(TrackedFace&&);

    ~TrackedFace();
    
    float GetScore() const;
    int   GetID()    const;
    
    TimeStamp_t GetTimeStamp() const;
    
    // Returns true if tracking is happening vs. false if face was just detected
    bool  IsBeingTracked() const;
    
    const Rectangle<f32>& GetRect() const;
    
    // LandmarkPolygons is a vector of polygons, each of which is a vector of
    // 2D points
    using LandmarkPolygons = std::vector<std::vector<Point2f> >;
    LandmarkPolygons GetLandmarkPolygons() const;
    
  private:
    
    std::unique_ptr<Impl> _pImpl;

  }; // class TrackedFace
  
  
  
  class Image;
  
  class FaceTracker
  {
  public:
    
    FaceTracker(const std::string& modelPath);
    ~FaceTracker();
    
    Result Update(const Vision::Image& frameOrig);
    
    std::vector<TrackedFace> GetFaces() const;
    
    void EnableDisplay(bool enabled);
    
  private:
    
    // Forward declaration
    class Impl;
    
    std::unique_ptr<Impl> _pImpl;
    
  }; // class FaceTracker
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_FaceTracker_H__