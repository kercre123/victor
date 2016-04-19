/**
 * File: faceRecognizer_okao.h
 *
 * Author: Andrew Stein
 * Date:   2/4/2016
 *
 * Description: Wrapper for OKAO Vision face recognition library. Maintains the 
 *              library of enrolled faces. Supports running facial feature 
 *              extraction on a separate thread by default, since that is the 
 *              slowest part of the recognition process.
 *
 * NOTE: This file should only be included by faceTrackerImpl_okao.h
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/trackedFace.h"
#include "anki/vision/basestation/profiler.h"
#include "anki/vision/basestation/enrolledFaceEntry.h"

// Omron OKAO Vision
#include "OkaoAPI.h"
#include "OkaoPtAPI.h" // Face parts detection
#include "OkaoFrAPI.h" // Face Recognition
#include "CommonDef.h"
#include "DetectorComDef.h"

#include <list>
#include <map>
#include <ctime>
#include <thread>
#include <mutex>

namespace Json {
  class Value;
}

namespace Anki {
namespace Vision {

  class FaceRecognizer : public Profiler
  {
  public:
    
    FaceRecognizer(const Json::Value& config);
    
    ~FaceRecognizer();
    
    Result Init(HCOMMON okaoCommonHandle);

    void AssignNameToID(FaceID_t faceID, const std::string& name);
    void EraseName(const std::string& name);
    
    // Request that the recognizer work on assigning a new or existing FaceID
    // from its album of known faces to the specified trackerID, using the
    // given facial part data. Returns true if the request is accepted (i.e.,
    // the recognizer isn't already busy with another request) and false
    // otherwise. If true, the caller must not modify the part detection handle
    // while processing is running (i.e. until false is returned).
    // If running synchronously, always returns true.
    bool SetNextFaceToRecognize(const Image& img,
                                const DETECTION_INFO& detectionInfo,
                                HPTRESULT okaoPartDetectionResultHandle,
                                bool enableEnrollment);
    
    void RemoveTrackingID(INT32 trackerID);
    
    EnrolledFaceEntry GetRecognitionData(INT32 forTrackingID);
    
    Result LoadAlbum(HCOMMON okaoCommonHandle, const std::string& albumName, std::list<FaceNameAndID>& namesAndIDs);
    Result SaveAlbum(const std::string& albumName);

  private:
    
    // Called by Run() in async mode whenever there's a new image to be processed.
    // Called on each use of SetNextFaceToRecognize() when running synchronously.
    // Assumes state is HasNewImage at start and goes to FeaturesReady on success
    // or Idle on failure.
    void ExtractFeatures();
    
    Result RegisterNewUser(HFEATURE& hFeature);
    
    Result UpdateExistingUser(INT32 userID, HFEATURE& hFeature);
    
    Result RecognizeFace(FaceID_t& faceID, INT32& recognitionScore);
    
    Result MergeFaces(FaceID_t keepID, FaceID_t mergeID);
    
    Result RemoveUser(INT32 userID);

    Result GetSerializedAlbum(std::vector<u8>& serializedAlbum) const;
    
    Result SetSerializedAlbum(HCOMMON okaoCommonHandle, const std::vector<u8>&serializedAlbum);

    // These cannot be changed while running (requires re-init of Okao album)
    static const INT32 MaxAlbumDataPerFace = 10; // can't be more than 10
    static const INT32 MaxAlbumFaces = 100; // can't be more than 1000
    
    static_assert(MaxAlbumFaces <= 1000 && MaxAlbumFaces > 1,
                  "MaxAlbumFaces should be between 1 and 1000 for OKAO Library.");
    static_assert(MaxAlbumDataPerFace > 1 && MaxAlbumDataPerFace <= 10,
                  "MaxAlbumDataPerFace should be between 1 and 10 for OKAO Library.");
    
    bool        _isInitialized                 = false;
    HCOMMON     _okaoCommonHandle              = NULL; // not allocated here, passed in
    
    // Okao handles allocated by this class
    HFEATURE    _okaoRecognitionFeatureHandle  = NULL;
    HFEATURE    _okaoRecogMergeFeatureHandle   = NULL;
    HALBUM      _okaoFaceAlbum                 = NULL;
    
    // Threading
    enum class ProcessingState : u8 {
      Idle,
      HasNewImage,
      ExtractingFeatures,
      FeaturesReady
    };
    std::mutex      _mutex;
    std::thread     _featureExtractionThread;
    bool            _isRunningAsync = true;
    ProcessingState _state = ProcessingState::Idle;
    void Run();
    
    // Passed-in state for processing
    Image          _img;
    HPTRESULT      _okaoPartDetectionResultHandle = NULL;
    DETECTION_INFO _detectionInfo;
    bool           _isEnrollmentEnabled;
    
    // Internal bookkeeping and parameters
    std::map<INT32, FaceID_t> _trackingToFaceID;
    
    INT32 _lastRegisteredUserID = 1; // Don't start at zero: that's the UnknownFace ID!
    
    // Store additinal bookkeeping information we need, on top of the album data
    // stored by Okao.
    std::map<FaceID_t,EnrolledFaceEntry> _enrollmentData;
    
    // For debugging what is in current enrollment images
    std::map<FaceID_t,std::array<Vision::ImageRGB, MaxAlbumDataPerFace>> _enrollmentImages;
    void SetEnrollmentImage(FaceID_t userID, s32 whichEnrollData, const EnrolledFaceEntry& enrollData);
    
  }; // class FaceRecognizer
  

} // namespace Vision
} // namespace Anki

