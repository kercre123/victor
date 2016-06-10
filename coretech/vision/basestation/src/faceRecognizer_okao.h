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

    Result   AssignNameToID(FaceID_t faceID, const std::string& name);
    FaceID_t EraseFace(const std::string& name);
    Result   EraseFace(FaceID_t faceID);
    void     EraseAllFaces();
    
    Result RenameFace(FaceID_t faceID, const std::string& oldName, const std::string& newName);
    
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
    
    // Use faceID = UnknownFaceID to allow enrollments for any face.
    // Use N = -1 to allow ongoing enrollment.
    void SetAllowedEnrollments(s32 N, FaceID_t forFaceID);
    
    void RemoveTrackingID(INT32 trackerID);
    
    // Return existing or newly-computed recognitino info for a given tracking ID.
    // If a specific enrollment ID and count are in use, and the enrollment just
    // completed (the count was just reached), then that count is returned in
    // 'enrollmentCountReached'. Otherwise 0 is returned.
    EnrolledFaceEntry GetRecognitionData(INT32 forTrackingID, s32& enrollmentCountReached);
    
    Result LoadAlbum(const std::string& albumName, std::list<FaceNameAndID>& namesAndIDs);
    Result SaveAlbum(const std::string& albumName);
    
    
    Result GetSerializedData(std::vector<u8>& albumData,
                             std::vector<u8>& enrollData);
    
    // Populates the list of names and IDs recovered from the serialized data on success
    Result SetSerializedData(const std::vector<u8>& albumData,
                             const std::vector<u8>& enrollData,
                             std::list<FaceNameAndID>& namesAndIDs);

  private:
    
    using EnrollmentData = std::map<FaceID_t,EnrolledFaceEntry>;
    
    // Called by Run() in async mode whenever there's a new image to be processed.
    // Called on each use of SetNextFaceToRecognize() when running synchronously.
    // Assumes state is HasNewImage at start and goes to FeaturesReady on success
    // or Idle on failure.
    void ExtractFeatures();
    
    Result RegisterNewUser(HFEATURE& hFeature);
    
    Result UpdateExistingUser(INT32 userID, HFEATURE& hFeature);
    
    Result RecognizeFace(FaceID_t& faceID, INT32& recognitionScore);
    
		bool   IsMergingAllowed(FaceID_t toFaceID) const;
		
    Result MergeFaces(FaceID_t keepID, FaceID_t mergeID);
    
    Result RemoveUser(INT32 userID);
    EnrollmentData::iterator RemoveUser(EnrollmentData::iterator userIter);
    Result RemoveUserHelper(INT32 userID);

    Result GetSerializedAlbum(std::vector<u8>& serializedAlbum) const;
    
    static Result SetSerializedAlbum(HCOMMON okaoCommonHandle, const std::vector<u8>&serializedAlbum, HALBUM& album);
    
    Result GetSerializedEnrollData(std::vector<u8>& serializedEnrollData);
    
    static Result SetSerializedEnrollData(const std::vector<u8>& serializedEnrollData,
                                          EnrollmentData& newEnrollmentData);
    
    // Makes sure the given album and enrollment data look consistent
    static Result SanityCheckBookkeeping(const HALBUM& okaoFaceAlbum,
                                         const EnrollmentData& enrollmentData);

    Result UseLoadedAlbumAndEnrollData(HALBUM& loadedAlbumData,
                                       EnrollmentData& loadedEnrollmentData);
    
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
    
    // Internal bookkeeping and parameters
    std::map<INT32, FaceID_t> _trackingToFaceID;
    
    INT32 _lastRegisteredUserID = 1; // Don't start at zero: that's the UnknownFace ID!
    
    // Which face we are allowed to add enrollment data for (UnknownFaceID == "any" face),
    // and how many enrollments we are allowed to add ( <0 means as many as we want)
    bool      _isEnrollmentEnabled = true;
    FaceID_t  _enrollmentID = UnknownFaceID;
    FaceID_t  _enrollmentTrackID = UnknownFaceID;
    s32       _enrollmentCount = -1; // Has no effect if enrollmentID not set
    s32       _origEnrollmentCount = -1;
    
    // Store additinal bookkeeping information we need, on top of the album data
    // stored by Okao.
    EnrollmentData _enrollmentData;
    
    // For debugging what is in current enrollment images
    std::map<FaceID_t,std::array<Vision::ImageRGB, MaxAlbumDataPerFace>> _enrollmentImages;
    void SetEnrollmentImage(FaceID_t userID, s32 whichEnrollData, const EnrolledFaceEntry& enrollData);
    
  }; // class FaceRecognizer
  

} // namespace Vision
} // namespace Anki

