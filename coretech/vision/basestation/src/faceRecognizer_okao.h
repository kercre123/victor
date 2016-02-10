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

// Omron OKAO Vision
#include <OkaoAPI.h>
#include <OkaoPtAPI.h> // Face parts detection
#include <OkaoFrAPI.h> // Face Recognition
#include <CommonDef.h>
#include <DetectorComDef.h>

#include <list>
#include <ctime>
#include <thread>
#include <mutex>

namespace Anki {
namespace Vision {

  class FaceRecognizer
  {
  public:
    FaceRecognizer() { }
    
    ~FaceRecognizer();
    
    Result Init(HCOMMON okaoCommonHandle);

    void AssignNameToID(TrackedFace::ID_t faceID, const std::string& name);
    
    // Request that the recognizer work on assigning a new or existing FaceID
    // from its album of known faces to the specified trackerID, using the
    // given facial part data. Returns true if the request is accepted (i.e.,
    // the recognizer isn't already busy with another request) and false
    // otherwise. If true, the caller must not modify the part detection handle
    // while processing is running (i.e. until false is returned).
    bool SetNextFaceToRecognize(const Vision::Image& img,
                                INT32 trackerID,
                                HPTRESULT okaoPartDetectionResultHandle);
    
    void RemoveTrackingID(INT32 trackerID);
    
    struct Entry
    {
      Vision::TrackedFace::ID_t faceID = Vision::TrackedFace::UnknownFace;
      std::string               name;
      INT32                     score  = 0;
    };
    
    Entry GetRecognitionData(INT32 forTrackingID);
    
    Result LoadAlbum(HCOMMON okaoCommonHandle, const std::string& albumName);
    Result SaveAlbum(const std::string& albumName);

    void EnableNewFaceEnrollment(bool enable) { _enrollNewFaces = enable; }
    bool IsNewFaceEnrollmentEnabled() const   { return _enrollNewFaces; }

  private:
    
    Result RegisterNewUser(HFEATURE& hFeature);
    
    Result UpdateExistingUser(INT32 userID, HFEATURE& hFeature);
    
    Result RecognizeFace(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                         Vision::TrackedFace::ID_t& faceID, INT32& recognitionScore);
    
    Result MergeFaces(Vision::TrackedFace::ID_t keepID, Vision::TrackedFace::ID_t mergeID);
    
    Result RemoveUser(INT32 userID);

    std::vector<u8> GetSerializedAlbum();
    
    Result SetSerializedAlbum(HCOMMON okaoCommonHandle, const std::vector<u8>&serializedAlbum);

    static const INT32 MaxAlbumDataPerFace = 10; // can't be more than 10
    static const INT32 MaxAlbumFaces = 100; // can't be more than 1000
    static const s32   TimeBetweenEnrollmentUpdates_ms = 2000;
    
    static_assert(MaxAlbumFaces <= 1000 && MaxAlbumFaces > 1,
                  "MaxAlbumFaces should be between 1 and 1000 for OKAO Library.");
    static_assert(MaxAlbumDataPerFace > 1 && MaxAlbumDataPerFace <= 10,
                  "MaxAlbumDataPerFace should be between 1 and 10 for OKAO Library.");
    
    bool _isInitialized = false;
    
    // Okao handles allocated by this class
    HFEATURE    _okaoRecognitionFeatureHandle  = NULL;
    HFEATURE    _okaoRecogMergeFeatureHandle   = NULL;
    HALBUM      _okaoFaceAlbum                 = NULL;
    
    // Threading
    std::mutex  _mutex;
    std::thread _recognitionThread;
    bool        _recognitionRunning = false;
    void Run();
    
    // Passed-in state for processing
    INT32         _currentTrackerID = -1;
    Vision::Image _img;
    HPTRESULT     _okaoPartDetectionResultHandle = NULL;
    
    // Internal bookkeeping and parameters
    std::map<INT32, Vision::TrackedFace::ID_t> _trackingToFaceID;
    
    std::list<INT32> _trackerIDsToRemove;
    
    INT32 _lastRegisteredUserID = 0;
    bool  _enrollNewFaces = true;
    
    // Store additinal bookkeeping information we need, on top of the album data
    // stored by Okao.
    struct EnrollmentData {
      INT32        oldestData = 0;
      time_t       enrollmentTime;
      std::string  name;
      INT32        lastScore = 0;
    };
    std::map<Vision::TrackedFace::ID_t,EnrollmentData> _enrollmentData;
    
    
  }; // class FaceRecognizer
  

} // namespace Vision
} // namespace Anki

