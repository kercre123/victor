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

#include "anki/vision/basestation/faceTracker.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/trackedFace.h"

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
#else 
#  error Unknown FACE_TRACKER_PROVIDER set!
#endif

namespace Anki {
namespace Vision {
  
  FaceTracker::FaceTracker(const std::string& modelPath, DetectionMode mode)
  : _pImpl(new Impl(modelPath, mode))
  {
    
  }
  
  FaceTracker::~FaceTracker()
  {

  }
  
  Result FaceTracker::Update(const Vision::Image &frameOrig)
  {
    return _pImpl->Update(frameOrig);
  }
 
  std::list<TrackedFace> FaceTracker::GetFaces() const
  {
    return _pImpl->GetFaces();
  }
  
  bool FaceTracker::IsRecognitionSupported()
  {
    return Impl::IsRecognitionSupported();
  }
  
  void FaceTracker::EnableNewFaceEnrollment(bool enable)
  {
    _pImpl->EnableNewFaceEnrollment(enable);
  }
  
  bool FaceTracker::IsNewFaceEnrollmentEnabled() const
  {
    return _pImpl->IsNewFaceEnrollmentEnabled();
  }
  
  void FaceTracker::AssignNameToID(TrackedFace::ID_t faceID, const std::string &name)
  {
    _pImpl->AssignNameToID(faceID, name);
  }
  
  Result FaceTracker::SaveAlbum(const std::string& albumName)
  {
    return _pImpl->SaveAlbum(albumName);
  }
  
  Result FaceTracker::LoadAlbum(const std::string& albumName)
  {
    return _pImpl->LoadAlbum(albumName);
  }
  
  void FaceTracker::PrintTiming()
  {
    _pImpl->Print();
  }
  
  /*
  void FaceTracker::EnableDisplay(bool enabled) {
    _pImpl->EnableDisplay(enabled);
  }
   */
  
} // namespace Vision
} // namespace Anki
