/**
 * File: faceRecognizer_okao.h
 *
 * Author: Andrew Stein
 * Date:   2/4/2016
 *
 * Description: Wrapper for OKAO Vision face recognition library, which runs on a thread.
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
    
    // Request that the recognizer work on assigning a new or existing FaceID
    // from its album of known faces to the specified trackerID, using the
    // given facial part data. Returns true if the request is accepted (i.e.,
    // the recognizer isn't already busy with another request) and false
    // otherwise. If true, the caller must not modify the part detection handle
    // while processing is running (i.e. until false is returned).
    // If running synchronously, always returns true.
    bool SetNextFaceToRecognize(const Image& img,
                                const DETECTION_INFO& detectionInfo,
                                HPTRESULT okaoPartDetectionResultHandle);
    
    void RemoveTrackingID(INT32 trackerID);
    
    EnrolledFaceEntry GetRecognitionData(INT32 forTrackingID);
    
    Result LoadAlbum(HCOMMON okaoCommonHandle, const std::string& albumName);
    Result SaveAlbum(const std::string& albumName);

    // See FaceTracker for description
    void EnableNewFaceEnrollment(s32 numToEnroll);
    bool IsNewFaceEnrollmentEnabled() const   { return _numToEnroll != 0; }

  private:
    
    // Called by Run() in async mode whenever there's a new image to be processed.
    // Called on each use of SetNextFaceToRecognize() when running synchronously.
    // Assumes state is HasNewImage at start and goes to FeaturesReady on success
    // or Idle on failure.
    void ExtractFeatures();
    
    Result RegisterNewUser(HFEATURE& hFeature);
    
    bool   IsEnrollable();
    
    Result UpdateExistingUser(INT32 userID, HFEATURE& hFeature);
    
    Result RecognizeFace(TrackedFace::ID_t& faceID, INT32& recognitionScore);
    
    Result MergeFaces(TrackedFace::ID_t keepID, TrackedFace::ID_t mergeID);
    
    Result RemoveUser(INT32 userID);

    std::vector<u8> GetSerializedAlbum();
    
    Result SetSerializedAlbum(HCOMMON okaoCommonHandle, const std::vector<u8>&serializedAlbum);

    // These cannot be changed while running (requires re-init of Okao album)
    static const INT32 MaxAlbumDataPerFace = 10; // can't be more than 10
    static const INT32 MaxAlbumFaces = 100; // can't be more than 1000
    
    static_assert(MaxAlbumFaces <= 1000 && MaxAlbumFaces > 1,
                  "MaxAlbumFaces should be between 1 and 1000 for OKAO Library.");
    static_assert(MaxAlbumDataPerFace > 1 && MaxAlbumDataPerFace <= 10,
                  "MaxAlbumDataPerFace should be between 1 and 10 for OKAO Library.");

    //
    // Parameters (initialized from Json config)
    //
    
    INT32 _recognitionThreshold = 750;
    INT32 _mergeThreshold       = 500;
    
    // Time between adding enrollment data for an existing user. "Initial" version
    // is used until all the slots are filled.
    // TODO: Expose as parameters
    f32   _timeBetweenEnrollmentUpdates_sec = 60.f;
    f32   _timeBetweenInitialEnrollmentUpdates_sec = 0.5f;
    
    bool _isInitialized = false;
    
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
    std::mutex  _mutex;
    std::thread _recognitionThread;
    bool        _recognitionRunning = false;
    bool        _isRunningAsync = true;
    ProcessingState _state = ProcessingState::Idle;
    void Run();
    
    // Passed-in state for processing
    Image          _img;
    HPTRESULT      _okaoPartDetectionResultHandle = NULL;
    DETECTION_INFO _detectionInfo;
    
    // Internal bookkeeping and parameters
    std::map<INT32, TrackedFace::ID_t> _trackingToFaceID;
    
    INT32 _lastRegisteredUserID = 1; // Don't start at zero: that's the UnknownFace ID!
    s32   _numToEnroll = 0;
    
    // Store additinal bookkeeping information we need, on top of the album data
    // stored by Okao.
    std::map<TrackedFace::ID_t,EnrolledFaceEntry> _enrollmentData;
    
    
  }; // class FaceRecognizer
  

} // namespace Vision
} // namespace Anki

