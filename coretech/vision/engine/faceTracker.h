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

#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/trackedFace.h"
#include "coretech/vision/engine/faceIdTypes.h"
#include "clad/types/faceEnrollmentPoses.h"
#include "clad/types/loadedKnownFace.h"

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
    
    FaceTracker(const Camera& camera,
                const std::string& modelPath,
                const Json::Value& config);
    
    ~FaceTracker();
    
    // Returns the faces found and any IDs that may have been updated (e.g. due
    // to a new recognition or a merge of existing records).
    Result Update(const Vision::Image&        frameOrig,
                  std::list<TrackedFace>&     faces,
                  std::list<UpdatedFaceID>&   updatedIDs);
    
    // Clear currently-tracked faces, e.g. in case camera has moved and could cause trouble
    void Reset();
    
    void EnableDisplay(bool enabled);
    
    void SetRecognitionIsSynchronous(bool isSynchronous);
    
    void SetFaceEnrollmentMode(FaceEnrollmentPose pose,
                               FaceID_t forFaceID = UnknownFaceID,
                               s32 numEnrollments = -1);
    
    void EnableEmotionDetection(bool enable);
    void EnableSmileDetection(bool enable);
    void EnableGazeDetection(bool enable);
    void EnableBlinkDetection(bool enable);
    
    // Will return false if the private implementation does not support face recognition
    static bool IsRecognitionSupported();
    
    // returns the minimum distance between eyes a face has to have in order to be enrollable
    static float GetMinEyeDistanceForEnrollment();
    
    Result AssignNameToID(FaceID_t faceID, const std::string& name, FaceID_t mergeWithID);
    
    Result EraseFace(FaceID_t faceID);
    void   EraseAllFaces();
    Result RenameFace(FaceID_t faceID, const std::string& oldName, const std::string& newName,
                      Vision::RobotRenamedEnrolledFace& renamedFace);
    
    std::vector<Vision::LoadedKnownFace> GetEnrolledNames() const;
    
    Result SaveAlbum(const std::string& albumName);
    Result LoadAlbum(const std::string& albumName, std::list<LoadedKnownFace>& loadedFaces);
    
    Result GetSerializedData(std::vector<u8>& albumData,
                             std::vector<u8>& enrollData);
    
    Result SetSerializedData(const std::vector<u8>& albumData,
                             const std::vector<u8>& enrollData,
                             std::list<LoadedKnownFace>& loadedFaces);

    void PrintTiming();
    
  private:
    
    // Forward declaration
    class Impl;
    
    std::unique_ptr<Impl> _pImpl;
    
  }; // class FaceTracker
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_FaceTracker_H__
