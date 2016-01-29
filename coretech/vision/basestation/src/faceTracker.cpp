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

#include <list>

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
    auto faces = _pImpl->GetFaces();
    if(!_names.empty())
    {
      for(auto & face : faces) {
        auto faceID = face.GetID();
        if(TrackedFace::UnknownFace != faceID)
        {
          auto nameIter = _names.find(faceID);
          if(nameIter != _names.end()) {
            face.SetName(std::string(nameIter->second));
          }
        } else {
          face.SetName("Unknown");
        }
      }
    }
    
    return faces;
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
  
  void FaceTracker::AssignNametoID(TrackedFace::ID_t faceID, const std::string& name)
  {
    _names[faceID] = name;
  }
  
  Result FaceTracker::SaveAlbum(const std::string& filename)
  {
    std::vector<u8> serializedAlbum = _pImpl->GetSerializedAlbum();
    if(serializedAlbum.empty()) {
      PRINT_NAMED_ERROR("FaceTracker.SaveAlbum.EmptyAlbum",
                        "No serialized data returned from private implementation");
      return RESULT_FAIL;
    }
    
    FILE* file = fopen(filename.c_str(), "wb");
    if(nullptr == file) {
      PRINT_NAMED_ERROR("FaceTracker.SaveAlbum.FileOpenFail", "Filename: %s", filename.c_str());
      return RESULT_FAIL;
    }
    
    size_t bytesWritten = fwrite(&(serializedAlbum[0]), serializedAlbum.size(), sizeof(u8), file);
    fclose(file);
    
    if(bytesWritten != serializedAlbum.size()) {
      PRINT_NAMED_ERROR("FaceTracker.SaveAlbum.FileWriteFail",
                        "%lu bytes written instead of expected %lu",
                        bytesWritten, serializedAlbum.size());
      
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  }
  
  Result FaceTracker::LoadAlbum(const std::string& filename)
  {
    std::vector<u8> serializedAlbum;
    
    FILE* file = fopen(filename.c_str(), "rb");
    if(nullptr == file) {
      PRINT_NAMED_ERROR("FaceTracker.LoadAlbum.FileOpenFail", "Filename: %s", filename.c_str());
      return RESULT_FAIL;
    }
    
    fseek(file, 0, SEEK_END);
    size_t fileLength = ftell(file);
    rewind(file);
  
    serializedAlbum.resize(fileLength);
    size_t bytesRead = fread(&(serializedAlbum[0]), fileLength, sizeof(u8), file);
    
    if(bytesRead != fileLength) {
      PRINT_NAMED_ERROR("FaceTracker.LoadAlbum.FileReadFail",
                        "%lu bytes read instead of expected %lu",
                        bytesRead, fileLength);
      
      return RESULT_FAIL;
    }
    
    Result result = _pImpl->SetSerializedAlbum(serializedAlbum);
    
    return result;
  }
  
  
  /*
  void FaceTracker::EnableDisplay(bool enabled) {
    _pImpl->EnableDisplay(enabled);
  }
   */
  
} // namespace Vision
} // namespace Anki
