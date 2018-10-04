/**
 * File: faceTrackerImpl_test.h
 *
 * Author: Robert Cosgriff
 * Date:   9/3/2018
 *
 * Description: Test harness for face detection and recognition. Specifically
 *              for performance testing with behavior enroll face.
 *
 * NOTE: This file should only be included by faceTracker.cpp
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/profiler.h"

#include "CommonDef.h"

#include "json/json.h"

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
