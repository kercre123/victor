/**
 * File: faceTrackerImpl_okao.h
 *
 * Author: Andrew Stein
 * Date:   11/30/2015
 *
 * Description: Wrapper for OKAO Vision face detection library.
 *
 * NOTE: This file should only be included by faceTracker.cpp
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/eyeContact.h"
#include "coretech/vision/engine/trackedFace.h"
#include "coretech/vision/engine/profiler.h"

#include "clad/types/loadedKnownFace.h"

#include "CommonDef.h"
#include "DetectorComDef.h"

#include "json/json.h"

#include <list>
#include <ctime>

namespace Anki {
  
namespace Util {
  class RandomGenerator;
}
  
namespace Vision {

class FaceTracker::Impl : public Profiler
{
public:
  Impl(const Camera&        camera,
       const std::string&   modelPath,
       const Json::Value&   config);
  ~Impl();

  Result Update(const Vision::Image&        frameOrig,
                std::list<TrackedFace>&     faces,
                std::list<UpdatedFaceID>&   updatedIDs);

  void EnableEmotionDetection(bool enable) {}
  void EnableSmileDetection(bool enable)   {}
  void EnableGazeDetection(bool enable)    {}
  void EnableBlinkDetection(bool enable)   {}

  void AddAllowedTrackedFace(const FaceID_t faceID);
  void SetRecognitionIsSynchronous(bool isSynchronous);

  static bool IsRecognitionSupported();
  static float GetMinEyeDistanceForEnrollment();

  void ClearAllowedTrackedFaces();
  bool HaveAllowedTrackedFaces();
  bool CanAddNamedFace() const;
  Result AssignNameToID(FaceID_t faceID, const std::string &name, FaceID_t mergeWithID);
  Result EraseFace(FaceID_t faceID);
  void EraseAllFaces();
  std::vector<Vision::LoadedKnownFace> GetEnrolledNames() const;
  Result LoadAlbum(const std::string& albumName, std::list<LoadedKnownFace>& loadedFaces);
  Result RenameFace(FaceID_t faceID, const std::string& oldName, const std::string& newName,
                    Vision::RobotRenamedEnrolledFace& renamedFace);
  Result SaveAlbum(const std::string& albumName);
  void SetFaceEnrollmentMode(Vision::FaceEnrollmentPose pose,
                             Vision::FaceID_t forFaceID,
                             s32 numEnrollments);
  Result GetSerializedData(std::vector<u8>& albumData,
                           std::vector<u8>& enrollData);
  Result SetSerializedData(const std::vector<u8>& albumData,
                           const std::vector<u8>& enrollData,
                           std::list<LoadedKnownFace>& loadedFaces);
  void PrintTiming();
  Result SetFacePoseWithoutParts(const s32 nrows, const s32 ncols, TrackedFace& face, f32& intraEyeDist);

private:
    const Camera& _camera;
    bool _startedEnrolling = false;
    s32 _numberOfEnrollments = 0;
    s32 _totalNumberOfCalls = 0;
    s32 _numberOfEnrollmentFrames = 0;
    s32 _enrollmentComplete = 0;
};

}
}
