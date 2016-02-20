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

// Forward declaration:
namespace Json {
  class Value;
}

namespace Anki {
namespace Vision {
  
  class Image;
  
  class FaceTracker
  {
  public:
    
    FaceTracker(const std::string& modelPath, const Json::Value& config);
    
    ~FaceTracker();
    
    using UpdatedID = struct { TrackedFace::ID_t oldID, newID; };
    
    // Returns the faces found and any IDs that may have been updated (e.g. due
    // to a new recognition or a merge of existing records)
    Result Update(const Vision::Image&    frameOrig,
                  std::list<TrackedFace>& faces,
                  std::list<UpdatedID>&   updatedIDs);
    
    void EnableDisplay(bool enabled);
    
    // Set the number of new users to enroll.
    // If numToEnroll == 0, new face enrollment is disabled.
    // If numToEnroll == -1, all new faces are enrolled ongoing.
    void EnableNewFaceEnrollment(s32 numToEnroll);
    bool IsNewFaceEnrollmentEnabled() const;
    
    // Will return false if the private implementation does not support face recognition
    static bool IsRecognitionSupported();
    
    Result SaveAlbum(const std::string& albumName);
    Result LoadAlbum(const std::string& albumName);
    
    void PrintTiming();
    
  private:
    
    // Forward declaration
    class Impl;
    
    std::unique_ptr<Impl> _pImpl;
    
  }; // class FaceTracker
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_FaceTracker_H__