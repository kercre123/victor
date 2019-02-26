/**
 * File: faceTracker.cpp
 *
 * Author: Andrew Stein
 * Date:   8/18/2015
 *
 * Description: Implements the wrappers for the private implementation of
 *              FaceTracker::Impl. The various implementations of Impl are 
 *              in separate faceTrackerImpl_<PROVIDER>.h files, which are 
 *              included here based on the defined FACE_TRACKER_PROVIDER.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/trackedFace.h"

#include "util/fileUtils/fileUtils.h"

#include <list>
#include <fstream>

#if FACE_TRACKER_PROVIDER == FACE_TRACKER_FACIOMETRIC || \
FACE_TRACKER_PROVIDER == FACE_TRACKER_OPENCV
  // Parameters used by cv::detectMultiScale for both of these face trackers
  static const f32 opencvDetectScaleFactor = 1.3f;
  static const cv::Size opencvDetectMinFaceSize(48,48);
#endif

#if FACE_TRACKER_PROVIDER == FACE_TRACKER_FACIOMETRIC
#  include "faceTrackerImpl_faciometric.h"
#elif FACE_TRACKER_PROVIDER == FACE_TRACKER_FACESDK
#  include "faceTrackerImpl_facesdk.h"
#elif FACE_TRACKER_PROVIDER == FACE_TRACKER_OKAO
#  include "faceTrackerImpl_okao.h"
#elif FACE_TRACKER_PROVIDER == FACE_TRACKER_OPENCV
#  include "faceTrackerImpl_opencv.h"
#elif FACE_TRACKER_PROVIDER == FACE_TRACKER_TEST
#  include "faceTrackerImpl_test.h"
#else 
#  error Unknown FACE_TRACKER_PROVIDER set!
#endif

namespace Anki {
namespace Vision {
  
  FaceTracker::FaceTracker(const Camera&        camera,
                           const std::string&   modelPath,
                           const Json::Value&   config)
  : _pImpl(new Impl(camera, modelPath, config))
  {
    
  }
  
  FaceTracker::~FaceTracker()
  {

  }
  
  Result FaceTracker::Update(const Vision::Image&        frameOrig,
                             const float                 cropFactor,
                             std::list<TrackedFace>&     faces,
                             std::list<UpdatedFaceID>&   updatedIDs,
                             DebugImageList<CompressedImage>& debugImages)
  {
    return _pImpl->Update(frameOrig, cropFactor, faces, updatedIDs, debugImages);
  }
  
  void FaceTracker::AddAllowedTrackedFace(const FaceID_t faceID)
  {
    _pImpl->AddAllowedTrackedFace(faceID);
  }
  
  void FaceTracker::SetRecognitionIsSynchronous(bool isSynchronous)
  {
    _pImpl->SetRecognitionIsSynchronous(isSynchronous);
  }

  bool FaceTracker::HaveAllowedTrackedFaces()
  {
    return _pImpl->HaveAllowedTrackedFaces();
  }

  void FaceTracker::ClearAllowedTrackedFaces()
  {
    _pImpl->ClearAllowedTrackedFaces();
  }

  void FaceTracker::AccountForRobotMove()
  {
    if (!_pImpl->HaveAllowedTrackedFaces())
    {
      _pImpl->ClearAllowedTrackedFaces();
    }
  }
  
  bool FaceTracker::IsRecognitionSupported()
  {
    return Impl::IsRecognitionSupported();
  }
  
  void FaceTracker::EnableRecognition(bool enable)
  {
    _pImpl->EnableRecognition(enable);
  }
  
  bool FaceTracker::IsRecognitionEnabled() const
  {
    return _pImpl->IsRecognitionEnabled();
  }
  
  float FaceTracker::GetMinEyeDistanceForEnrollment()
  {
    return Impl::GetMinEyeDistanceForEnrollment();
  }
  
  bool FaceTracker::CanAddNamedFace() const
  {
    return _pImpl->CanAddNamedFace();
  }
  
  Result FaceTracker::AssignNameToID(FaceID_t faceID, const std::string &name, FaceID_t mergeWithID)
  {
    return _pImpl->AssignNameToID(faceID, name, mergeWithID);
  }

  Result FaceTracker::EraseFace(FaceID_t faceID)
  {
    return _pImpl->EraseFace(faceID);
  }
  
  void FaceTracker::EraseAllFaces()
  {
    return _pImpl->EraseAllFaces();
  }
  
  std::vector<Vision::LoadedKnownFace> FaceTracker::GetEnrolledNames() const
  {
    return _pImpl->GetEnrolledNames();
  }
  
  Result FaceTracker::RenameFace(FaceID_t faceID, const std::string& oldName, const std::string& newName,
                                 Vision::RobotRenamedEnrolledFace& renamedFace)
  {
    return _pImpl->RenameFace(faceID, oldName, newName, renamedFace);
  }
  
  Result FaceTracker::SaveAlbum(const std::string& albumName)
  {
    return _pImpl->SaveAlbum(albumName);
  }
  
  Result FaceTracker::LoadAlbum(const std::string& albumName, std::list<LoadedKnownFace>& loadedFaces)
  {
    return _pImpl->LoadAlbum(albumName, loadedFaces);
  }
  
  void FaceTracker::PrintTiming()
  {
    _pImpl->PrintAverageTiming();
  }
  
  void FaceTracker::SetFaceEnrollmentMode(FaceID_t forFaceID, s32 numEnrollments, bool forceNewID)
  {
    _pImpl->SetFaceEnrollmentMode(forFaceID, numEnrollments, forceNewID);
  }
  
  void FaceTracker::EnableEmotionDetection(bool enable)
  {
    _pImpl->EnableEmotionDetection(enable);
  }
  
  void FaceTracker::EnableSmileDetection(bool enable)
  {
    _pImpl->EnableSmileDetection(enable);
  }
  
  void FaceTracker::EnableGazeDetection(bool enable)
  {
    _pImpl->EnableGazeDetection(enable);
  }
  
  void FaceTracker::EnableBlinkDetection(bool enable)
  {
    _pImpl->EnableBlinkDetection(enable);
  }
  
  Result FaceTracker::GetSerializedData(std::vector<u8>& albumData,
                                        std::vector<u8>& enrollData)
  {
    return _pImpl->GetSerializedData(albumData, enrollData);
  }
  
  Result FaceTracker::SetSerializedData(const std::vector<u8>& albumData,
                                        const std::vector<u8>& enrollData,
                                        std::list<LoadedKnownFace>& loadedFaces)
  {
    return _pImpl->SetSerializedData(albumData, enrollData, loadedFaces);
  }
  
#if ANKI_DEVELOPER_CODE
  Result FaceTracker::DevAddFaceToAlbum(const Image& img, const TrackedFace& face, int albumEntry)
  {
    return _pImpl->DevAddFaceToAlbum(img, face, albumEntry);
  }
  
  Result FaceTracker::DevFindFaceInAlbum(const Image& img, const TrackedFace& face,
                                         int& albumEntry, float& score) const
  {
    return _pImpl->DevFindFaceInAlbum(img, face, albumEntry, score);
  }
  
  Result FaceTracker::DevFindFaceInAlbum(const Image& img, const TrackedFace& face, const int maxMatches,
                                         std::vector<std::pair<int, float>>& matches) const
  {
    return _pImpl->DevFindFaceInAlbum(img, face, maxMatches, matches);
  }
  
  float FaceTracker::DevComputePairwiseMatchScore(FaceID_t faceID1, FaceID_t faceID2) const
  {
    return _pImpl->DevComputePairwiseMatchScore(faceID1, faceID2);
  }
  
  float FaceTracker::DevComputePairwiseMatchScore(int faceID1, const Image& img2, const TrackedFace& face2) const
  {
    return _pImpl->DevComputePairwiseMatchScore(faceID1, img2, face2);
  }
#endif /* ANKI_DEVELOPER_CODE */

#if ANKI_DEV_CHEATS
  void FaceTracker::SaveAllRecognitionImages(const std::string& imagePathPrefix)
  {
    _pImpl->SaveAllRecognitionImages(imagePathPrefix);
  }

  void FaceTracker::DeleteAllRecognitionImages()
  {
    _pImpl->DeleteAllRecognitionImages();
  }
#endif // ANKI_DEV_CHEATS
  
  /*
  void FaceTracker::EnableDisplay(bool enabled) {
    _pImpl->EnableDisplay(enabled);
  }
   */
  
} // namespace Vision
} // namespace Anki
