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
#include "anki/vision/basestation/trackedFace.h"

#include <list>

// Available Face Trackers to select from:
// NOTE: You will now also have to set "face_library" in coretech-internal.gyp
//       AND cozmoGame.gyp correspondingly to get these to work (which also
//       requires you actually have the 3rd party SDKs in coretech-external.
#define FACE_TRACKER_FACIOMETRIC 0
#define FACE_TRACKER_FACESDK     1
#define FACE_TRACKER_OPENCV      2

#define FACE_TRACKER_PROVIDER FACE_TRACKER_OPENCV

namespace Anki {
namespace Vision {
  
  class Image;
  
  class FaceTracker
  {
  public:
    
    FaceTracker(const std::string& modelPath);
    ~FaceTracker();
    
    Result Update(const Vision::Image& frameOrig);
    
    std::list<TrackedFace> GetFaces() const;
    
    void EnableDisplay(bool enabled);
    
  private:
    
    // Forward declaration
    class Impl;
    
    std::unique_ptr<Impl> _pImpl;
    
  }; // class FaceTracker
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_FaceTracker_H__