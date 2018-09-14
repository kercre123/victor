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

#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/trackedFace.h"
#include "coretech/vision/engine/profiler.h"
#include "coretech/vision/engine/enrolledFaceEntry.h"

#include "clad/types/loadedKnownFace.h"

// Omron OKAO Vision
#include "OkaoAPI.h"
#include "OkaoPtAPI.h" // Face parts detection
#include "OkaoFrAPI.h" // Face Recognition
#include "CommonDef.h"
#include "DetectorComDef.h"

#include "util/math/numericCast.h"

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
    using TrackingID_t = EnrolledFaceEntry::TrackingID_t;
    
    FaceRecognizer(const Json::Value& config);
    
    ~FaceRecognizer();
    
    Result Init(HCOMMON okaoCommonHandle);
    Result Shutdown();

    void SetIsSynchronous(bool isSynchronous);
    
    bool     CanAddNamedFace() const;
    Result   AssignNameToID(FaceID_t faceID, const std::string& name, FaceID_t mergeWithID);
    Result   EraseFace(FaceID_t faceID);
    void     EraseAllFaces();
    
    std::vector<Vision::LoadedKnownFace> GetEnrolledNames() const;
    
    Result RenameFace(FaceID_t faceID, const std::string& oldName, const std::string& newName,
                      Vision::RobotRenamedEnrolledFace& renamedFace);
    
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
    
    TrackingID_t GetEnrollmentTrackID() const { return _enrollmentTrackID; }
    FaceID_t     GetEnrollmentID()      const { return _enrollmentID; }
    
    void RemoveTrackingID(TrackingID_t trackerID);
    void ClearAllTrackingData();
    
    // Return existing or newly-computed recognitino info for a given tracking ID.
    // If a specific enrollment ID and count are in use, and the enrollment just
    // completed (the count was just reached), then that count is returned in
    // 'enrollmentCountReached'. Otherwise 0 is returned.
    EnrolledFaceEntry GetRecognitionData(TrackingID_t forTrackingID, s32& enrollmentCountReached);
    
    bool HasRecognitionData(TrackingID_t forTrackingID) const;
    
    Result LoadAlbum(const std::string& albumName, std::list<LoadedKnownFace>& loadedFaces);
    Result SaveAlbum(const std::string& albumName);
    
    Result GetSerializedData(std::vector<u8>& albumData,
                             std::vector<u8>& enrollData);
    
    // Populates the list of names and IDs recovered from the serialized data on success
    Result SetSerializedData(const std::vector<u8>& albumData,
                             const std::vector<u8>& enrollData,
                             std::list<LoadedKnownFace>& loadedFaces);

    bool GetFaceIDFromTrackingID(const TrackingID_t trackingID, FaceID_t& faceID);

  private:
    
    // Aliases for better readability
    using OkaoResult       = INT32; // NOTE: "INT32" is an OKAO library type
    using AlbumEntryID_t   = EnrolledFaceEntry::AlbumEntryID_t;
    using RecognitionScore = EnrolledFaceEntry::RecognitionScore;
    using EnrollmentData   = std::map<FaceID_t,EnrolledFaceEntry>;
    
    using AlbumEntryToFaceID = std::map<AlbumEntryID_t, FaceID_t>;
    
    // Called by Run() in async mode whenever there's a new image to be processed.
    // Called on each use of SetNextFaceToRecognize() when running synchronously.
    // Assumes state is HasNewImage at start and goes to FeaturesReady on success
    // or Idle on failure.
    void ExtractFeatures();
    
    AlbumEntryID_t GetNextAlbumEntryToUse();
    FaceID_t GetNextFaceID();
    
    Result RegisterNewUser(HFEATURE& hFeature, FaceID_t& faceID);
    
    Result UpdateExistingAlbumEntry(AlbumEntryID_t albumEntry, HFEATURE& hFeature, RecognitionScore score);
    
    // Matches features to known faces, when features are done being computed
    Result RecognizeFace(FaceID_t& faceID, RecognitionScore& recognitionScore);
    
    // Uses the ID and score from RecognizeFace to update the data. Also checks
    // for merge opportunities.
    Result UpdateRecognitionData(const FaceID_t recognizedID,
                                 const RecognitionScore score);
    
		bool   IsMergingAllowed(FaceID_t toFaceID) const;
    
    s32    GetNumNamedFaces() const;
    
    Result MergeFaces(FaceID_t keepID, FaceID_t mergeID);
  
    Result SelectiveMergeHelper(EnrollmentData::iterator keepIter, EnrollmentData::iterator mergeIter);
    
    Result RemoveUser(FaceID_t userID);
    EnrollmentData::iterator RemoveUser(EnrollmentData::iterator userIter);

    Result GetSerializedAlbum(std::vector<u8>& serializedAlbum) const;
    
    static Result SetSerializedAlbum(HCOMMON okaoCommonHandle, const std::vector<u8>&serializedAlbum, HALBUM& album);
    
    Result GetSerializedEnrollData(std::vector<u8>& serializedEnrollData);
    
    static Result SetSerializedEnrollData(const std::vector<u8>& serializedEnrollData,
                                          EnrollmentData& newEnrollmentData,
                                          FaceID_t& newNextFaceID);
    
    static void CreateAlbumEntryToFaceLUT(const EnrollmentData& enrollmentData,
                                          AlbumEntryToFaceID& albumEntryToFaceID);
  
    // Makes sure the given album and enrollment data look consistent
    static Result SanityCheckBookkeeping(const HALBUM& okaoFaceAlbum,
                                         const EnrollmentData& enrollmentData,
                                         const AlbumEntryToFaceID& albumEntryToFaceID);

    Result UseLoadedAlbumAndEnrollData(HALBUM& loadedAlbumData,
                                       EnrollmentData& loadedEnrollmentData);
    
    // Returns UnknownFaceID on failure
    FaceID_t GetFaceIDforAlbumEntry(AlbumEntryID_t albumEntry) const;
    
    void AddDebugInfo(FaceID_t matchedID, RecognitionScore score,
                      std::list<FaceRecognitionMatch>& newDebugInfo) const;
    
    // Cancel existing and remove partial album entries
    void CancelExistingEnrollment();
    
    // TODO: Using Util::numeric_cast here would be nice, but its not (always) constexpr...
    static constexpr s32 kMaxNamedFacesInAlbum       = (s32)FaceRecognitionConstants::MaxNumFacesInAlbum;
    static constexpr s32 kMaxAlbumEntriesPerFace     = (s32)FaceRecognitionConstants::MaxNumAlbumEntriesPerFace;
    static constexpr s32 kMaxEnrollDataPerAlbumEntry = (s32)FaceRecognitionConstants::MaxNumEnrollDataPerAlbumEntry;
    
    // Make sure we are within fixed limits of the OKAO library
    static constexpr s32 kMaxTotalAlbumEntries = 1000;
    static constexpr s32 kMinSessionOnlyFaces = 10;
    static_assert( (kMaxNamedFacesInAlbum+kMinSessionOnlyFaces)*kMaxAlbumEntriesPerFace <= kMaxTotalAlbumEntries,
                  "Combination of min/max face parameters too large for OKAO Library (Max is 1000).");
    static_assert(kMaxEnrollDataPerAlbumEntry <= 10,
                  "MaxEnrollDataPerAlbumEntry too large for OKAO Library (Max is 10).");
    
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
    bool            _isEnrollmentCancelled = false;
    ProcessingState _state = ProcessingState::Idle;
    void StartThread();
    void Run();
    void StopThread();
    
    // Passed-in state for processing
    Image          _img;
    HPTRESULT      _okaoPartDetectionResultHandle = NULL;
    DETECTION_INFO _detectionInfo;
    
    // Internal bookkeeping and parameters
    std::map<TrackingID_t, FaceID_t> _trackingToFaceID;
    AlbumEntryToFaceID   _albumEntryToFaceID;
    
    FaceID_t       _nextFaceID     = 1; // Skip UnknownFaceID
    AlbumEntryID_t _nextAlbumEntry = 0; 

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
    std::map<AlbumEntryID_t,std::array<Vision::ImageRGB, kMaxEnrollDataPerAlbumEntry>> _enrollmentImages;
    void SetEnrollmentImage(AlbumEntryID_t albumEntry, s32 dataEntry);
    void DisplayEnrollmentImages() const;
    void SaveEnrollmentImages() const;
    
  }; // class FaceRecognizer
  

} // namespace Vision
} // namespace Anki

