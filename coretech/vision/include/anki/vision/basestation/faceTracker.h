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
  
  class TrackedFaceImpl;
  
  class TrackedFace
  {
  public:
    
    TrackedFace();
    TrackedFace(TrackedFaceImpl* impl);
    
    ~TrackedFace();
    
    float GetScore() const;
    int   GetID()    const;
    
    TimeStamp_t GetTimeStamp() const;
    
    // Returns true if tracking is happening vs. false if face was just detected
    bool  IsBeingTracked() const;
    
    const Rectangle<f32>& GetRect() const;
    
    using LandmarkPolygons = std::vector<std::pair<std::vector<Point2f>,bool> >;
    LandmarkPolygons GetLandmarkPolygons() const;
    
  private:
    
    TrackedFaceImpl* _pImpl;
    bool _ownsImpl;

  }; // class TrackedFace
  
  
  class Image;
  class FaceTrackerImpl;
  
  class FaceTracker
  {
  public:
    
    FaceTracker(const std::string& modelPath);
    ~FaceTracker();
    
    Result Update(const Vision::Image& frameOrig);
    
    void GetFaces(std::vector<TrackedFace>& faces);
    
    void EnableDisplay(bool enabled);
    
  private:
    
    FaceTrackerImpl* _pImpl;
    
  }; // class FaceTracker
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_FaceTracker_H__