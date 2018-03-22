/**
 * File: faceRecognizer_okao.cpp
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

#if FACE_TRACKER_PROVIDER == FACE_TRACKER_OKAO

#include "faceRecognizer_okao.h"

#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"
#include "util/console/consoleInterface.h"
#include "util/helpers/boundedWhile.h"
#include "util/helpers/cleanupHelper.h"
#include "util/threading/threadPriority.h"
#include "json/json.h"
#include "coretech/common/engine/jsonTools.h"

#include "OkaoCoAPI.h"

#include <fstream>

#define LOG_CHANNEL "FaceRecognizer"

#define DEBUG_ENROLLMENT_IMAGES 0 // Everything will need to be running synchronously!

namespace Anki {
namespace Vision {

  // Versioning so we can avoid badness if/when we change EnrolledFaceStorage CLAD
  // structure. This prefix will be at the beginning of the serialized enrollment
  // data stored in NVStorage on the robot.
  static const u16 VersionNumber = Util::numeric_cast<u16>(FaceRecognitionConstants::EnrolledFaceStorageVersionNumber);
  static const u16 VersionPrefix[2] = {0xFACE, VersionNumber};

  //
  // Console Vars
  //

  // Face must be recognized with score above this threshold to be considered a match
  CONSOLE_VAR_RANGED(s32, kFaceRecognitionThreshold, "Vision.FaceRecognition", 750, 0, 1000);
  
  // Non-match must be this much less than the recognition threshold to trigger registering new user
  CONSOLE_VAR_RANGED(s32, kFaceRecognitionThresholdMarginForAdding, "Vision.FaceRecognition", 200, 0, 1000);
  
  // Second-best named match when top match is session-only must be greater than FaceRecognition
  // threshold minus this margin to be used. This makes it a little easier to use 2nd best entry.
  // If there is 3rd best match that is also named but with different ID, it must also be this margin
  // _below_ the 2nd best match's score.
  CONSOLE_VAR_RANGED(s32, kFaceRecognitionThresholdMarginForUsing2ndBest, "Vision.FaceRecognition", 75, 0, 1000);
  
  // When enabled, this will merge the individual data elements of the session-only album entries
  // for two face records being merged. I don't believe this is a good idea anymore because
  // you can create a single album entry with very different features in it, which is apparently
  // not good for the OKAO libraries.
  CONSOLE_VAR(bool, kEnableMergingOfSessionOnlyAlbumEntries, "Vision.FaceRecognition", false);
  
  // Time between adding enrollment data for an existing user.
  CONSOLE_VAR(f32, kTimeBetweenFaceEnrollmentUpdates_sec, "Vision.FaceRecognition", 1.f);
  
  // If true, gets enrollment time from the image timestamp instead of system time.
  // This is especially useful for testing using image files, which may be processed
  // faster than real time.
  CONSOLE_VAR(bool, kGetEnrollmentTimeFromImageTimestamp, "Vision.FaceRecognition", false);
  
  // If true, we can add new enrollment data even after the initial slots are full,
  // where we replace the oldest. Otherwise, the initial enrollment data is the final
  // record for a person.
  CONSOLE_VAR(bool, kEnableEnrollmentAfterFull, "Vision.FaceRecognition", false);
  
  CONSOLE_VAR_RANGED(u8, kFaceRec_MinSleepTime_ms, "Vision.FaceRecognition", 5, 1, 10);
  
  CONSOLE_VAR_RANGED(u8, kFaceRecMaxDebugResults, "Vision.FaceRecognition", 2, 2, 10);
  
  CONSOLE_VAR(bool, kFaceRecognitionExtraDebug, "Vision.FaceRecognition", false);
  
  // For simulating slow processing (e.g. on a device)
  CONSOLE_VAR(u32, kFaceRecognitionSimulatedDelay_ms, "Vision.FaceRecognition", 0);
  
  namespace JsonKey
  {
    const char* FaceRecognitionGroup = "FaceRecognition";
    const char* RunMode = "RunMode";
    const char* Synchronous = "synchronous";
    const char* Asynchronous = "asynchronous";
    
    const char* PerformanceLoggingGroup = "PerformanceLogging";
    const char* TimeBetweenInfoPrints = "TimeBetweenProfilerInfoPrints_sec";
    const char* TimeBetweenDasEvents = "TimeBetweenProfilerDasLogs_sec";
  }
  
  inline static s64 GetSecondsSince(std::chrono::time_point<std::chrono::system_clock> now, EnrolledFaceEntry::Time t)
  {
    using namespace std::chrono;
    const s64 secondsSince = Util::numeric_cast<s64>(duration_cast<seconds>(now - t).count());
    return secondsSince;
  }
  
  FaceRecognizer::FaceRecognizer(const Json::Value& config)
  {
    if(config.isMember(JsonKey::FaceRecognitionGroup))
    {
      const Json::Value& recognitionConfig = config[JsonKey::FaceRecognitionGroup];
      
      if(DEBUG_ENROLLMENT_IMAGES)
      {
        // Have to run synchronously
        _isRunningAsync = false;
      }
      else
      {
        // TODO: Make this a console var too? (Not sure about switching it _while_ running though.
        std::string runModeStr;
        if(JsonTools::GetValueOptional(recognitionConfig, JsonKey::RunMode, runModeStr)) {
          if(runModeStr == JsonKey::Asynchronous) {
            _isRunningAsync = true;
          } else if(runModeStr == JsonKey::Synchronous) {
            _isRunningAsync = false;
          } else {
            DEV_ASSERT(false, "FaceRecognizer.Constructor.BadRunMode");
          }
        }
      }
    } else {
      PRINT_NAMED_WARNING("FaceRecognizer.Constructor.NoFaceRecParameters",
                          "Did not find '%s' group in config", JsonKey::FaceRecognitionGroup);
    }
    
    PRINT_CH_INFO(LOG_CHANNEL, "FaceRecognizer.Constructor.RunMode",
                  "Running in %s mode",
                  (_isRunningAsync ? JsonKey::Asynchronous : JsonKey::Synchronous));
    
    // Set up profiler logging frequencies
    f32 timeBetweenProfilerInfoPrints_sec = 5.f;
    f32 timeBetweenProfilerDasLogs_sec = 60.f;
    
    if(config.isMember(JsonKey::PerformanceLoggingGroup))
    {
      const Json::Value& performanceConfig = config[JsonKey::PerformanceLoggingGroup];
      if(!JsonTools::GetValueOptional(performanceConfig, JsonKey::TimeBetweenInfoPrints,
                                      timeBetweenProfilerInfoPrints_sec))
      {
        PRINT_NAMED_WARNING("FaceRecognizer.Constructor.MissingJsonField", "%s.%s",
                            JsonKey::PerformanceLoggingGroup, JsonKey::TimeBetweenInfoPrints);
      }
      
      if(!JsonTools::GetValueOptional(performanceConfig, JsonKey::TimeBetweenDasEvents,
                                      timeBetweenProfilerDasLogs_sec))
      {
        PRINT_NAMED_WARNING("FaceRecognizer.Constructor.MissingJsonField", "%s.%s",
                            JsonKey::PerformanceLoggingGroup, JsonKey::TimeBetweenDasEvents);
      }
      
    } else {
      PRINT_NAMED_WARNING("FaceRecognizer.Constructor.NoPerfLoggingParameters",
                          "Did not find '%s' group in config", JsonKey::PerformanceLoggingGroup);
    }

    Profiler::SetPrintFrequency(Util::SecToMilliSec(timeBetweenProfilerInfoPrints_sec));
    Profiler::SetDasLogFrequency(Util::SecToMilliSec(timeBetweenProfilerDasLogs_sec));
    Profiler::SetPrintChannelName("FaceRecognizer");
    Profiler::SetProfileGroupName("FaceRecognizer.Profiler");
  }
  
  FaceRecognizer::~FaceRecognizer()
  {
    Shutdown();
  }
  
  Result FaceRecognizer::Init(HCOMMON okaoCommonHandle)
  {
    if(NULL == okaoCommonHandle) {
      PRINT_NAMED_ERROR("FaceRecognizer.Init.NullCommonHandle", "");
      return RESULT_FAIL;
    }
    
    _okaoCommonHandle = okaoCommonHandle;
    
    _okaoRecognitionFeatureHandle = OKAO_FR_CreateFeatureHandle(_okaoCommonHandle);
    if(NULL == _okaoRecognitionFeatureHandle) {
      PRINT_NAMED_ERROR("FaceRecognizer.Init.FaceLibFeatureHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoRecogMergeFeatureHandle = OKAO_FR_CreateFeatureHandle(_okaoCommonHandle);
    if(NULL == _okaoRecogMergeFeatureHandle) {
      PRINT_NAMED_ERROR("FaceRecognizer.Init.FaceLibMergeFeatureHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoFaceAlbum = OKAO_FR_CreateAlbumHandle(_okaoCommonHandle, kMaxTotalAlbumEntries, kMaxEnrollDataPerAlbumEntry);
    if(NULL == _okaoFaceAlbum) {
      PRINT_NAMED_ERROR("FaceRecognizer.Init.FaceLibAlbumHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _detectionInfo.nID = -1;
    
    _isInitialized = true;

    if(_isRunningAsync) {
      StartThread();
    }
    
    return RESULT_OK;
  }

  Result FaceRecognizer::Shutdown()
  {
    // Wait for recognition thread to die before destructing since we gave it a
    // reference to *this
    StopThread();

    if (NULL != _okaoFaceAlbum) {
      if (OKAO_NORMAL != OKAO_FR_DeleteAlbumHandle(_okaoFaceAlbum)) {
        PRINT_NAMED_ERROR("FaceRecognizer.Shutdown.FaceLibAlbumHandleDeleteFail", "");
      }
      _okaoFaceAlbum = NULL;
    }

    if (NULL != _okaoRecogMergeFeatureHandle) {
      if (OKAO_NORMAL != OKAO_FR_DeleteFeatureHandle(_okaoRecogMergeFeatureHandle)) {
        PRINT_NAMED_ERROR("FaceRecognizer.Shutdown.FaceLibRecognitionMergeFeatureHandleDeleteFail", "");
      }
      _okaoRecogMergeFeatureHandle = NULL;
    }

    if (NULL != _okaoRecognitionFeatureHandle) {
      if (OKAO_NORMAL != OKAO_FR_DeleteFeatureHandle(_okaoRecognitionFeatureHandle)) {
        PRINT_NAMED_ERROR("FaceRecognizer.Shutdown.FaceLibRecognitionFeatureHandleDeleteFail", "");
      }
      _okaoRecognitionFeatureHandle = NULL;
    }

    return RESULT_OK;
  }

  void FaceRecognizer::StartThread()
  {
    if(_isInitialized)
    {
      if(_isRunningAsync)
      {
        // If already running,
        StopThread();
      }
      
      _isRunningAsync = true;
      
      _featureExtractionThread = std::thread(&FaceRecognizer::Run, this);
    }
    else
    {
      PRINT_NAMED_WARNING("FaceRecognizer.StartThread.NotInitialized", "");
    }
  }
  
  void FaceRecognizer::StopThread()
  {
    if(_isRunningAsync)
    {
      _isRunningAsync = false; // Must be done first, to stop the thread and make it joinable
      if(_featureExtractionThread.joinable()) {
        _featureExtractionThread.join();
      }
    }
  }
  
  void FaceRecognizer::SetIsSynchronous(bool shouldRunSynchronous)
  {
    if(shouldRunSynchronous && _isRunningAsync)
    {
      PRINT_NAMED_INFO("FaceRecognizer.SetSynchronousMode.SwitchToSynchronous", "");
      StopThread();
      
    }
    else if(!shouldRunSynchronous && !_isRunningAsync)
    {
      PRINT_NAMED_INFO("FaceRecognizer.SetSynchronousMode.SwitchToAsynchronous", "");
      if(_isInitialized)
      {
        StartThread();
      }
    }
  }
  
  Result FaceRecognizer::SanityCheckBookkeeping(const HALBUM& okaoFaceAlbum,
                                                const EnrollmentData& enrollmentData,
                                                const AlbumEntryToFaceID& albumEntryToFaceID)
  {
    // Sanity checks to make sure enrollment data and Okao albums are in sync
    
    // Number of entries stored in OKAO album should match number of entries in albumEntryToFaceID
    INT32 numEntries = 0;
    OKAO_FR_GetRegisteredUserNum(okaoFaceAlbum, &numEntries);
    if(numEntries != albumEntryToFaceID.size()) {
      PRINT_NAMED_ERROR("FaceRecognizer.SanityCheckBookkeeping.NumAlbumEntriesMismatch",
                          "FaceLibNumEntries=%d, AlbumEntryToFaceIDSize=%zu",
                          numEntries, albumEntryToFaceID.size());
      return RESULT_FAIL;
    }
    
    // Every albumEntry in albumEntryToFaceID should be registered in OKAO's album
    for(auto & albumEntry : albumEntryToFaceID)
    {
      BOOL isRegistered = false;
      OKAO_FR_IsRegistered(okaoFaceAlbum, albumEntry.first, 0, &isRegistered);
      if(!isRegistered) {
        PRINT_NAMED_ERROR("FaceRecognizer.SanityCheckBookkeeping.AlbumEntryNotRegistered",
                          "AlbumEntry=%d", albumEntry.first);
        return RESULT_FAIL;
      }
    }
    
    for(auto & enrollData : enrollmentData)
    {
      // Every album entry for this FaceID should exist in _albumEntryToFaceID and it
      // should refer back to this FaceID
      for(auto & albumEntryPair : enrollData.second.GetAlbumEntries())
      {
        const AlbumEntryID_t albumEntry = albumEntryPair.first;
        
        auto iter = albumEntryToFaceID.find(albumEntry);
        if(iter == albumEntryToFaceID.end()) {
          PRINT_NAMED_ERROR("FaceRecognizer.SanityCheckBookkeeping.MissingAlbumEntry",
                            "AlbumEntry %d for FaceID %d does not exist in albumEntryToFaceID LUT",
                            albumEntry, enrollData.first);
          return RESULT_FAIL;
        }
        
        if(iter->second != enrollData.first) {
          PRINT_NAMED_ERROR("FaceRecognizer.SanityCheckBookkeeping.LookupTablesOutOfSync",
                            "AlbumEntryToFaceID[%d] = FaceID %d instead of %d",
                            iter->first, iter->second, enrollData.first);
          return RESULT_FAIL;
        }
      }
    }

    return RESULT_OK;
    
  } // SanityCheckBookkeeping()
  
  Result FaceRecognizer::UpdateRecognitionData(const FaceID_t recognizedID,
                                               const RecognitionScore score)
  {
    // See who we're currently recognizing this tracker ID as
    FaceID_t faceID = UnknownFaceID;
    auto iter = _trackingToFaceID.find(_detectionInfo.nID);
    if(iter != _trackingToFaceID.end()) {
      faceID = iter->second;
    }
    
    if(UnknownFaceID == recognizedID) {
      // We did not recognize the tracked face in its current position, just leave
      // the faceID alone
      
      // (Nothing to do)
      
    } else if(UnknownFaceID == faceID) {
      // We have not yet assigned a recognition ID to this tracker ID. Use the
      // one we just found via recognition.
      PRINT_CH_DEBUG(LOG_CHANNEL, "UpdateRecognitionData.RecognizedNewTrackingID",
                     "Tracking ID=%d recognized as FaceID=%d",
                     -_detectionInfo.nID, recognizedID);
      
      faceID = recognizedID;
      
    } else if(faceID != recognizedID) {
      // We recognized this face as a different ID than the one currently
      // assigned to the tracking ID. Trust the tracker that they are in
      // fact the same and merge the two, keeping the original (first enrolled)
      auto faceIDenrollData = _enrollmentData.find(faceID);
      if(faceIDenrollData == _enrollmentData.end()) {
        // The tracker's assigned ID got removed (via merge while doing RecognizeFaces).
        faceID = recognizedID;
      } else {
        
        auto recIDenrollData  = _enrollmentData.find(recognizedID);
        DEV_ASSERT(recIDenrollData  != _enrollmentData.end(),
                   "FaceRecognizer.UpdateRecognitionData.MissingEnrollmentData");
        
        FaceID_t mergeTo = Vision::UnknownFaceID, mergeFrom = Vision::UnknownFaceID;
        
        if(false == recIDenrollData->second.IsForThisSessionOnly() &&
           true  == faceIDenrollData->second.IsForThisSessionOnly())
        {
          // If the recognized ID is a "permanent" one, and the tracked ID
          // is for this session only, make sure to keep the "permanent" one
          mergeFrom = faceID;
          mergeTo   = recognizedID;
        }
        else if(true  == recIDenrollData->second.IsForThisSessionOnly() &&
                false == faceIDenrollData->second.IsForThisSessionOnly())
        {
          // If the tracked ID is a "permanent" one, and the recognized ID
          // is for this session only, make sure to keep the "permanent" one
          mergeFrom = recognizedID;
          mergeTo   = faceID;
        }
        else if(false == recIDenrollData->second.IsForThisSessionOnly() &&
                false == faceIDenrollData->second.IsForThisSessionOnly())
        {
          DEV_ASSERT(!faceIDenrollData->second.GetName().empty() &&
                     !recIDenrollData->second.GetName().empty(),
                     "FaceRecognizer.UpdateRecognitionData.PermanentIDsWithNoNames");
          
          // Both IDs are named and permanent. Issue a warning, because it's
          // unclear we'd want to merge two separately enrolled people! We
          // hope this confusion never happens.
          // NOTE: Debug version displays names, warning does not (so they don't get logged)
          PRINT_NAMED_WARNING("FaceRecognizer.UpdateRecognitionData.ConfusedTwoNamedIDs",
                              "While tracking face %d with ID=%d (%s), recognized as ID=%d (%s). Not merging!",
                              -_detectionInfo.nID, faceID,
                              Util::HidePersonallyIdentifiableInfo(faceIDenrollData->second.GetName().c_str()),
                              recognizedID,
                              Util::HidePersonallyIdentifiableInfo(recIDenrollData->second.GetName().c_str()));
          
          RemoveTrackingID(_detectionInfo.nID);
          faceID = recognizedID; // So that we'll udpate the tracking ID to face ID info below
          
          // NOTE: Leave mergeTo / mergeFrom set to unknown ID to signal _not_ to merge below
        }
        else {
          // Both IDs are for this session only. Keep the oldest one.
          DEV_ASSERT(faceIDenrollData->second.IsForThisSessionOnly() &&
                     recIDenrollData->second.IsForThisSessionOnly(),
                     "FaceRecognizer.UpdateRecognitionData.BothIDsNotSessionOnly");
          
          if(faceIDenrollData->second.GetEnrollmentTime() <= recIDenrollData->second.GetEnrollmentTime())
          {
            mergeFrom = recognizedID;
            mergeTo   = faceID;
          } else {
            mergeFrom = faceID;
            mergeTo   = recognizedID;
          }
          
          if(kFaceRecognitionExtraDebug)
          {
            PRINT_CH_INFO(LOG_CHANNEL, "UpdateRecognitionData.MergeBasedOnEnrollmentTime",
                          "Merging ID=%d into ID=%d (%d enrolled at %s, %d enrolled at %s)",
                          mergeFrom,  mergeTo, faceID,
                          EnrolledFaceEntry::GetTimeString(faceIDenrollData->second.GetEnrollmentTime()).c_str(),
                          recognizedID,
                          EnrolledFaceEntry::GetTimeString(recIDenrollData->second.GetEnrollmentTime()).c_str());
          }
        }
        
        if(mergeFrom != Vision::UnknownFaceID && mergeTo != Vision::UnknownFaceID)
        {
          // Only merge the face we are currently tracking into another record
          // if it passes the enrollment criteria from the tracker.
          const bool isMergingAllowed = IsMergingAllowed(mergeTo);
          if(mergeFrom == recognizedID || isMergingAllowed)
          {
            PRINT_CH_INFO(LOG_CHANNEL, "UpdateRecognitionData.MergingFaces",
                          "Tracking %d: merging ID=%d into ID=%d (merging allowed=%d)",
                          -_detectionInfo.nID, mergeFrom, mergeTo, isMergingAllowed);
            
            Result mergeResult = MergeFaces(mergeTo, mergeFrom);
            
            if(RESULT_OK != mergeResult) {
              PRINT_NAMED_WARNING("FaceRecognizer.UpdateRecognitionData.MergeFail",
                                  "Trying to merge %d with %d", faceID, recognizedID);
            }
          }
          else if(mergeFrom == _enrollmentID)
          {
            PRINT_CH_INFO(LOG_CHANNEL, "UpdateRecognitionData.UpdateEnrollmentID",
                          "Updating enrollment ID %d->%d while to tracking %d (not merging)",
                          _enrollmentID, mergeTo, -_detectionInfo.nID);
            _enrollmentID = mergeTo;
          } else if(kFaceRecognitionExtraDebug) {
            PRINT_CH_INFO(LOG_CHANNEL, "UpdateRecognitionData.NotMergingTrackedFace",
                          "Enrollment disabled: not merging tracked face ID=%d into recognized ID=%d",
                          faceID, recognizedID);
          }
          
          // Either way we want to update the ID associated with this tracker ID
          faceID = mergeTo;
        }
      }
      
    } else {
      // We recognized this person as the same ID already assigned to its tracking ID
      DEV_ASSERT(faceID == recognizedID, "FaceRecognizer.UpdateRecognitionData.UnexpectedRecognizedID");
    }
    
    // Update the stored faceID assigned to this trackerID
    // (This creates an entry for the current trackerID if there wasn't one already,
    //  and an entry for this faceID if there wasn't one already)
    if(UnknownFaceID != recognizedID) {
      _trackingToFaceID[_detectionInfo.nID] = faceID;
      _enrollmentData[faceID].SetScore( score );
    }
    
    return RESULT_OK;
    
  } // UpdateRecognitionData()
  
  bool FaceRecognizer::HasRecognitionData(TrackingID_t forTrackingID) const
  {
    const bool haveEntry = (_trackingToFaceID.find(forTrackingID) != _trackingToFaceID.end());
    return haveEntry;
  }
  
  EnrolledFaceEntry FaceRecognizer::GetRecognitionData(INT32 forTrackingID, s32& enrollmentCountReached)
  {
    if(ProcessingState::FeaturesReady == _state)
    {
      if(_detectionInfo.nID == UnknownFaceID)
      {
        // If the tracker ID is set to Unknown, it's because a ClearTrackingData call
        // was received while the recognition thread was running on a detection.
        // That detection ID is no longer valid, so drop the result on the floor.
        // This should only be possible when running asynchronously.
        DEV_ASSERT(_isRunningAsync, "FaceRecognizer.GetRecognitionData.InvalidTrackIDinSyncMode");
        PRINT_CH_INFO(LOG_CHANNEL, "GetRecognitionData.DroppingFeaturesComputedWhileClearing", "");
      }
      else if(_isEnrollmentCancelled)
      {
        PRINT_CH_INFO(LOG_CHANNEL, "GetRecognitionData.DroppingFeaturesComputedWhileCancelled", "");
        _isEnrollmentCancelled = false;
      }
      else
      {
        // Verbose, but useful for enrollment debugging
        //  PRINT_CH_DEBUG("FaceRecognizer", "GetRecognitionData.EnrollmentStatus",
        //                 "ForTrackingID:%d EnrollmentCount=%d EnrollID=%d",
        //                 -_detectionInfo.nID, _enrollmentCount, _enrollmentID);
        
        // Feature extraction thread is done: finish the rest of the recognition
        // process so we can start accepting new requests to recognize
        FaceID_t recognizedID = UnknownFaceID;
        RecognitionScore score = 0;
        Result result = RecognizeFace(recognizedID, score);
        
        if(RESULT_OK == result)
        {
          result = UpdateRecognitionData(recognizedID, score);
          if(RESULT_OK != result)
          {
            PRINT_NAMED_ERROR("FaceRecognizer.GetRecognitionData.UpdateRecognitionDataFailed", "");
          }
        }
        else
        {
          PRINT_NAMED_ERROR("FaceRecognizer.GetRecognitionData.RecognizeFaceFailed", "");
        }
        
        if(DEBUG_ENROLLMENT_IMAGES)
        {
          DisplayEnrollmentImages();
        }
        
        if(ANKI_DEVELOPER_CODE)
        {
          const Result sanityResult = SanityCheckBookkeeping(_okaoFaceAlbum,
                                                             _enrollmentData,
                                                             _albumEntryToFaceID);
          DEV_ASSERT(sanityResult == RESULT_OK, "FaceRecognizer.GetRecognitionData.SanityCheckFailed");
        }
      }
      
      // Whether or not we used the computed features, mark that we are ready to
      // process more
      _mutex.lock();
      _state = ProcessingState::Idle;
      _mutex.unlock();
      
    } // if(ProcessingState::FeaturesReady == _state)
    
    EnrolledFaceEntry entryToReturn;
    enrollmentCountReached = 0;
    
    auto iter = _trackingToFaceID.find(forTrackingID);
    if(iter != _trackingToFaceID.end()) {
      const FaceID_t faceID = iter->second;
      DEV_ASSERT(faceID != UnknownFaceID, "FaceRecognizer.GetRecognitionData.TrackedFaceWithUnknownID");
      auto enrollIter = _enrollmentData.find(faceID);
      
      // If this assert triggers, something has gotten messed up with bookkeeping:
      // For example the enrollment data got changed (via load?) without updating
      // or clearing the trackingToFaceID LUT.
      DEV_ASSERT(enrollIter != _enrollmentData.end(), "FaceRecognizer.GetRecognitionData.TrackedFaceWithNoEnrollData");
      
      auto & enrolledEntry = enrollIter->second;
      if(_enrollmentID != UnknownFaceID &&
         enrolledEntry.WasFaceIDJustUpdated() &&
         _enrollmentID == enrolledEntry.GetPreviousFaceID())
      {
        // ID just changed for this entry and the old ID was the one we were enrolling.
        // Update the enrollmentID to match.
        if(kFaceRecognitionExtraDebug) {
          PRINT_CH_INFO(LOG_CHANNEL, "GetRecognitionData.UpdatingEnrollmentID",
                        "Old:%d -> New:%d", _enrollmentID, enrolledEntry.GetFaceID());
        }
        _enrollmentID = enrolledEntry.GetFaceID();
      }
      
      // Make a copy to return. Use the numEnrollments to tell if we just completed
      // the specified enrollment count and return that count to the caller.
      entryToReturn = enrolledEntry; // Make a copy to return
      if(_enrollmentCount == 0 && _origEnrollmentCount > 0 && _enrollmentID == entryToReturn.GetFaceID()) {
        if(kFaceRecognitionExtraDebug) {
          PRINT_CH_INFO(LOG_CHANNEL, "GetRecognitionData.EnrollmentCountReached",
                        "Count=%d", _origEnrollmentCount);
        }
        
        // Log the enrollment ID we just completed and how many album entries it now has
        const size_t numAlbumEntries = entryToReturn.GetAlbumEntries().size();
        Util::sEventF("robot.vision.face_enrollment_count_reached",
                      {{DDATA, std::to_string(numAlbumEntries).c_str()}},
                      "%d", _enrollmentID);
        
        enrollmentCountReached = _origEnrollmentCount;
        _origEnrollmentCount = 0; // signifies we've already returned it
      } 
      
      // no longer "new" or "updated"
      enrolledEntry.UpdatePreviousIDs();
    }
    
    return entryToReturn;
  } // GetRecognitionData()

  
  void FaceRecognizer::RemoveTrackingID(INT32 trackerID)
  {
    // Remove the trackerID from the LUT for face IDs, and clear it from
    // any associated enrollment entry as well
    auto iter = _trackingToFaceID.find(trackerID);
    if(iter != _trackingToFaceID.end())
    {
      const FaceID_t faceID = iter->second;
      auto enrollIter = _enrollmentData.find(faceID);
      if(enrollIter != _enrollmentData.end())
      {
        enrollIter->second.ClearTrackingID();
      }
      _trackingToFaceID.erase(iter);
    }
  }
  
  void FaceRecognizer::ClearAllTrackingData()
  {
    for(auto & enrollData : _enrollmentData)
    {
      enrollData.second.ClearTrackingID();
    }
    _trackingToFaceID.clear();
    
    if(_isRunningAsync)
    {
      // If we're in the middle of computing features on the other thread, then
      // we need to mark that the recognition data that comes back is associated
      // with a now-invalidated track ID
      _detectionInfo.nID = UnknownFaceID;
    }
  }
  
  void FaceRecognizer::Run()
  {
    Anki::Util::SetThreadName(pthread_self(), "FaceRecognizer");
    const char* threadName = "FaceRecognizer";
    #if defined(LINUX) || defined(ANDROID)
    pthread_setname_np(pthread_self(), threadName);
    #else
    pthread_setname_np(threadName);
    #endif
    
    
    while(_isRunningAsync)
    {
      _mutex.lock();
      const bool anythingToDo = ProcessingState::HasNewImage == _state;
      _mutex.unlock();
      
      if(anythingToDo) {
        
        // Note: this puts us in FeaturesReady state when done (or Idle if failure)
        ExtractFeatures();
      }
      // Sleep for a bit
      std::this_thread::sleep_for(std::chrono::milliseconds(kFaceRec_MinSleepTime_ms));
    }
    
    PRINT_NAMED_WARNING("FaceRecognizer.Run.ThreadHasStopped",
                        "FaceRecognizer thread has stopped. No longer running asynchronously");
  } // Run()
  
  
  void FaceRecognizer::ExtractFeatures()
  {
    DEV_ASSERT(ProcessingState::HasNewImage == _state, "FaceRecognizer.ExtractFeatures.ShouldBeInHasNewImageState");
    
    _mutex.lock();
    _state = ProcessingState::ExtractingFeatures;
    _mutex.unlock();
    
    INT32 nWidth  = _img.GetNumCols();
    INT32 nHeight = _img.GetNumRows();
    RAWIMAGE* dataPtr = _img.GetDataPointer();
    
    Tic("OkaoFeatureExtraction");
    OkaoResult okaoResult = OKAO_FR_ExtractHandle_GRAY(_okaoRecognitionFeatureHandle,
                                                  dataPtr, nWidth, nHeight, GRAY_ORDER_Y0Y1Y2Y3,
                                                  _okaoPartDetectionResultHandle);
    Toc("OkaoFeatureExtraction");
    
    if(kFaceRecognitionSimulatedDelay_ms > 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(kFaceRecognitionSimulatedDelay_ms));
    }
    
    ProcessingState newState = ProcessingState::FeaturesReady;
    
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_WARNING("FaceRecognizer.ExtractFeatures.FaceLibFailure",
                          "Going back to Idle state. FaceLib Result=%d", okaoResult);
      newState = ProcessingState::Idle;
    }
    
    _mutex.lock();
    _state = newState;
    _mutex.unlock();
    
  } // ExtractFeatures()
  
  FaceRecognizer::AlbumEntryID_t FaceRecognizer::GetNextAlbumEntryToUse()
  {
    const AlbumEntryID_t failureEntry = EnrolledFaceEntry::UnknownAlbumEntryID;
    
    AlbumEntryID_t numEntriesInAlbum = 0;
    OkaoResult okaoResult = OKAO_FR_GetRegisteredUserNum(_okaoFaceAlbum, &numEntriesInAlbum);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_WARNING("FaceRecognizer.GetNextAlbumEntryToUse.FaceLibGetNumUsersInAlbumFailed", "");
      return failureEntry;
    }
    
    if(numEntriesInAlbum >= kMaxTotalAlbumEntries)
    {
      // See if there are any session-only faces we can overwrite before giving up.
      // If so, choose the one that it's been the longest since we updated.
      FaceID_t oldestSessionOnlyID = UnknownFaceID;
      EnrolledFaceEntry::Time oldestUpdateTime(std::chrono::seconds::max());
      for(auto & enrollData : _enrollmentData) {
        if(enrollData.second.IsForThisSessionOnly() && enrollData.second.GetLastUpdateTime() < oldestUpdateTime) {
          oldestSessionOnlyID = enrollData.second.GetFaceID();
          oldestUpdateTime    = enrollData.second.GetLastUpdateTime();
        }
      }
      
      if(UnknownFaceID != oldestSessionOnlyID)
      {
        PRINT_CH_INFO(LOG_CHANNEL, "GetNextAlbumEntryToUse.ReplacingOldestSessionOnlyUser",
                      "Session-only face %d not updated since %s and will be replaced.",
                      oldestSessionOnlyID, EnrolledFaceEntry::GetTimeString(oldestUpdateTime).c_str());
        
        auto & albumEntries = _enrollmentData.at(oldestSessionOnlyID).GetAlbumEntries();
        
        DEV_ASSERT(!albumEntries.empty(), "FaceRecognizer.GetNextAlbumEntryToUse.OldestFaceHasNoEntries");
        
        _nextAlbumEntry = albumEntries.begin()->first;
        
        Result removeResult = RemoveUser(oldestSessionOnlyID);
        
        if(RESULT_OK != removeResult)
        {
          PRINT_NAMED_WARNING("FaceRecognizer.GetNextAlbumEntryToUse.RemoveOldestUserFailed",
                              "Attempting to remove %d", oldestSessionOnlyID);
        }
        
        // That should have freed up some entries (at least the first one!)
        BOOL isStillRegistered = false;
        okaoResult = OKAO_FR_IsRegistered(_okaoFaceAlbum, _nextAlbumEntry, 0, &isStillRegistered);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_WARNING("FaceRecognizer.GetNextAlbumEntryToUse.CouldNotCheckIfStillRegistered",
                              "Using anyway. Entry:%d FaceLib Result:%d", _nextAlbumEntry, okaoResult);
        }
          
        if(isStillRegistered)
        {
          PRINT_NAMED_WARNING("FaceRecognizer.GetNextAlbumEntryToUse.FirstFreedEntryStillRegistered",
                              "Entry:%d", _nextAlbumEntry);
          return failureEntry;
        }
        
      } else {
        PRINT_NAMED_WARNING("FaceRecognizer.GetNextAlbumEntryToUse.TooManyUsers",
                            "Already have %d users, could not add another", kMaxFacesInAlbum);
        return failureEntry;
      }
    }
    else
    {
      // Find unused ID
      BOOL isRegistered = true;
      auto tries = 0;
      do {
        okaoResult = OKAO_FR_IsRegistered(_okaoFaceAlbum, _nextAlbumEntry, 0, &isRegistered);
        
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_WARNING("FaceRecognizer.GetNextAlbumEntryToUse.IsRegisteredCheckFailed",
                              "Failed to determine if albumEntry %d is already registered. FaceLib result=%d",
                              _nextAlbumEntry, okaoResult);
          return failureEntry;
        }
        
        if(isRegistered) {
          // Try next ID
          ++_nextAlbumEntry;
          if(_nextAlbumEntry >= kMaxTotalAlbumEntries) {
            // Roll over
            _nextAlbumEntry = 0;
          }
        }
        
      } while(isRegistered && tries++ < kMaxTotalAlbumEntries);
      
      if(tries >= kMaxTotalAlbumEntries) {
        PRINT_NAMED_WARNING("FaceRecognizer.GetNextAlbumEntryToUse.NoIDsAvailable",
                            "Could not find free space in the album to use for a new entry");
        return failureEntry;
      }
    } // if/else numUsersInAlbum > max

    return _nextAlbumEntry;
    
  } // GetNextAlbumEntry()
  
  FaceID_t FaceRecognizer::GetNextFaceID()
  {
    // The only way we get stuck in this loop is if we have all INT_MAX IDs
    // stored in enrollmentData, but I think we have bigger problems if that has
    // happened...
    while(_enrollmentData.find(_nextFaceID) != _enrollmentData.end())
    {
      ++_nextFaceID;
      if(_nextFaceID == UnknownFaceID)
      {
        ++_nextFaceID; // Skip UnknownFaceID
      }
    }
      
    return _nextFaceID;
  }
  
  Result FaceRecognizer::RegisterNewUser(HFEATURE& hFeature, FaceID_t& faceID)
  {
    DEV_ASSERT(ProcessingState::FeaturesReady == _state, "FaceRecognizer.RegisterNewUser.FeaturesShouldBeReady");
    
    AlbumEntryID_t albumEntry = GetNextAlbumEntryToUse();
    faceID = GetNextFaceID();
    
    if(albumEntry < 0)
    {
      PRINT_NAMED_WARNING("FaceRecognizer.RegisterNewUser.NoAlbumEntriesAvailable", "");
      return RESULT_FAIL;
    }
    
    // Register the recognition features with the OKAO album
    OkaoResult okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, hFeature, albumEntry, 0);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_WARNING("FaceRecognizer.RegisterNewUser.RegisterDataFailed",
                          "Failed trying to register album entry %d", albumEntry);
      return RESULT_FAIL;
    }
    
    // Create new enrollment entry (defaults set by constructor)
    // NOTE: enrollData.prevID == UnknownID now, which indicates this is "new"
    // NOTE: all new registrations will start out "session only" (otherwise they would have matched something)
    const EnrolledFaceEntry::Time enrollmentTime = (kGetEnrollmentTimeFromImageTimestamp ?
                                                    EnrolledFaceEntry::Time(std::chrono::milliseconds(_img.GetTimestamp())) :
                                                    std::chrono::system_clock::now());
    EnrolledFaceEntry enrollData(faceID, enrollmentTime);
    enrollData.SetTrackingID(_detectionInfo.nID);
    enrollData.AddOrUpdateAlbumEntry(albumEntry, enrollmentTime, true);
    
    // Keep bookkeeping in sync
    _albumEntryToFaceID[albumEntry] = faceID;
    
    if(_enrollmentCount > 0) {
      --_enrollmentCount;
    }
    
    _enrollmentData.emplace(faceID, std::move(enrollData));
    
    if(DEBUG_ENROLLMENT_IMAGES) {
      SetEnrollmentImage(albumEntry, 0);
    }
    
    PRINT_CH_INFO(LOG_CHANNEL, "RegisterNewUser.Success",
                  "Added user with ID %d to album", faceID);
    
    return RESULT_OK;
  } // RegisterNewUser()
  
  void FaceRecognizer::SetAllowedEnrollments(s32 N, FaceID_t forFaceID)
  {
    // If we were enrolling a specific face and now we're being told not to
    // then mark the last enrollment as cancelled
    if(forFaceID == UnknownFaceID && _enrollmentID != UnknownFaceID && _enrollmentCount > 0)
    {
      PRINT_CH_INFO(LOG_CHANNEL, "FaceRecognizer.SetAllowedEnrollments.Cancel",
                    "Cancelling enrollment of ID %d", _enrollmentID);
      _isEnrollmentCancelled = true;
    }
    
    _enrollmentCount = N;
    _origEnrollmentCount = N;
    _enrollmentID = forFaceID;
    
    if(_enrollmentID == UnknownFaceID)
    {
      _enrollmentTrackID = UnknownFaceID;
    }
    else
    {
      auto enrollDataIter = _enrollmentData.find(_enrollmentID);
      if(enrollDataIter == _enrollmentData.end()) {
        PRINT_NAMED_WARNING("FaceRecognizer.SetAllowedEnrollments.NoEnrollmentData",
                            "No data for enrollmentID=%d", _enrollmentID);
        _enrollmentTrackID = UnknownFaceID;
      } else {
        _enrollmentTrackID = enrollDataIter->second.GetTrackingID();
      }
    }
  }
 
  bool FaceRecognizer::SetNextFaceToRecognize(const Image& img,
                                              const DETECTION_INFO& detectionInfo,
                                              HPTRESULT okaoPartDetectionResultHandle,
                                              bool enableEnrollment)
  {
    // Nothing to do if we aren't allowed to enroll anyone and there's nobody
    // in the album yet to match this face to. Also ignore detections that are
    // being "held" (not actually tracked/detected in this frame).
    const bool anythingToDo = (enableEnrollment || !_enrollmentData.empty()) && detectionInfo.nHoldCount==0;
    
    if(ProcessingState::Idle == _state && anythingToDo)
    {
      // Not currently busy: copy in the given data and start working on it
      
      _mutex.lock();
      img.CopyTo(_img);
      _isEnrollmentEnabled = enableEnrollment;
      _okaoPartDetectionResultHandle = okaoPartDetectionResultHandle;
      _detectionInfo = detectionInfo;
      _state = ProcessingState::HasNewImage;
      _mutex.unlock();
      
      //PRINT_CH_INFO("FaceRecognizer", "SetNextFaceToRecognize.SetNextFace",
      //                  "Setting next face to recognize: tracked ID %d", -_detectionInfo.nID);
      
      if(!_isRunningAsync) {
        // Immediately extract features when running synchronously
        ExtractFeatures();
      }
      
      return true;
      
    } else {
      // Busy: tell the caller no
      return false;
    }
  }
  
  
  Result FaceRecognizer::UpdateExistingAlbumEntry(AlbumEntryID_t albumEntry, HFEATURE& hFeature, RecognitionScore score)
  {
    DEV_ASSERT(ProcessingState::FeaturesReady == _state, "FaceRecognizer.UpdateExistingAlbumEntry.FeaturesShouldBeReady");
    
    Result result = RESULT_OK;
    
    const EnrolledFaceEntry::Time updateTime = (kGetEnrollmentTimeFromImageTimestamp ?
                                                EnrolledFaceEntry::Time(std::chrono::milliseconds(_img.GetTimestamp())) :
                                                std::chrono::system_clock::now());
    
    auto albumToFaceIter = _albumEntryToFaceID.find(albumEntry);
    DEV_ASSERT_MSG(albumToFaceIter != _albumEntryToFaceID.end(),
                   "FaceRecognizer.UpdateExistingAlbumEntry.MissingAlbumToFaceEntry",
                   "AlbumEntry:%d", albumEntry);
    const FaceID_t faceID = albumToFaceIter->second;
    
    auto enrollDataIter = _enrollmentData.find(faceID);
    DEV_ASSERT_MSG(enrollDataIter != _enrollmentData.end(),
                   "FaceRecognizer.UpdateExistingAlbumEntry.MissingEnrollmentStatus",
                   "FaceID:%d", faceID);
    
    auto & enrollData = enrollDataIter->second;
    
    // Always update the last seen time for this entry
    enrollData.SetAlbumEntryLastSeenTime(albumEntry, updateTime);
    
    // Keep the tracking ID current
    if(_detectionInfo.nID != enrollData.GetTrackingID()) {
      PRINT_CH_INFO(LOG_CHANNEL, "UpdateExistingAlbumEntry.UpdateTrackID",
                    "Update trackID for face %d: %d -> %d",
                    faceID, -enrollData.GetTrackingID(), -_detectionInfo.nID);
      _trackingToFaceID.erase(enrollData.GetTrackingID());
      enrollData.SetTrackingID(_detectionInfo.nID);
    }
    
    INT32 numDataStored = 0;
    OkaoResult okaoResult = OKAO_FR_GetRegisteredUsrDataNum(_okaoFaceAlbum, albumEntry, &numDataStored);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceRecognizer.UpdateExistingAlbumEntry.GetRegisteredUserDataFailed",
                        "albumEntry:%d dataEntry:%d FaceLib result=%d",
                        albumEntry, numDataStored, okaoResult);
      return RESULT_FAIL;
    }
    
    auto const timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::seconds>(updateTime - enrollData.GetLastUpdateTime());
    
    auto const timeBetweenEnrollments = std::chrono::seconds(Util::numeric_cast<s64>(std::round(kTimeBetweenFaceEnrollmentUpdates_sec)));
    
    const bool isEnrollmentEnabled = _enrollmentCount != 0 && (_enrollmentID == UnknownFaceID ||
                                                               _enrollmentID == faceID);
    
    const bool isNamedFace = !enrollData.GetName().empty();
    const bool hasEnrollSpaceLeft = numDataStored < kMaxEnrollDataPerAlbumEntry;
    const bool haveEnrollmentCountAndID = _enrollmentCount > 0 && _enrollmentID != UnknownFaceID;
    const bool enrollEvenIfFull = !isNamedFace || kEnableEnrollmentAfterFull || haveEnrollmentCountAndID;
    const bool isTimeToEnroll = timeSinceLastUpdate.count() >= timeBetweenEnrollments.count();
    
    if(isEnrollmentEnabled && isTimeToEnroll && (hasEnrollSpaceLeft || enrollEvenIfFull))
    {
      // Num data for user should be > 0 if we are updating!
      DEV_ASSERT(numDataStored > 0, "FaceRecognizer.UpdateExistingAlbumEntry.BadNumData");
      
      AlbumEntryID_t entryToReplace = EnrolledFaceEntry::UnknownAlbumEntryID;
      if(hasEnrollSpaceLeft)
      {
        // Simple case: just add this data to the album entry. No need to figure which records to keep.
        entryToReplace = numDataStored;
        OkaoResult okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, hFeature, albumEntry, entryToReplace);
        if(OKAO_NORMAL != okaoResult)
        {
          PRINT_NAMED_WARNING("FaceRecognizer.UpdateExistingUser.FailedToRegisterNewEntry",
                              "AlbumEntry:%d Data:%d for FaceID:%d, FaceLib result %d",
                              albumEntry, entryToReplace, faceID, okaoResult);
          return RESULT_FAIL;
        }
        
        if(kFaceRecognitionExtraDebug) {
          PRINT_CH_INFO(LOG_CHANNEL, "UpdateExistinguser",
                        "Adding Data %d to AlbumEntry %d of %zu for FaceID %d",
                        entryToReplace, albumEntry, enrollData.GetAlbumEntries().size(), faceID);
        }
      }
      else if(enrollEvenIfFull)
      {
        RecognitionScore newEntryScore;
        OkaoResult okaoResult = OKAO_FR_Verify(hFeature, _okaoFaceAlbum, albumEntry, &newEntryScore);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_ERROR("FaceRecognizer.UpdateExistingAlbumEntry.VerifyNewFeatureFailed",
                            "albumEntry:%d FaceLib result=%d",
                            albumEntry, okaoResult);
          return RESULT_FAIL;
        }
        
        // Find the entry with a score lower than the potential new entry (and keep
        // the lowest of those). If we find one, we will replace it with this new
        // better-matching feature.
        RecognitionScore lowestScoreBelowNewEntry = newEntryScore;
        for(s32 iData=0; iData < kMaxEnrollDataPerAlbumEntry; ++iData)
        {
          // Temporarily remove each data entry from the album entry and see what it's
          // score is when compared back against the album entry.
          okaoResult = OKAO_FR_GetFeatureFromAlbum(_okaoFaceAlbum, albumEntry, iData, _okaoRecogMergeFeatureHandle);
          if(OKAO_NORMAL != okaoResult) {
            PRINT_NAMED_ERROR("FaceRecognizer.UpdateExistingAlbumEntry.GetFeatureFailed",
                              "albumEntry:%d dataEntry:%d FaceLib result=%d",
                              albumEntry, iData, okaoResult);
            return RESULT_FAIL;
          }
          
          // Remove the current data
          okaoResult = OKAO_FR_ClearData(_okaoFaceAlbum, albumEntry, iData);
          if(OKAO_NORMAL != okaoResult) {
            PRINT_NAMED_ERROR("FaceRecognizer.UpdateExistingAlbumEntry.ClearDataFailed",
                              "albumEntry:%d dataEntry:%d FaceLib result=%d",
                              albumEntry, iData, okaoResult);
            return RESULT_FAIL;
          }
          
          RecognitionScore currentEntryScore;
          okaoResult = OKAO_FR_Verify(_okaoRecogMergeFeatureHandle, _okaoFaceAlbum, albumEntry, &currentEntryScore);
          if(OKAO_NORMAL != okaoResult) {
            PRINT_NAMED_ERROR("FaceRecognizer.UpdateExistingAlbumEntry.VerifyExistingFeatureFailed",
                              "albumEntry:%d FaceLib result=%d",
                              albumEntry, okaoResult);
            return RESULT_FAIL;
          }
          
          if(currentEntryScore < lowestScoreBelowNewEntry) {
            entryToReplace = iData;
            lowestScoreBelowNewEntry = currentEntryScore;
          }
          
          // Put the current data back
          okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, _okaoRecogMergeFeatureHandle, albumEntry, iData);
          if(OKAO_NORMAL != okaoResult) {
            PRINT_NAMED_ERROR("FaceRecognizer.UpdateExistingAlbumEntry.ReRegisterDataFailed",
                              "albumEntry:%d dataEntry:%d FaceLib result=%d",
                              albumEntry, iData, okaoResult);
            return RESULT_FAIL;
          }
          
          if(kFaceRecognitionExtraDebug) {
            PRINT_CH_INFO(LOG_CHANNEL, "UpdateExistingUser.EvaluatingAlbumEntryClusteredness",
                          "Data:%d in AlbumEntry:%d got score:%d (vs. possible addition with score:%d)",
                          iData, albumEntry, currentEntryScore, newEntryScore);
          }
        }
        
        if(EnrolledFaceEntry::UnknownAlbumEntryID != entryToReplace)
        {
          okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, hFeature, albumEntry, entryToReplace);
          if(OKAO_NORMAL != okaoResult) {
            PRINT_NAMED_ERROR("FaceRecognizer.UpdateExistingAlbumEntry.RegisterReplacementDataFailed",
                              "albumEntry:%d dataEntry:%d FaceLib result=%d",
                              albumEntry, entryToReplace, okaoResult);
            return RESULT_FAIL;
          }

          if(kFaceRecognitionExtraDebug) {
            PRINT_CH_INFO(LOG_CHANNEL, "UpdateExistingUser.ReplacedDataEntry",
                          "Replaced Data:%d in AlbumEntry:%d with new feature (score:%d)",
                          entryToReplace, albumEntry, lowestScoreBelowNewEntry);
          }
        }
      }
      else
      {
        // one of hasEnrollSpaceLeft or enrollEvenIfFull should be true
        DEV_ASSERT(false, "FaceRecognizer.UpdateExistingAlbumEntry.IfElseLogicError");
      }
      
      // Even if we didn't actually update: the following will update the last time we at
      // least considered doing so.
      const bool isSessionOnly = (_enrollmentID != enrollData.GetFaceID());
      enrollData.AddOrUpdateAlbumEntry(albumEntry, updateTime, isSessionOnly);
      
      if(DEBUG_ENROLLMENT_IMAGES && entryToReplace != EnrolledFaceEntry::UnknownAlbumEntryID) {
        // Update the enrollment image _after_ we update the lastUpdateTime so it shows
        // up in the display
        SetEnrollmentImage(albumEntry, entryToReplace);
      }
      
      if(_enrollmentCount > 0) {
        --_enrollmentCount;
      }
      
    }
    else
    {
      PRINT_CH_DEBUG(LOG_CHANNEL, "FaceRecognizer.UpdateExistingAlbumEntry.NotAddingEnrollmentEntry",
                     "FaceID:%d isEnabled:%d isNamed:%d hasSpaceLeft:%d haveCountAndID:%d enrollIfFull:%d isTime:%d",
                     faceID, isEnrollmentEnabled, isNamedFace, hasEnrollSpaceLeft,
                     haveEnrollmentCountAndID, enrollEvenIfFull, isTimeToEnroll);
    }
    
    return result;
  } // UpdateExistingUser()
  
  void FaceRecognizer::SetEnrollmentImage(AlbumEntryID_t albumEntry, s32 dataEntry)
  {
    ImageRGB enrollmentImg(_img); // copy!
    
    POINT ptLeftTop, ptRightTop, ptLeftBottom, ptRightBottom;
    OkaoResult okaoResult = OKAO_CO_ConvertCenterToSquare(_detectionInfo.ptCenter,
                                                     _detectionInfo.nHeight,
                                                     0, &ptLeftTop, &ptRightTop,
                                                     &ptLeftBottom, &ptRightBottom);
    DEV_ASSERT(OKAO_NORMAL == okaoResult, "FaceRecognizer.SetEnrollmentImage.GetDetectionSquareFail");
    
    const Rectangle<f32> detectionRect(ptLeftTop.x, ptLeftTop.y,
                                       ptRightBottom.x-ptLeftTop.x,
                                       ptRightBottom.y-ptLeftTop.y);
    
    enrollmentImg.DrawRect(detectionRect, NamedColors::RED, 3);
    
    _enrollmentImages[albumEntry][dataEntry] = enrollmentImg;
  }

  // Helper to create the title for the dislay window. Either "UserN" or "UserN:<name>"
  inline std::string GetDisplayName(const EnrolledFaceEntry& enrollData)
  {
    std::string dispName = "User" + std::to_string(enrollData.GetFaceID());
    if(!enrollData.IsForThisSessionOnly())
    {
      dispName += ":";
      dispName += enrollData.GetName();
    }
    return dispName;
  }
  
  void FaceRecognizer::DisplayEnrollmentImages() const
  {
    // Parameters for creating tiled display of enrollment images
    const s32 numDispRows = kMaxAlbumEntriesPerFace, numDispCols = kMaxEnrollDataPerAlbumEntry;
    std::vector<s32> dispRows, dispCols;
    dispRows.reserve(numDispRows * numDispCols);
    dispCols.reserve(numDispRows * numDispCols);
    for(s32 iRow=0; iRow<numDispRows; ++iRow)
    {
      for(s32 jCol=0; jCol<numDispCols; ++jCol)
      {
        dispRows.push_back(iRow);
        dispCols.push_back(jCol);
      }
    }
    const s32 dispResDownsample = 3;
    const s32 dispWidth  = _img.GetNumCols() / dispResDownsample;
    const s32 dispHeight = _img.GetNumRows() / dispResDownsample;
    
    ImageRGB dispImg(dispHeight*numDispRows, dispWidth*numDispCols);
    
    const EnrolledFaceEntry::Time currentTime = std::chrono::system_clock::now();
    
    for(auto & enrollData : _enrollmentData)
    {
      if(enrollData.second.FindLastSeenTime() < currentTime - std::chrono::seconds(2))
      {
        // Don't bother if this entry hasn't been updated recently
        continue;
      }
      
      dispImg.FillWith(0);
      
      s32 iAlbumEntry=0;
      EnrolledFaceEntry::Time lastSeenTime = EnrolledFaceEntry::Time(std::chrono::seconds(0));
      s32 lastSeenIndex = -1;
      
      const AlbumEntryID_t sessionOnlyEntry = enrollData.second.GetSessionOnlyAlbumEntry();
      s32 sessionOnlyIndex = -1;
      
      for(auto & albumEntry : enrollData.second.GetAlbumEntries())
      {
        auto enrollmentImgIter = _enrollmentImages.find(albumEntry.first);
        if(enrollmentImgIter == _enrollmentImages.end())
        {
          // We don't have enrollment image data for this album entry (likely
          // loaded from saved data)
          continue;
        }
        
        for(s32 iDataEntry=0; iDataEntry<kMaxEnrollDataPerAlbumEntry; ++iDataEntry)
        {
          const ImageRGB& enrollmentImage = enrollmentImgIter->second[iDataEntry];
          
          if(!enrollmentImage.IsEmpty())
          {
            // Get ROI for this tile in the display image
            Rectangle<s32> dispRect(dispCols[iDataEntry]*dispWidth,
                                    dispRows[iAlbumEntry]*dispHeight,
                                    dispWidth, dispHeight);
            
            ImageRGB dispROI = dispImg.GetROI(dispRect);
            
            enrollmentImage.Resize(dispROI, Vision::ResizeMethod::NearestNeighbor);
            
            dispROI.DrawText({2,dispROI.GetNumRows()-2},
                             std::string("Score: ") + std::to_string(enrollData.second.GetScore()),
                             NamedColors::RED, 0.15f, true);
            dispROI.DrawText({0,10}, EnrolledFaceEntry::GetTimeString(enrollData.second.GetLastUpdateTime()),
                             NamedColors::RED, 0.15f, true);
          }
          
          // Keep track of most-recently-seen album entry
          if(albumEntry.second > lastSeenTime)
          {
            lastSeenTime = albumEntry.second;
            lastSeenIndex = iAlbumEntry;
          }
          
          if(albumEntry.first == sessionOnlyEntry)
          {
            sessionOnlyIndex = iAlbumEntry;
          }
          
          ++iAlbumEntry;
        }
      }
      
      // Draw a thick blue bar to the left of the row that's the session-only entry
      if(sessionOnlyIndex >= 0 && sessionOnlyIndex < dispRows.size())
      {
        dispImg.DrawLine({2, dispRows[sessionOnlyIndex]*dispHeight},
                         {2, (dispRows[sessionOnlyIndex]+1)*dispHeight},
                         NamedColors::BLUE, 4);
      }
      
      // Draw a thick yellow bar to the right of the row that was seen most recently
      if(lastSeenIndex >= 0 && lastSeenIndex < dispRows.size())
      {
        dispImg.DrawLine({(f32)dispImg.GetNumCols()-3, dispRows[lastSeenIndex]*dispHeight},
                         {(f32)dispImg.GetNumCols()-3, (dispRows[lastSeenIndex]+1)*dispHeight},
                         NamedColors::YELLOW, 4);
      }
      
      dispImg.Display(GetDisplayName(enrollData.second).c_str());
    }
  }
  
  FaceRecognizer::EnrollmentData::iterator FaceRecognizer::RemoveUser(EnrollmentData::iterator userIter)
  {
    _trackingToFaceID.erase(userIter->second.GetTrackingID());
    
    const FaceID_t faceID = userIter->first;
    
    for(auto & albumEntryPair : userIter->second.GetAlbumEntries())
    {
      const AlbumEntryID_t albumEntry = albumEntryPair.first;
      
      _albumEntryToFaceID.erase(albumEntry);
      
      OkaoResult okaoResult = OKAO_FR_ClearUser(_okaoFaceAlbum, albumEntry);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_WARNING("FaceRecognizer.RemoveUser.ClearUserFailed",
                            "AlbumEntry:%d FaceID:%d FaceLib result:%d",
                            albumEntry, faceID, okaoResult);
      }
      
      if(DEBUG_ENROLLMENT_IMAGES)
      {
        _enrollmentImages.erase(albumEntry);
      }
    }
    
    if(DEBUG_ENROLLMENT_IMAGES)
    {
      Vision::Image::CloseDisplayWindow(GetDisplayName(userIter->second).c_str());
    }
    
    // TODO: Keep a reference to which trackerIDs are pointing to each faceID to avoid this search
    if(ANKI_DEVELOPER_CODE)
    {
      // Sanity check: there should be no more references to this faceID
      for(auto iter = _trackingToFaceID.begin(); iter != _trackingToFaceID.end(); )
      {
        if(iter->second == faceID) {
          PRINT_NAMED_WARNING("FaceRecognizer.RemoveUserHelper.StaleTrackingToFaceID",
                              "TrackID %d still maps to FaceID %d",
                              iter->first, iter->second);
          iter = _trackingToFaceID.erase(iter);
        } else {
          ++iter;
        }
      }
      for(auto iter = _albumEntryToFaceID.begin(); iter != _albumEntryToFaceID.end(); )
      {
        if(iter->second == faceID) {
          PRINT_NAMED_WARNING("FaceRecognizer.RemoveUserHelper.StaleAlbumEntryToFaceID",
                              "AlbumEntry %d still maps to FaceID %d",
                              iter->first, iter->second);
          iter = _albumEntryToFaceID.erase(iter);
        } else {
          ++iter;
        }
      }
    } // if(ANKI_DEVELOPER_CODE)
    
    return _enrollmentData.erase(userIter);
  }
  
  Result FaceRecognizer::RemoveUser(FaceID_t faceID)
  {
    auto userIter = _enrollmentData.find(faceID);
    if(userIter != _enrollmentData.end())
    {
      RemoveUser(userIter);
    } else {
      PRINT_CH_INFO(LOG_CHANNEL, "RemoveUser.UserDoesNotExist", "FaceID=%d", faceID);
      // Should this return failure?
    }
    
    return RESULT_OK;
  }
  
  FaceID_t FaceRecognizer::GetFaceIDforAlbumEntry(AlbumEntryID_t albumEntry) const
  {
    auto iter = _albumEntryToFaceID.find(albumEntry);
    if(iter == _albumEntryToFaceID.end())
    {
      // Bookkeeping has failed us...
      PRINT_NAMED_ERROR("FaceRecognizer.GetFaceIDforAlbumEntry.MissingEntry",
                        "AlbumEntry:%d", albumEntry);
      return UnknownFaceID;
    }
    
    return iter->second;
  }
  
  void FaceRecognizer::AddDebugInfo(FaceID_t matchedID, RecognitionScore score,
                                    std::list<FaceRecognitionMatch>& newDebugInfo) const
  {
    auto iter = _enrollmentData.find(matchedID);
    newDebugInfo.emplace_back(FaceRecognitionMatch{
      .name = (iter == _enrollmentData.end() ? "" : iter->second.GetName()),
      .matchedID = matchedID,
      .score  = score,
    });
  }

  Result FaceRecognizer::RecognizeFace(FaceID_t& faceID, RecognitionScore& recognitionScore)
  {
    // In case something goes wrong, make sure return values are sane
    faceID = UnknownFaceID;
    recognitionScore = 0;
    
    DEV_ASSERT(ProcessingState::FeaturesReady == _state,
               "FaceRecognizer.RecognizerFace.FeaturesShouldBeReady");
    
    INT32 numUsersInAlbum = 0;
    OkaoResult okaoResult = OKAO_FR_GetRegisteredUserNum(_okaoFaceAlbum, &numUsersInAlbum);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_WARNING("FaceRecognizer.RecognizeFace.FaceLibGetNumUsersInAlbumFailed", "");
      return RESULT_FAIL;
    }
    
    const bool haveEnrollmentCountsLeft = _enrollmentCount != 0;
    const bool canEnrollAnyFace         = (_enrollmentID == UnknownFaceID);
    
    const INT32 kMaxScore = 1000;
    
    if(numUsersInAlbum == 0 && haveEnrollmentCountsLeft && canEnrollAnyFace)
    {
      // Special case: Nobody in album yet, just add this person
      PRINT_CH_INFO(LOG_CHANNEL, "RecognizeFace.AddingFirstUser",
                    "Adding first user to empty album");
      
      Result lastResult = RegisterNewUser(_okaoRecognitionFeatureHandle, faceID);
      if(RESULT_OK != lastResult) {
        PRINT_NAMED_WARNING("FaceRecognizer.RecognizeFace.FailedToRegisterFirstUser",
                            "FaceID:%d", faceID);
        return lastResult;
        
      }
      
      recognitionScore = kMaxScore;
      return RESULT_OK;
    }
    
    // See if we recognize the person using the features we extracted on the
    // feature-extraction thread
    INT32 resultNum = 0;
    std::vector<AlbumEntryID_t> matchingAlbumEntries(kMaxFacesInAlbum);
    std::vector<RecognitionScore> scores(kMaxFacesInAlbum);
    Tic("OkaoIdentify");
    okaoResult = OKAO_FR_Identify(_okaoRecognitionFeatureHandle, _okaoFaceAlbum, kMaxFacesInAlbum,
                                  matchingAlbumEntries.data(), scores.data(), &resultNum);
    Toc("OkaoIdentify");
    
    if(resultNum > kMaxFacesInAlbum)
    {
      PRINT_NAMED_ERROR("FaceRecognizer.RecognizeFace.FaceLibReturnedBadResultNum",
                        "%d > %d", resultNum, kMaxFacesInAlbum);
      resultNum = kMaxFacesInAlbum;
    }
    
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_WARNING("FaceRecognizer.RecognizeFace.FaceLibFaceRecognitionIdentifyFailed",
                          "maxResults:%d, hFeature:%p, hAlbum:%p, FaceLib Result Code=%d",
                          kMaxFacesInAlbum, _okaoRecognitionFeatureHandle, _okaoFaceAlbum, okaoResult);
      // Sometimes this happens (bad features?), so just warn and return "OK"
      return RESULT_OK;
    }
    
    //const f32   RelativeRecogntionThreshold = 1.5; // Score of top result must be this times the score of the second best result
    
    const bool foundMatchAboveThreshold = (resultNum > 0) && (scores[0] > kFaceRecognitionThreshold);
    if(foundMatchAboveThreshold)
    {
      s32 matchIndex = 0;
      FaceID_t matchingID = GetFaceIDforAlbumEntry(matchingAlbumEntries[0]);
      if(matchingID == UnknownFaceID) {
        PRINT_NAMED_WARNING("FaceRecognizer.RecognizeFace.FailedToGetTopFaceID",
                            "MatchingAlbumEntry=%d", matchingAlbumEntries[0]);
        return RESULT_FAIL;
      }
      
      RecognitionScore matchingScore = scores[0];
      
      auto matchIter = _enrollmentData.find(matchingID);
      
      if(matchIter == _enrollmentData.end()) {
        PRINT_NAMED_WARNING("FaceRecognizer.RecognizeFace.MissingTopMatchEnrollmentData",
                            "ID:%d", matchingID);
        return RESULT_FAIL;
      }
      
      {
        // Populate the debug info for the recognized face. Always put on the top match
        // Then add the first (top-scoring) album match for each face ID, until we've
        // filled the debug list Do this before checking for 2nd-best or calling
        // UpdateExistingUser because those could cause us to cull an album entry
        // associated with matchedID, which could result in GetFaceIDforAlbumEntry
        // to fail in this loop.
        std::list<FaceRecognitionMatch> newDebugInfo{
          FaceRecognitionMatch{
            .name = matchIter->second.GetName(),
            .matchedID = matchingID,
            .score = matchingScore,
          }
        };
        s32 iResult = matchIndex+1;
        BOUNDED_WHILE(kMaxFacesInAlbum, iResult < resultNum && newDebugInfo.size() < kFaceRecMaxDebugResults)
        {
          // It's possible, due to using a non-top match above, that we have performed
          // a merge and entries in matchingAlbumEntries
          auto const matchedID = GetFaceIDforAlbumEntry(matchingAlbumEntries[iResult]);
          if(matchedID != newDebugInfo.back().matchedID)
          {
            auto iter = _enrollmentData.find(matchedID);
            newDebugInfo.emplace_back(FaceRecognitionMatch{
              .name = (iter == _enrollmentData.end() ? "" : iter->second.GetName()),
              .matchedID = matchedID,
              .score  = scores[iResult],
            });
          }
          ++iResult;
        }
        
        matchIter->second.SetDebugMatchingInfo(std::move(newDebugInfo));
      }
      
      // Typically, we will update the recognized album entry. If we end up
      // using a 2nd best match, though, it is possible we can't, and this
      // flag will get set to false.
      bool shouldUpdateAlbumEntry = true;
      
      // If we've got more than one match, the top match is session-only, and it's
      // not the ID we're specifically enrolling, then consider using a lower-scoring,
      // named match instead
      const bool haveMoreResults = resultNum >= 2;
      const bool isTopMatchSessionOnly = matchIter->second.IsForThisSessionOnly();
      const bool isTopMatchBeingEnrolled = matchingID == _enrollmentID;
      
      if(haveMoreResults && isTopMatchSessionOnly && !isTopMatchBeingEnrolled)
      {
        // Find the next match with a different FaceID than the top match, and which
        // is also named (non-session-only, unlike the top match)
        FaceID_t nextMatchingID = UnknownFaceID;
        s32 nextIndex = 1;
        do {
          nextMatchingID = GetFaceIDforAlbumEntry(matchingAlbumEntries[nextIndex]);
          
          auto nextMatchIter = _enrollmentData.find(nextMatchingID);
          
          if(nextMatchIter == _enrollmentData.end())
          {
            PRINT_NAMED_WARNING("FaceRecognizer.RecognizeFace.Missing2ndMatchEnrollmentData",
                                "ID:%d", nextMatchingID);
            nextMatchingID = UnknownFaceID;
            break;
          }
          
          if(nextMatchIter->second.IsForThisSessionOnly())
          {
            // Nope, this one is session only so keep looking.
            nextMatchingID = UnknownFaceID;
          }
          else
          {
            // Yep, it's named, so it's got potential. Check its score.
            const bool nextScoreHighEnough = (scores[nextIndex] > kFaceRecognitionThreshold - kFaceRecognitionThresholdMarginForUsing2ndBest);
            
            if(nextScoreHighEnough)
            {
              // Yep, score is high enough. Check to make sure there are no other named matches
              // with a different ID that are too close to the 2nd best.
              
              const s32 lowThreshold = std::min(scores[nextIndex] - kFaceRecognitionThresholdMarginForUsing2ndBest,
                                                kFaceRecognitionThreshold - 2*kFaceRecognitionThresholdMarginForUsing2ndBest);
              
              FaceID_t nextNextID = Vision::UnknownFaceID;
              bool nextNextScoreLowEnough = true;
              s32 nextNextIndex = nextIndex + 1;
              BOUNDED_WHILE(kMaxFacesInAlbum, nextNextIndex < resultNum && scores[nextNextIndex] > lowThreshold)
              {
                nextNextID = GetFaceIDforAlbumEntry(matchingAlbumEntries[nextNextIndex]);
                if(nextNextID != nextMatchingID)
                {
                  auto nextNextIter = _enrollmentData.find(nextNextID);
                  if(nextNextIter == _enrollmentData.end())
                  {
                    PRINT_NAMED_WARNING("FaceRecognizer.RecognizeFace.Missing3rdMatchEnrollmentData",
                                        "ID:%d", nextNextID);
                    
                  }
                  else if(!nextMatchIter->second.IsForThisSessionOnly())
                  {
                    // If _any_ named match with a different ID is above the low threshold
                    // then we are done
                    nextNextScoreLowEnough = false;
                    break;
                  }
                }
                
                ++nextNextIndex;
              }
              
              if(nextNextScoreLowEnough)
              {
                // We just realized a session-only face is actually someone we know with a high enough score.
                // (And the score after that, if there is one, is low enough.)
                // Merge the two. Note that if this is not the second best match and there are other session-only
                // entries in between, this will likely happen again and they will get merged in one by one.
                // At least that's the theory... This is an unlikely scenario regardless.
                PRINT_CH_INFO(LOG_CHANNEL, "RecognizeFace.UsingLowerRankedMatch",
                              "Top match (AlbumEntry:%d ID:%d Score:%d) is session-only. "
                              "Match at index %d (AlbumEntry:%d ID:%d Score:%d) is named ('%s'). Using it and merging.",
                              matchingAlbumEntries[matchIndex], matchingID, matchingScore,
                              nextIndex, matchingAlbumEntries[nextIndex], nextMatchingID, scores[nextIndex],
                              Util::HidePersonallyIdentifiableInfo(nextMatchIter->second.GetName().c_str()));
                
                // Log IDs in s_val and scores in DDATA:
                Util::sEventF("robot.vision.face_recognition.using_lower_ranked_match",
                              {{DDATA, (std::to_string(matchingScore) + ", " +
                                        std::to_string(scores[nextIndex]) + ", " +
                                        std::to_string(nextNextID == Vision::UnknownFaceID ? -1 : scores[nextNextIndex])).c_str()}},
                              "%d, %d, %d", matchingID, nextMatchingID, nextNextID);
                
                if(ANKI_DEV_CHEATS)
                {
                  PRINT_CH_DEBUG(LOG_CHANNEL, "RecognizeFace.BeforeMergeOfLowerRankedMatch",
                                 "Top match entries: %s. Next match entries: %s.",
                                 matchIter->second.GetAlbumEntriesString().c_str(),
                                 nextMatchIter->second.GetAlbumEntriesString().c_str());
                }
                
                Result mergeResult = MergeFaces(nextMatchingID, matchingID);
                if(RESULT_OK != mergeResult) {
                  PRINT_NAMED_WARNING("FaceRecognizer.RecognizeFace.MergeFacesFailed", "Merging %d into %d",
                                      matchingID, nextMatchingID);
                }
                
                // If nextMatchingID is set, nextMatchIter should be as well
                DEV_ASSERT(nextMatchIter != _enrollmentData.end(),
                           "FaceRecognizer.RecognizeFace.NextMatchIterNotSet");
                
                if(ANKI_DEV_CHEATS)
                {
                  // Note: matchIter is no longer valid -- its entry has been merged into nextMatchIter!
                  PRINT_CH_DEBUG(LOG_CHANNEL, "RecognizeFace.AfterMergeOfLowerRankedMatch",
                                 "Merged match entries: %s",
                                 nextMatchIter->second.GetAlbumEntriesString().c_str());
                }
                
                // We just merged, so the matching album entry for "matchIndex" no
                // longer exists. Update it to be what is in "nextIndex".
                matchingAlbumEntries[matchIndex] = matchingAlbumEntries[nextIndex];
                
                matchingID    = nextMatchingID;
                matchingScore = scores[nextIndex];
                matchIndex    = nextIndex;
                matchIter     = nextMatchIter;
                
                const AlbumEntryID_t recognizedAlbumEntryID = matchingAlbumEntries[matchIndex];
                auto const& matchIterAlbumEntries = matchIter->second.GetAlbumEntries();
                if(matchIterAlbumEntries.find(recognizedAlbumEntryID) == matchIterAlbumEntries.end())
                {
                  // The merge above removed the album entry we actually recognized, so
                  // we have nothing to update below with the call to UpdateExistingAlbumEntry().
                  shouldUpdateAlbumEntry = false;
                  
                  // If the recognized entry isn't part of the matched enrollment data anymore (matchIter),
                  // then neither should it be in the album entry to face ID lookup table anymore.
                  DEV_ASSERT_MSG(_albumEntryToFaceID.find(recognizedAlbumEntryID) == _albumEntryToFaceID.end(),
                                 "FaceRecognizer.RecognizeFace.UnexpectedAlbumEntryToFaceID",
                                 "matchIndex:%d albumEntry:%d still maps to faceID:%d",
                                     matchIndex, recognizedAlbumEntryID,
                                     _albumEntryToFaceID.at(recognizedAlbumEntryID));
                }
              } // if(nextNextScoreLowEnough)
            } // if(nextScoreHighEnough)
            
            // Stop looking once we find a non-session-only face, whether or not
            // we used it
            break;
          }
          
          ++nextIndex;
          
        } while(nextMatchingID == matchingID && nextIndex < resultNum);
        
      } // if(resultNum >= 2)
      
      faceID = matchingID;
      recognitionScore = matchingScore;
      
      if(shouldUpdateAlbumEntry)
      {
        Result result = UpdateExistingAlbumEntry(matchingAlbumEntries[matchIndex],
                                                 _okaoRecognitionFeatureHandle,
                                                 recognitionScore);
        if(RESULT_OK != result) {
          PRINT_NAMED_WARNING("FaceRecognizer.RecognizeFace.UpdatingExistingAlbumEntryFailed",
                              "albumEntry:%d", matchingAlbumEntries[matchIndex]);
        }
      }
      
      if(_enrollmentID == faceID && _enrollmentTrackID != _detectionInfo.nID)
      {
        // We recognized the current track as the face ID we're enrolling,
        // update the track ID to match if it has changed.
        PRINT_CH_INFO(LOG_CHANNEL, "RecognizeFace.SetEnrollmentTrackID",
                      "EnrollmentID:%d EnrollmentTrackID:%d (was %d)",
                      _enrollmentID, -_detectionInfo.nID, -_enrollmentTrackID);
        _enrollmentTrackID = _detectionInfo.nID;
      }
      
    } // if(foundMatchAboveThreshold)
    
    else if(_isEnrollmentEnabled && haveEnrollmentCountsLeft)
    {
      // Yes, enrollSpecificFace is just the opposite of canEnrollAnyFace, but
      // I'm doing this for better readability of the concept.
      const bool enrollingSpecificFace  = (_enrollmentID != UnknownFaceID);
      const bool isTrackingEnrollmentID = (_detectionInfo.nID == _enrollmentTrackID);
      
      if(enrollingSpecificFace && isTrackingEnrollmentID)
      {
        DEV_ASSERT(_detectionInfo.nID != UnknownFaceID, "FaceRecognizer.RecognizeFace.BadDetectionID");
        
        // Did not recognize the current enrollmentID in the image, but the trackingID matches
        // the enrollment tracking ID. Update using the match score for _enrollmentID.
        recognitionScore = 0;
        for(INT32 iResult=0; iResult<resultNum; ++iResult)
        {
          if(GetFaceIDforAlbumEntry(matchingAlbumEntries[iResult]) == _enrollmentID)
          {
            recognitionScore = scores[iResult];
            Result result = UpdateExistingAlbumEntry(matchingAlbumEntries[iResult],
                                                     _okaoRecognitionFeatureHandle,
                                                     recognitionScore);
            if(RESULT_OK != result) {
              return result;
            }
            
            PRINT_CH_INFO(LOG_CHANNEL, "RecognizeFace.UpdatingEnrollmentFace",
                          "Did not recognize enrollmentID:%d, updating with score %d based on enrolledTrackID:%d",
                          _enrollmentID, recognitionScore, _enrollmentTrackID);
            
            break;
          }
        }
        
        faceID = _enrollmentID;
      }
      else if(canEnrollAnyFace)
      {
        // If the score with which we matched something in the album isn't "too close" to
        // (i.e. within specified marghin of) an entry already in the album, then add
        // this as a new user. The margin is meant to keep multiple entries from being "too"
        // similar, since that is apparently not great for OKAO.
        if(resultNum == 0 || scores[0] < kFaceRecognitionThreshold-kFaceRecognitionThresholdMarginForAdding)
        {
          PRINT_CH_INFO(LOG_CHANNEL, "RecognizeFace.AddingNewUser",
                        "Observed new person. Adding to album.");
          Result lastResult = RegisterNewUser(_okaoRecognitionFeatureHandle, faceID);
          if(RESULT_OK != lastResult) {
            return lastResult;
          }
          recognitionScore = kMaxScore;
        }
      }
    } // else if(_isEnrollmentEnabled && haveEnrollmentCountsLeft)
    
    return RESULT_OK;
    
  } // RecognizeFace()

  bool FaceRecognizer::IsMergingAllowed(FaceID_t toFaceID) const
  {
    const bool enrollingThisFace   = (_enrollmentID == toFaceID);
    const bool enrollingAnyFace    = (_enrollmentID == UnknownFaceID);
    const bool haveEnrollmentsLeft = (_enrollmentCount != 0);
    
		const bool isAllowed = (_isEnrollmentEnabled &&
                            haveEnrollmentsLeft &&
                            (enrollingThisFace || enrollingAnyFace));
    
  	return isAllowed;
  }
  
  Result FaceRecognizer::MergeFaces(FaceID_t keepID, FaceID_t mergeID)
  {
    if(keepID == mergeID)
    {
      PRINT_CH_INFO(LOG_CHANNEL, "FaceRecognizer.MergeFaces.NothingToDo",
                    "keepID=mergeID=%d", keepID);
      return RESULT_OK;
    }
    
    // Look up enrollment data for keepID and mergeID
    auto keepIter = _enrollmentData.find(keepID);
    if(keepIter == _enrollmentData.end()) {
      PRINT_NAMED_WARNING("FaceRecognizer.MergeFaces.MissingEnrollDataForKeepID",
                          "KeepID:%d", keepID);
      return RESULT_FAIL;
    }
    
    auto mergeIter = _enrollmentData.find(mergeID);
    if(mergeIter == _enrollmentData.end()) {
      PRINT_NAMED_WARNING("FaceRecognizer.MergeFaces.MissingEnrollDataForMergeID",
                          "MergeID:%d", mergeID);
      return RESULT_FAIL;
    }
    
    if(kEnableMergingOfSessionOnlyAlbumEntries &&
       mergeIter->second.GetSessionOnlyAlbumEntry() >= 0 &&
       keepIter->second.GetSessionOnlyAlbumEntry() >= 0)
    {
      INT32 numSessionOnlyMergeData = 0;
      OKAO_FR_GetRegisteredUsrDataNum(_okaoFaceAlbum, mergeIter->second.GetSessionOnlyAlbumEntry(), &numSessionOnlyMergeData);
      for(s32 iMergeFeature=0; iMergeFeature < numSessionOnlyMergeData; ++iMergeFeature)
      {
        OKAO_FR_GetFeatureFromAlbum(_okaoFaceAlbum, mergeIter->second.GetSessionOnlyAlbumEntry(), iMergeFeature, _okaoRecogMergeFeatureHandle);
        
        UpdateExistingAlbumEntry(keepIter->second.GetSessionOnlyAlbumEntry(), _okaoRecogMergeFeatureHandle,
                                 mergeIter->second.GetScore());
      }
    }
    
    // Tell the EnrolledFaceEntry for keepID to merge with the one from mergeID
    std::vector<AlbumEntryID_t> entriesRemovedFromKeep;
    Result mergeResult = keepIter->second.MergeWith(mergeIter->second, kMaxAlbumEntriesPerFace, entriesRemovedFromKeep);
    if(RESULT_OK != mergeResult) {
      PRINT_NAMED_WARNING("FaceRecognizer.MergeFaces.MergeFailed",
                          "Merging faceID:%d with %d",
                          keepIter->first, mergeIter->first);
      return mergeResult;
    }
    
    // Fix the albumEntryToFaceID bookkeeping now that keepID's albumEntries have changed
    for(auto & albumEntryPair : keepIter->second.GetAlbumEntries())
    {
      _albumEntryToFaceID[albumEntryPair.first] = keepID;
    }
    
    for(AlbumEntryID_t removedEntry : entriesRemovedFromKeep)
    {
      _albumEntryToFaceID.erase(removedEntry);
      OkaoResult okaoResult = OKAO_FR_ClearUser(_okaoFaceAlbum, removedEntry);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_WARNING("FaceRecognizer.MergeFaces.ClearUserFailed",
                            "AlbumEntry:%d originally part of keepID:%d",
                            removedEntry, keepID);
        return RESULT_FAIL;
      }
    }
    
    // "Unregister" all album entries still left associated with mergeID (since they
    // were not taken by keepID above).
    for(auto & albumEntryPair : mergeIter->second.GetAlbumEntries())
    {
      // If this entry is not now claimed by keepID, we should clear it because we're about
      // to clear its album entries. Note that we must do this before calling RemoveUser
      // below because it would otherwise undo the reassignment to keepID above
      if(_albumEntryToFaceID.at(albumEntryPair.first) == mergeID)
      {
        _albumEntryToFaceID.erase(albumEntryPair.first);
        OkaoResult okaoResult = OKAO_FR_ClearUser(_okaoFaceAlbum, albumEntryPair.first);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_WARNING("FaceRecognizer.MergeFaces.ClearUserFailed",
                              "AlbumEntry:%d from mergeID:%d",
                              albumEntryPair.first, mergeID);
          return RESULT_FAIL;
        }
      }
    }
    mergeIter->second.ClearAlbumEntries();
    
    // After merging it, remove the old user that just got merged
    Result removeResult = RemoveUser(mergeID);
    
    if(kFaceRecognitionExtraDebug) {
      PRINT_CH_INFO(LOG_CHANNEL, "MergeFaces.Success",
                    "Merged FaceID %d into %d. Kept %zu album entries.",
                    mergeID, keepID, keepIter->second.GetAlbumEntries().size());
    }
    
    return removeResult;
    
  } // MergeFaces()
  
  Result FaceRecognizer::GetSerializedAlbum(std::vector<u8>& serializedAlbum) const
  {
    if(!_isInitialized) {
      PRINT_NAMED_WARNING("FaceRecognizer.GetSerializedAlbum.NotInitialized", "");
      return RESULT_FAIL;
    }

    HALBUM  tempAlbumHandle  = NULL;
    
    // Make sure temp album gets cleaned up in case of early returns below
    Util::CleanupHelper cleanup([&tempAlbumHandle](){
      if(NULL != tempAlbumHandle) {
        if(OKAO_NORMAL != OKAO_FR_ClearAlbum(tempAlbumHandle)) {
          PRINT_NAMED_WARNING("FaceRecognizer.GetSerializedAlbum.ClearTempAlbumFail", "");
        } else if(OKAO_NORMAL != OKAO_FR_DeleteAlbumHandle(tempAlbumHandle)) {
          PRINT_NAMED_WARNING("FaceRecognizer.GetSerializedAlbum.DeleteTempAlbumFail", "");
        }
      }
    });
    
    OkaoResult okaoResult = OKAO_NORMAL;
    
    tempAlbumHandle = OKAO_FR_CreateAlbumHandle(_okaoCommonHandle,
                                                kMaxTotalAlbumEntries,
                                                kMaxEnrollDataPerAlbumEntry);
    if(tempAlbumHandle == NULL) {
      PRINT_NAMED_WARNING("FaceRecognizer.GetSerializedAlbum.AlbumHandleFail",
                          "Could not create temporary album handle for serializing.");
      return RESULT_FAIL;
    }
    
    s32 permanentIdCount = 0;
    for(auto const& enrollData : _enrollmentData)
    {
      if(permanentIdCount >= kMaxFacesInAlbum)
      {
        PRINT_NAMED_WARNING("FaceRecognizer.GetSerializedAlbum.MaxNumFacesReached",
                            "Can't save more than %d faces",
                            kMaxFacesInAlbum);
        break;
      }
      
      const auto faceID = enrollData.first;
      if(enrollData.second.IsForThisSessionOnly() == false)
      {
        for(auto & albumEntryPair : enrollData.second.GetAlbumEntries())
        {
          const AlbumEntryID_t albumEntry = albumEntryPair.first;
          
          for(s32 iData=0; iData<kMaxEnrollDataPerAlbumEntry; ++iData)
          {
            BOOL isRegistered = false;
            okaoResult = OKAO_FR_IsRegistered(_okaoFaceAlbum, albumEntry, iData, &isRegistered);
            if(OKAO_NORMAL != okaoResult) {
              PRINT_NAMED_WARNING("FaceRecognizer.GetSerializedAlbum.IsRegisteredFail",
                                  "Could not get registration status for face:%d albumEntry:%d data:%d",
                                  faceID, albumEntry, iData);
              return RESULT_FAIL;
            }
            
            if(isRegistered)
            {
              // Copy this user's registered feature data to temp album for serialization
              okaoResult = OKAO_FR_GetFeatureFromAlbum(_okaoFaceAlbum, albumEntry, iData, _okaoRecognitionFeatureHandle);
              if(OKAO_NORMAL != okaoResult) {
                PRINT_NAMED_WARNING("FaceRecognizer.GetSerializedAlbum.GetFeatureFail",
                                    "Could not get feature for face:%d albumEntry:%d data:%d",
                                    faceID, albumEntry, iData);
                return RESULT_FAIL;
              }
              
              okaoResult = OKAO_FR_RegisterData(tempAlbumHandle, _okaoRecognitionFeatureHandle, albumEntry, iData);
              if(OKAO_NORMAL != okaoResult) {
                PRINT_NAMED_WARNING("FaceRecognizer.GetSerializedAlbum.IsRegisteredFail",
                                    "Could not register temp feature for face:%d albumEntry:%d data:%d",
                                    faceID, albumEntry, iData);
                return RESULT_FAIL;
              }
              
            } // if isRegistered
          } // for each data
        }
        ++permanentIdCount;
        
      } // if enrollData isForThisSessionOnly == false
        
    } // for each enrollData
    
    
    UINT32 albumSize = 0;
    if(permanentIdCount > 0)
    {
      okaoResult = OKAO_FR_GetSerializedAlbumSize(tempAlbumHandle, &albumSize);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_WARNING("FaceRecognizer.GetSerializedAlbum.GetSizeFail",
                            "FaceLib Result=%d", okaoResult);
        return RESULT_FAIL;
      }
      
      serializedAlbum.resize(albumSize);
      okaoResult = OKAO_FR_SerializeAlbum(tempAlbumHandle, &(serializedAlbum[0]), albumSize);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_WARNING("FaceRecognizer.GetSerializedAlbum.SerializeFail",
                            "FaceLib Result=%d", okaoResult);
        return RESULT_FAIL;
      }

      PRINT_CH_INFO(LOG_CHANNEL, "GetSerializedAlbum.AlbumSize",
                    "Album with %d permanent faces = %d bytes (%.1f bytes/person)",
                    permanentIdCount, albumSize, (f32)albumSize/(f32)permanentIdCount);
    
    } // if(permanentIdCount == 0)
    else
    {
      PRINT_CH_INFO(LOG_CHANNEL, "GetSerializedAlbum.NoNamedIDs", "");
    }
  
    return RESULT_OK;
  } // GetSerializedAlbum()
  
  
  Result FaceRecognizer::SetSerializedAlbum(HCOMMON okaoCommonHandle, const std::vector<u8>&serializedAlbum, HALBUM& album)
  {
    if(NULL == okaoCommonHandle) {
      PRINT_NAMED_ERROR("FaceRecognizer.SetSerializedAlbum.NullFaceLibCommonHandle", "");
      return RESULT_FAIL;
    }
    
    if(serializedAlbum.empty())
    {
      PRINT_NAMED_WARNING("FaceRecognizer.SetSerializedAlbum.EmptyAlbumData", "");
      return RESULT_FAIL;
    }
    
    FR_ERROR error;
    album = OKAO_FR_RestoreAlbum(okaoCommonHandle, const_cast<UINT8*>(serializedAlbum.data()), (UINT32)serializedAlbum.size(), &error);
    if(NULL == album) {
      PRINT_NAMED_WARNING("FaceRecognizer.SetSerializedAlbum.RestoreFail",
                          "FaceLib Result=%d", error);
      return RESULT_FAIL;
    }
    
    INT32 numAlbumEntries=0;
    OkaoResult okaoResult = OKAO_FR_GetRegisteredUserNum(album, &numAlbumEntries);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_WARNING("FaceRecognizer.SetSerializedAlbum.GetNumEntriesFailed",
                          "FaceLib result=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    INT32 numDataEntries = 0;
    okaoResult = OKAO_FR_GetRegisteredAllDataNum(album, &numDataEntries);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_WARNING("FaceRecognizer.SetSerializedAlbum.GetNumDataFailed",
                          "FaceLib result=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    PRINT_CH_INFO(LOG_CHANNEL, "SetSerializedAlbum.RestoredAlbum",
                  "Restored FaceLib album with %d album entries and %d data entries from %zu-byte serialized album",
                  numAlbumEntries, numDataEntries, serializedAlbum.size());
   
    return RESULT_OK;
  } // SetSerializedAlbum()
  
  
  Result FaceRecognizer::AssignNameToID(FaceID_t faceID, const std::string& name, FaceID_t mergeWithID)
  {
    auto iterToRename = _enrollmentData.end();
    
    if(mergeWithID != UnknownFaceID)
    {
      // We should only merge into a named ID
      iterToRename = _enrollmentData.find(mergeWithID);
      
      if(iterToRename == _enrollmentData.end())
      {
        PRINT_NAMED_WARNING("FaceRecognizer.AssignNameToID.InvalidMergeWithID",
                            "No enrollment data for mergeWithID=%d", mergeWithID);
        return RESULT_FAIL;
      }
      
      if(iterToRename->second.IsForThisSessionOnly())
      {
        PRINT_NAMED_WARNING("FaceRecognizer.AssignNameToID.SessionOnlyMergeWithID",
                            "MergeWithID must be for a named face, not session-only");
        return RESULT_FAIL;
      }
      
      // Merge the two records, keeping mergeWithID, but use the name of the new one,
      // since that's what was just assigned
      Result lastResult = MergeFaces(mergeWithID, faceID);
      if(RESULT_OK != lastResult) {
        PRINT_NAMED_WARNING("FaceRecognizer.AssignNameToID.MergeFailed",
                            "Merging faceID:%d into mergeWithID:%d",
                            faceID, mergeWithID);
        return lastResult;
      }
    }
    else
    {
      iterToRename = _enrollmentData.find(faceID);
    }
    
    if(iterToRename != _enrollmentData.end())
    {
      if(DEBUG_ENROLLMENT_IMAGES)
      {
        // Close old display window before we rename, since it's matched by name
        Vision::Image::CloseDisplayWindow(GetDisplayName(iterToRename->second).c_str());
      }
      
      iterToRename->second.SetName( name );
    }
    else
    {
      PRINT_NAMED_WARNING("FaceRecognizer.AssignNameToID.InvalidID",
                          "Unknown ID %d, ignoring name %s",
                          faceID, Util::HidePersonallyIdentifiableInfo(name.c_str()));
      
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
    
  } // AssignNameToID()
  
  
  Result FaceRecognizer::EraseFace(FaceID_t faceID)
  {
     auto enrollIter = _enrollmentData.find(faceID);
     if(enrollIter != _enrollmentData.end())
     {
       Result removeResult = RemoveUser(enrollIter->first);
       if(RESULT_OK != removeResult) {
         PRINT_NAMED_WARNING("FaceRecognizer.EraseName.RemoveUserFailed", "ID=%d", enrollIter->first);
       }
       return removeResult;
     }
     else {
       PRINT_NAMED_WARNING("FaceRecognizer.EraseFace.NotFound",
                           "Did not find a record for ID=%d", faceID);
       return RESULT_FAIL;
     }
  } // EraseFace()

  std::vector<Vision::LoadedKnownFace> FaceRecognizer::GetEnrolledNames() const
  {
    std::vector<LoadedKnownFace> ret;
    ret.reserve( _enrollmentData.size() );
    auto const nowTime = std::chrono::system_clock::now();
    for( const auto& entry : _enrollmentData ) {
      if( !entry.second.GetName().empty() ) {
        ret.emplace_back( LoadedKnownFace{GetSecondsSince(nowTime, entry.second.GetEnrollmentTime()),
                                          GetSecondsSince(nowTime, entry.second.GetLastUpdateTime()),
                                          GetSecondsSince(nowTime, entry.second.FindLastSeenTime()),
                                          entry.second.GetFaceID(),
                                          entry.second.GetName()} );
      }
    }
    return ret;
  }
  
  void FaceRecognizer::EraseAllFaces()
  {
    // Remove each user one at a time, to make sure all the cleanup gets done
    // (Like dissociating tracker IDs)
    for(auto enrollIter=_enrollmentData.begin(); enrollIter!=_enrollmentData.end(); )
    {
      enrollIter = RemoveUser(enrollIter);
    }
    
    // These should not be necessary since all the RemoveUser calls should have done it, but...
    _enrollmentData.clear();
    _albumEntryToFaceID.clear();
    OkaoResult okaoResult = OKAO_FR_ClearAlbum(_okaoFaceAlbum);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_WARNING("FaceRecognizer.EraseAllFaces.FaceLibClearAlbumFailed",
                          "FaceLib Result=%d", okaoResult);
    }
    
    PRINT_CH_INFO(LOG_CHANNEL, "EraseAllFaces.Complete", "");
    
  } // EraseAllFaces()
  
  Result FaceRecognizer::RenameFace(FaceID_t faceID, const std::string& oldName, const std::string& newName,
                                    Vision::RobotRenamedEnrolledFace& renamedFace)
  {
    auto enrollIter = _enrollmentData.find(faceID);
    if(enrollIter != _enrollmentData.end())
    {
      if(enrollIter->second.GetName() == oldName)
      {
        enrollIter->second.SetName( newName );
        
        PRINT_CH_INFO("FaceRecognizer", "RenameFace.Success", "Renamed ID=%d from '%s' to '%s'",
                      faceID, Util::HidePersonallyIdentifiableInfo(oldName.c_str()),
                      Util::HidePersonallyIdentifiableInfo(enrollIter->second.GetName().c_str()));
        
        // Construct and then swap to make sure we miss fields that get added to LoadedKnownFace later
        Vision::RobotRenamedEnrolledFace temp(enrollIter->second.GetFaceID(),
                                              enrollIter->second.GetName());
        std::swap(temp, renamedFace);
        
        return RESULT_OK;
      }
      else
      {
        PRINT_NAMED_WARNING("FaceRecognizer.RenameFace.OldNameMismatch",
                            "OldName '%s' does not match stored name '%s' for ID=%d",
                            Util::HidePersonallyIdentifiableInfo(oldName.c_str()),
                            Util::HidePersonallyIdentifiableInfo(enrollIter->second.GetName().c_str()),
                            faceID);
        
        return RESULT_FAIL;
      }
    }
    else
    {
      PRINT_NAMED_WARNING("FaceRecognizer.RenameFace.InvalidID",
                          "No record for ID=%d", faceID);
      return RESULT_FAIL;
    }
  }
  
  Result FaceRecognizer::GetSerializedData(std::vector<u8>& albumData,
                                           std::vector<u8>& enrollData)
  {
    Result lastResult = GetSerializedAlbum(albumData);
    
    if(RESULT_OK != lastResult) {
      PRINT_NAMED_WARNING("FaceRecognizer.GetSerializedData.GetSerializedAlbumFail", "");
    } else if(!albumData.empty()) {
      lastResult = GetSerializedEnrollData(enrollData);
      if(RESULT_OK != lastResult) {
        PRINT_NAMED_WARNING("FaceRecognizer.GetSerializedData.GetSerializedAlbumFail", "");
      }
    }
    
    return lastResult;
  }
  
  Result FaceRecognizer::SetSerializedData(const std::vector<u8>& albumData,
                                           const std::vector<u8>& enrollData,
                                           std::list<LoadedKnownFace>& loadedFaces)
  {
    if(NULL == _okaoCommonHandle) {
      PRINT_NAMED_ERROR("FaceRecognizer.SetSerializedData.NullFaceLibCommonHandle", "");
      return RESULT_FAIL;
    }
    
    // Note that we load into these temporary data structures and swap them into the
    // real ones later
    HALBUM loadedAlbum = NULL;
    EnrollmentData loadedEnrollmentData;
    
    Result lastResult = SetSerializedAlbum(_okaoCommonHandle, albumData, loadedAlbum);
    
    if(RESULT_OK != lastResult) {
      PRINT_NAMED_WARNING("FaceRecognizer.SetSerializedData.SetSerializedAlbumFail", "");
    } else {
      FaceID_t loadedNextFaceID = UnknownFaceID;
      lastResult = SetSerializedEnrollData(enrollData, loadedEnrollmentData, loadedNextFaceID);
      if(RESULT_OK != lastResult) {
        PRINT_NAMED_WARNING("FaceRecognizer.SetSerializedData.SetSerializedEnrollDataFail", "");
      } else {
        
        lastResult = UseLoadedAlbumAndEnrollData(loadedAlbum, loadedEnrollmentData);

        if(RESULT_OK == lastResult)
        {
          PRINT_CH_INFO(LOG_CHANNEL, "SetSerializedData.NewNextFaceID",
                        "Setting next FaceID=%d", loadedNextFaceID);
          
          _nextFaceID = loadedNextFaceID;
          
          auto const nowTime = std::chrono::system_clock::now();
          
          for(auto & entry : _enrollmentData)
          {
            const auto & faceID = entry.second.GetFaceID();
            const auto & faceName = entry.second.GetName();
            
            // Log the ID and num of album entries (as DDATA) of each entry we load
            const size_t numAlbumEntries = entry.second.GetAlbumEntries().size();
            Util::sEventF("robot.vision.loaded_face_enrollment_entry",
                          {{DDATA, std::to_string(numAlbumEntries).c_str()}},
                          "%d", faceID);
            
            const s64 secSinceEnrolled = GetSecondsSince(nowTime, entry.second.GetEnrollmentTime());
            const s64 secSinceUpdated  = GetSecondsSince(nowTime, entry.second.GetLastUpdateTime());
            const s64 secSinceSeen     = GetSecondsSince(nowTime, entry.second.FindLastSeenTime());
            
            loadedFaces.emplace_back(LoadedKnownFace(secSinceEnrolled, secSinceUpdated, secSinceSeen,
                                                     faceID,
                                                     faceName));
            
            PRINT_CH_INFO(LOG_CHANNEL, "SetSerializedData.AddedEnrollmentDataEntry",
                          "User '%s' with ID=%d. Seconds since: Enrolled=%lld Updated=%lld Seen=%lld",
                          Util::HidePersonallyIdentifiableInfo(faceName.c_str()),
                          faceID, secSinceEnrolled, secSinceUpdated, secSinceSeen);
          }
        }
      }
    }

    //
    // UseLoadedAlbumAndEnrollData() may decide to keep using an existing album handle or the one we just created.
    // Either way, we need to clean up the handle that is NOT being kept open.
    //
    if (loadedAlbum != NULL) {
      PRINT_CH_DEBUG(LOG_CHANNEL, "FaceRecognizer.SetSerializedData.DeleteAlbumHandle",
                     "Delete album handle %p", loadedAlbum);
      DEV_ASSERT(loadedAlbum != _okaoFaceAlbum, "FaceRecognizer.SetSerializedData.InvalidAlbumHandle");
      OKAO_FR_DeleteAlbumHandle(loadedAlbum);
      loadedAlbum = NULL;
    }
    
    return lastResult;
  }
  
  Result FaceRecognizer::UseLoadedAlbumAndEnrollData(HALBUM& loadedAlbumData,
                                                     EnrollmentData& loadedEnrollmentData)
  {
    // Populate albumToFaceID LUT using loaded enrollment data
    AlbumEntryToFaceID loadedAlbumEntryToFaceID;
    for(auto & enrollData : loadedEnrollmentData)
    {
      for(auto & albumEntry : enrollData.second.GetAlbumEntries())
      {
        loadedAlbumEntryToFaceID[albumEntry.first] = enrollData.first;
      }
    }
    
    Result lastResult = SanityCheckBookkeeping(loadedAlbumData,
                                               loadedEnrollmentData,
                                               loadedAlbumEntryToFaceID);
    
    if(lastResult == RESULT_OK)
    {
      // Only if sanity checks pass do we actually swap in the loaded album/enrollment data
      INT32 currentMaxAlbumEntries=0, loadedMaxAlbumEntries=0;
      INT32 currentMaxDataEntries=0,  loadedMaxDataEntries=0;
      
      OkaoResult okaoResult = OKAO_FR_GetAlbumMaxNum(_okaoFaceAlbum, &currentMaxAlbumEntries, &currentMaxDataEntries);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_WARNING("FaceRecognizer.UseLoadedAlbumAndEnrollData.GetCurrentMaxNumFailed",
                            "FaceLib Result=%d", okaoResult);
        return RESULT_FAIL;
      }
      
      okaoResult = OKAO_FR_GetAlbumMaxNum(loadedAlbumData, &loadedMaxAlbumEntries, &loadedMaxDataEntries);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_WARNING("FaceRecognizer.UseLoadedAlbumAndEnrollData.GetLoadedMaxNumFailed",
                            "FaceLib Result=%d", okaoResult);
        return RESULT_FAIL;
      }
      
      if(loadedMaxAlbumEntries > currentMaxAlbumEntries ||  loadedMaxDataEntries > currentMaxDataEntries)
      {
        PRINT_NAMED_WARNING("FaceRecognizer.UseLoadedAlbumAndEnrollmentData.LoadedMaxNumTooLarge",
                            "Loaded album too large (maxAlbumEntries:%d maxDataEntries:%d) too large "
                            "for current settings (%d and %d)",
                            loadedMaxAlbumEntries, loadedMaxDataEntries,
                            currentMaxAlbumEntries, currentMaxDataEntries);
        
        return RESULT_FAIL;
      }
      
      if(loadedMaxAlbumEntries == currentMaxAlbumEntries && loadedMaxDataEntries == currentMaxDataEntries)
      {
        // Simple case: just swap in the new album
        PRINT_CH_DEBUG(LOG_CHANNEL, "FaceRecognizer.UseLoadedAlbumAndEnrollmentData.Swap",
                       "Stop using %p, start using %p",
                       _okaoFaceAlbum, loadedAlbumData);
        std::swap(loadedAlbumData, _okaoFaceAlbum);
      }
      else
      {
        // No way to update an album's max sizes, so copy all data over manually,
        // one at a time. We've already verified that the current album is large
        // enough in both dimensions (album entries and data entries)
        PRINT_CH_DEBUG(LOG_CHANNEL, "FaceRecognizer.UseLoadedAlbumAndEnrollmentData.Update",
                       "Keep using %p, copy data from %p",
                       _okaoFaceAlbum, loadedAlbumData);

        okaoResult = OKAO_FR_ClearAlbum(_okaoFaceAlbum);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_WARNING("FaceRecognizer.UserLoadedAlbumAndEnrollmentData.ClearAlbumFailed",
                              "FaceLib Result=%d", okaoResult);
          return RESULT_FAIL;
        }
        
        PRINT_CH_INFO(LOG_CHANNEL, "UseLoadedAlbumAndEnrollmentData.ManualLoadDueToDifferingMaxNums",
                      "Loaded album has smaller max album/data entries than current settings (%d/%d vs. %d/%d) "
                      "Loading each manually.",
                      loadedMaxAlbumEntries, loadedMaxDataEntries,
                      currentMaxAlbumEntries, currentMaxDataEntries);
        
        for(AlbumEntryID_t iAlbumEntry=0; iAlbumEntry<loadedMaxAlbumEntries; ++iAlbumEntry)
        {
          for(INT32 iData=0; iData<loadedMaxDataEntries; ++iData)
          {
            BOOL isRegistered = false;
            okaoResult = OKAO_FR_IsRegistered(loadedAlbumData, iAlbumEntry, iData, &isRegistered);
            if(OKAO_NORMAL != okaoResult) {
              PRINT_NAMED_WARNING("FaceRecognizer.UserLoadedAlbumAndEnrollmentData.IsRegisteredFailed",
                                  "AlbumEntry:%d DataEntry:%d FaceLib Result=%d",
                                  iAlbumEntry, iData, okaoResult);
              return RESULT_FAIL;
            }
            
            if(isRegistered)
            {
              okaoResult = OKAO_FR_GetFeatureFromAlbum(loadedAlbumData, iAlbumEntry, iData, _okaoRecogMergeFeatureHandle);
              if(OKAO_NORMAL != okaoResult) {
                PRINT_NAMED_WARNING("FaceRecognizer.UserLoadedAlbumAndEnrollmentData.GetFeatureFailed",
                                    "AlbumEntry:%d DataEntry:%d FaceLib Result=%d",
                                    iAlbumEntry, iData, okaoResult);
                return RESULT_FAIL;
              }
              
              okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, _okaoRecogMergeFeatureHandle, iAlbumEntry, iData);
              if(OKAO_NORMAL != okaoResult) {
                PRINT_NAMED_WARNING("FaceRecognizer.UserLoadedAlbumAndEnrollmentData.RegisterDataFailed",
                                    "AlbumEntry:%d DataEntry:%d FaceLib Result=%d",
                                    iAlbumEntry, iData, okaoResult);
                return RESULT_FAIL;
              }
              
            }
            
          }
        }
      }
      
      std::swap(loadedEnrollmentData, _enrollmentData);
      std::swap(loadedAlbumEntryToFaceID, _albumEntryToFaceID);
      
      _trackingToFaceID.clear();
      
      PRINT_CH_INFO(LOG_CHANNEL, "UseLoadedAlbumAndEnrollData.Success",
                    "Loaded album and enroll data passed sanity checks (%zu entries)",
                    _enrollmentData.size());
    }

    return lastResult;
  }

  
  Result FaceRecognizer::GetSerializedEnrollData(std::vector<u8>& serializedEnrollData)
  {
    // Put version prefix at start: 4 bytes = [0xFA 0xCE XX XX], as in "FACEXXXX"
    serializedEnrollData.clear();
    
    if(!_enrollmentData.empty())
    {
      const u8 * VersionPrefixU8 = (u8*)VersionPrefix;
      std::copy(VersionPrefixU8, VersionPrefixU8+4, std::back_inserter(serializedEnrollData));
      
      DEV_ASSERT(((u32*)serializedEnrollData.data())[0] == ((u32*)VersionPrefix)[0],
                 "FaceRecognizer.GetSerialzedEnrollData.AddingVersionPrefixFailed");
      
      PRINT_CH_INFO(LOG_CHANNEL, "GetSerializedEnrollData.AddedVersionData",
                    "Added to front: %04X%04X",
                    ((u16*)serializedEnrollData.data())[0],
                    ((u16*)serializedEnrollData.data())[1]);
      
      // Put the next faceID in first, so we can start there next time and avoid
      // re-using IDs, since that is better for DAS
      const u8* faceIdU8 = (u8*)(&_nextFaceID);
      std::copy(faceIdU8, faceIdU8+sizeof(FaceID_t), std::back_inserter(serializedEnrollData));
      
      for(auto & enrollData : _enrollmentData)
      {
        // Skip session-only entries
        if(false == enrollData.second.IsForThisSessionOnly()) {
          enrollData.second.Serialize(serializedEnrollData);
        }
      }
    }
    
    return RESULT_OK;
  }
  
  Result FaceRecognizer::SetSerializedEnrollData(const std::vector<u8>& serializedEnrollData,
                                                 EnrollmentData& newEnrollmentData,
                                                 FaceID_t& newNextFaceID)
  {
    // Check for correct version prefix
    const size_t kMinSize = sizeof(VersionPrefix) + sizeof(FaceID_t);
    if(serializedEnrollData.size() < kMinSize) {
      PRINT_NAMED_WARNING("FaceRecognizer.SetSerializedEnrollData.TooShortForVersion",
                          "Data is not even %zu bytes long to read version and nextFaceID",
                          kMinSize);
      return RESULT_FAIL;
    }
    
    const u32 incomingVersion = ((u32*)serializedEnrollData.data())[0];
    const u32 correctVersion  = ((u32*)VersionPrefix)[0];
    
    if(incomingVersion != correctVersion)
    {
      PRINT_NAMED_WARNING("FaceRecognizer.SetSerializedEnrollData.VersionPrefixMismatch",
                          "Expected: %04X%04X, Incoming: %04X%04X",
                          VersionPrefix[0], VersionPrefix[1],
                          ((u16*)serializedEnrollData.data())[0],
                          ((u16*)serializedEnrollData.data())[1]);
      
      // TODO: Provide helpers to "upgrade" old versions?
      
      return RESULT_FAIL;
    }
    
    PRINT_CH_INFO(LOG_CHANNEL, "SetSerializedEnrollData.MatchedVersionPrefix",
                  "Got correct prefix: %04X%04X", VersionPrefix[0], VersionPrefix[1]);
    
    EnrolledFaceEntry entry;
    
    size_t startIndex = sizeof(VersionPrefix); // start after the prefix
    
    // Grab the starting faceID from the start of the buffer
    newNextFaceID = ((FaceID_t*)(serializedEnrollData.data()+startIndex))[0];
    
    startIndex += sizeof(FaceID_t);
    
    while(startIndex < serializedEnrollData.size()-3) // "-3" to handle an extra bytes of padding
    {
      Result lastResult = entry.Deserialize(serializedEnrollData, startIndex);
      if(RESULT_OK != lastResult) {
        PRINT_NAMED_WARNING("FaceRecognizer.SetSerializedEnrollData.DeserializeFail",
                            "Failed to deserialize EnrolledFaceEntry at start=%zu (from %zu-length buffer)",
                            startIndex, serializedEnrollData.size());
        return RESULT_FAIL;
      }
      
      newEnrollmentData[entry.GetFaceID()] = entry;
    }
    
    PRINT_CH_INFO(LOG_CHANNEL, "SetSerializedEnrollData.Complete",
                  "Deserialized %zu entries from buffer", newEnrollmentData.size());
    
    return RESULT_OK;
  }
  
  Result FaceRecognizer::SaveAlbum(const std::string &albumName)
  {
    Result result = RESULT_OK;
    
    std::vector<u8> serializedAlbum;
    result = GetSerializedAlbum(serializedAlbum);
    if(RESULT_OK != result) {
      return result;
    }

    if(serializedAlbum.empty()) {
      PRINT_NAMED_WARNING("FaceRecognizer.SaveAlbum.EmptyAlbum",
                          "No serialized data returned from private implementation");
      result = RESULT_FAIL;
    } else {
      
      if(false == Util::FileUtils::CreateDirectory(albumName, false, true)) {
        PRINT_NAMED_WARNING("FaceRecognizer.SaveAlbum.DirCreationFail",
                            "Tried to create: %s", albumName.c_str());
        result = RESULT_FAIL;
      } else {
        
        const std::string dataFilename(albumName + "/data.bin");
        std::fstream fs;
        fs.open(dataFilename, std::ios::binary | std::ios::out);
        if(!fs.is_open()) {
          PRINT_NAMED_WARNING("FaceRecognizer.SaveAlbum.FileOpenFail", "Filename: %s", dataFilename.c_str());
          result = RESULT_FAIL;
        } else {
          
          fs.write((const char*)&(serializedAlbum[0]), serializedAlbum.size());
          fs.close();
          
          if((fs.rdstate() & std::ios::badbit) || (fs.rdstate() & std::ios::failbit)) {
            PRINT_NAMED_WARNING("FaceRecognizer.SaveAlbum.FileWriteFail", "Filename: %s", dataFilename.c_str());
            result = RESULT_FAIL;
          } else {
            Json::Value json;
            for(auto & enrollData : _enrollmentData)
            {
              if(false == enrollData.second.IsForThisSessionOnly())
              {
                Json::Value entry;
                enrollData.second.FillJson(entry);
                json[std::to_string(enrollData.first)] = std::move(entry);
              }
            }
            
            const std::string enrollDataFilename(albumName + "/enrollData.json");
            Json::FastWriter writer;
            fs.open(enrollDataFilename, std::ios::out);
            if (!fs.is_open()) {
              PRINT_NAMED_WARNING("FaceRecognizer.SaveAlbum.EnrollDataFileOpenFail", "");
              result = RESULT_FAIL;
            } else {
              fs << writer.write(json);
              fs.close();
            }
          }
        }
      }
    }
    
    return result;
  } // SaveAlbum()
  
  
  Result FaceRecognizer::LoadAlbum(const std::string& albumName,
                                   std::list<LoadedKnownFace>& namesAndIDs)
  {
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("FaceRecognizer.LoadAlbum.NotInitialized", "");
      return RESULT_FAIL;
    }
    
    if(NULL == _okaoCommonHandle) {
      PRINT_NAMED_ERROR("FaceRecognizer.LoadAlbum.NullFaceLibCommonHandle", "");
      return RESULT_FAIL;
    }
    
    Result result = RESULT_OK;
    
    const std::string dataFilename(albumName + "/data.bin");
    std::ifstream fs(dataFilename, std::ios::in | std::ios::binary);
    if(!fs.is_open()) {
      PRINT_NAMED_WARNING("FaceRecognizer.LoadAlbum.FileOpenFail", "Filename: %s", dataFilename.c_str());
      result = RESULT_FAIL;
    } else {
      
      // Stop eating new lines in binary mode!!!
      fs.unsetf(std::ios::skipws);
      
      fs.seekg(0, std::ios::end);
      auto fileLength = fs.tellg();
      fs.seekg(0, std::ios::beg);
      
      std::vector<u8> serializedAlbum;
      serializedAlbum.reserve((size_t)fileLength);
      
      serializedAlbum.insert(serializedAlbum.begin(),
                             std::istream_iterator<u8>(fs),
                             std::istream_iterator<u8>());
      fs.close();
      
      if(serializedAlbum.size() != fileLength) {
        PRINT_NAMED_WARNING("FaceRecognizer.LoadAlbum.FileReadFail", "Filename: %s", dataFilename.c_str());
        result = RESULT_FAIL;
      } else {
        // Temporary data structures to load into, so we can check they are consistent
        // before using them
        HALBUM loadedAlbum;
        EnrollmentData loadedEnrollmentData;
        
        result = SetSerializedAlbum(_okaoCommonHandle, serializedAlbum, loadedAlbum);
        
        // Now try to read the names data
        if(RESULT_OK == result) {
          Json::Value json;
          const std::string namesFilename(albumName + "/enrollData.json");
          std::ifstream jsonFile(namesFilename);
          Json::Reader reader;
          bool success = reader.parse(jsonFile, json);
          jsonFile.close();
          if(! success) {
            PRINT_NAMED_WARNING("FaceRecognizer.LoadAlbum.EnrollDataFileReadFail", "");
            result = RESULT_FAIL;
          }
          else
          {
            auto const nowTime = std::chrono::system_clock::now();
            
            for(auto & idStr : json.getMemberNames()) {
              FaceID_t faceID = std::stoi(idStr);
              if(!json.isMember(idStr)) {
                PRINT_NAMED_WARNING("FaceRecognizer.LoadAlbum.BadFaceIdString",
                                    "Could not find member for string %s with value %d",
                                    idStr.c_str(), faceID);
                result = RESULT_FAIL;
              } else {
                EnrolledFaceEntry entry(faceID, json[idStr]);
                
                namesAndIDs.emplace_back( LoadedKnownFace(GetSecondsSince(nowTime, entry.GetEnrollmentTime()),
                                                          GetSecondsSince(nowTime, entry.GetLastUpdateTime()),
                                                          GetSecondsSince(nowTime, entry.FindLastSeenTime()),
                                                          entry.GetFaceID(),
                                                          entry.GetName()) );
                
                PRINT_CH_INFO(LOG_CHANNEL, "LoadAlbum.LoadedEnrollmentData", "ID=%d, '%s'",
                              faceID, Util::HidePersonallyIdentifiableInfo(entry.GetName().c_str()));
                
                loadedEnrollmentData[faceID] = std::move(entry);
              }
            }
          }
        }
        
        result = UseLoadedAlbumAndEnrollData(loadedAlbum, loadedEnrollmentData);
      }
    }
    
    return result;
  } // LoadAlbum()
  
  
  
  //
  // This may prove useful later if we ever want to try to do more complicated / smarter
  // merging of records based on cluster similarity / dissimilarity. I ran out of
  // time and steam working on this, but rather than it getting totally lost in git
  // history, I'm going to deliberately leave it here for now.
  //
  //  Result FaceRecognizer::SelectiveMergeHelper(EnrollmentData::iterator keepIter,
  //                                              EnrollmentData::iterator mergeIter)
  //  {
  //    // - Create one big list of all the album entries from both mergeID and keepID
  //    // - Compute pairwise comparison of all the entries
  //    // - Start with everything in the "remove" set
  //    // - First choose the two entries that are furthest apart and move to the "keep" set
  //    // - Incrementally move entries from "remove" to "keep" according to which one
  //    //    has the nearest neighbor in the current "keep" set that's furthest away (this
  //    //    will be the one with the lowest maximum score)
  //    // - The goal here is to get a diverse set of album entries
  //
  //    OkaoResult okaoResult = OKAO_NORMAL;
  //
  //    std::vector<AlbumEntryID_t> combinedEntries;
  //
  //    std::copy(keepIter->second.GetAlbumEntries().begin(),
  //              keepIter->second.GetAlbumEntries().end(),
  //              std::back_inserter(combinedEntries));
  //
  //    std::copy(mergeIter->second.GetAlbumEntries().begin(),
  //              mergeIter->second.GetAlbumEntries().end(),
  //              std::back_inserter(combinedEntries));
  //
  //    const size_t numKeepEntries  = keepIter->second.GetAlbumEntries().size();
  //    const size_t numMergeEntries = mergeIter->second.GetAlbumEntries().size();
  //
  //    // Start with everything in the "remove" set. We will shift things from this
  //    // set to the "keep" set below.
  //    std::set<s32> indicesToRemove;
  //    for(s32 index=0; index<numKeepEntries+numMergeEntries; ++index)
  //    {
  //      indicesToRemove.insert(index);
  //    }
  //
  //    const s32 matrixSize = Util::numeric_cast<s32>(combinedEntries.size());
  //    Matrix<f32> pairwiseScores(matrixSize, matrixSize);
  //    pairwiseScores.FillWith(0);
  //
  //    // Pre-store row pointers for the matrix:
  //    std::vector<f32*> pairwiseScoresRow(pairwiseScores.GetNumRows());
  //    for(s32 i=0; i<pairwiseScores.GetNumRows(); ++i)
  //    {
  //      pairwiseScoresRow[i] = pairwiseScores.GetRow(i);
  //    }
  //
  //    for(s32 iComp=0; iComp < combinedEntries.size(); ++iComp)
  //    {
  //      // Since Okao does not offer a way to do albumEntry to albumEntry (i.e. "userID" to "userID")
  //      // comparisons (??), I'm using the maximum score of any one of entry_i's features
  //      // to entry_j as a proxy. So I have to loop over each data entry in albumEntry i
  //      // and compare it to albumEntry j using the "Verify" function to get a score.
  //      for(s32 iData=0; iData < kMaxEnrollDataPerAlbumEntry; ++iData)
  //      {
  //        BOOL isRegistered = false;
  //        okaoResult = OKAO_FR_IsRegistered(_okaoFaceAlbum, combinedEntries[iComp], iData, &isRegistered);
  //        if(OKAO_NORMAL != okaoResult) {
  //          PRINT_NAMED_ERROR("FaceRecognizer.SelectiveMergeHelper.IsRegisteredCheckFailed",
  //                            "FaceLib result=%d", okaoResult);
  //          return RESULT_FAIL;
  //        }
  //
  //        if(!isRegistered) {
  //          continue;
  //        }
  //
  //        okaoResult = OKAO_FR_GetFeatureFromAlbum(_okaoFaceAlbum, combinedEntries[iComp], iData, _okaoRecogMergeFeatureHandle);
  //        if(OKAO_NORMAL != okaoResult) {
  //          PRINT_NAMED_ERROR("FaceRecognizer.SelectiveMergeHelper.GetFeatureFromMergeAlbumFailed",
  //                            "AlbumEntry:%d Data:%d FaceLib result=%d", combinedEntries[iComp], iData, okaoResult);
  //          return RESULT_FAIL;
  //        }
  //
  //        // Make sure we never choose diagonal entry
  //        pairwiseScoresRow[iComp][iComp] = std::numeric_limits<f32>::max();
  //
  //        // Note we start jComp at iComp+1 because (a) comparisons are symmetric so we can
  //        // update two entries at once, and (b) there's no reason to compare an entry to itself
  //        for(s32 jComp=iComp+1; jComp<combinedEntries.size(); ++jComp)
  //        {
  //          RecognitionScore currentScore = 0;
  //          okaoResult = OKAO_FR_Verify(_okaoRecogMergeFeatureHandle, _okaoFaceAlbum,
  //                                      combinedEntries[jComp], &currentScore);
  //          if(OKAO_NORMAL != okaoResult) {
  //            PRINT_NAMED_ERROR("FaceRecognizer.SelectiveMergeHelper.VerifyFailed",
  //                              "Comparing AlbumEntry %d and %d. FaceLib result=%d",
  //                              combinedEntries[iComp], combinedEntries[jComp], okaoResult);
  //            return RESULT_FAIL;
  //          }
  //
  //          // Store highest score over all the data entries, as discussed above
  //          if(currentScore > pairwiseScoresRow[iComp][jComp])
  //          {
  //            pairwiseScoresRow[iComp][jComp] = currentScore;
  //            pairwiseScoresRow[jComp][iComp] = currentScore;
  //          }
  //        }
  //      }
  //    }
  //
  //    // Find the minimum scoring entry in the matrix and keep both corresponding AlbumEntries
  //    double minScore=0;
  //    cv::Point minLoc;
  //    cv::minMaxLoc(pairwiseScores.get_CvMat_(), &minScore, nullptr, &minLoc, nullptr);
  //
  //    s32 firstToKeep  = minLoc.x;
  //    s32 secondToKeep = minLoc.y;
  //    std::set<s32> indicesToKeep;
  //    indicesToKeep.insert(firstToKeep);
  //    indicesToKeep.insert(secondToKeep);
  //    indicesToRemove.erase(firstToKeep);
  //    indicesToRemove.erase(secondToKeep);
  //    PRINT_CH_INFO("FaceRecognizerDebug", "SelectiveMergeHelper.FirstTwoEntriesToKeep",
  //                  "Keeping indices %d and %d (albumEntries %d and %d, from FaceIDs %d and %d) "
  //                  "with pairwise score %d",
  //                  firstToKeep, secondToKeep,
  //                  combinedEntries[firstToKeep], combinedEntries[secondToKeep],
  //                  _albumEntryToFaceID.at(combinedEntries[firstToKeep]),
  //                  _albumEntryToFaceID.at(combinedEntries[secondToKeep]),
  //                  static_cast<RecognitionScore>(minScore));
  //
  //    // Now repeatedly find the entry not yet selected to keep (i.e. still in the "remove"
  //    // set) that has the lowest max score to whatever is currently in the set of keepers.
  //    // This will be the entry who is furthest away from its closest neighbor in the keeper set.
  //    while(indicesToKeep.size() < kMaxAlbumEntriesPerFace && !indicesToRemove.empty())
  //    {
  //      RecognitionScore minMaxScore = std::numeric_limits<RecognitionScore>::max();
  //      s32 nextToAdd = -1;
  //      for(s32 iComp : indicesToRemove)
  //      {
  //        RecognitionScore maxScore = 0;
  //        for(s32 jComp : indicesToKeep)
  //        {
  //          RecognitionScore currentScore = pairwiseScoresRow[iComp][jComp];
  //          if(currentScore > maxScore)
  //          {
  //            maxScore = currentScore;
  //          }
  //        }
  //
  //        if(maxScore < minMaxScore)
  //        {
  //          minMaxScore = maxScore;
  //          nextToAdd = iComp;
  //        }
  //      }
  //
  //      // Switch entry from "remove" to "keep"
  //      indicesToKeep.insert(nextToAdd);
  //      indicesToRemove.erase(nextToAdd);
  //
  //      PRINT_CH_INFO("FaceRecognizerDebug", "SelectiveMergeHelper.KeepingAlbumEntry",
  //                    "Index:%d AlbumEntry:%d from FaceID:%d with minimum maxScore:%d",
  //                    nextToAdd, combinedEntries.at(nextToAdd),
  //                    _albumEntryToFaceID.at(combinedEntries.at(nextToAdd)), minMaxScore);
  //    }
  //
  //    // Update those to keep
  //    for(auto index : indicesToKeep)
  //    {
  //      const AlbumEntryID_t albumEntry = combinedEntries.at(index);
  //      EnrolledFaceEntry::Time origTime = keepIter->second.GetAlbumEntries().at(albumEntry);
  //      keepIter->second.AddOrUpdateAlbumEntry( albumEntry, origTime ); // NOTE: Does nothing if already an entry
  //      _albumEntryToFaceID[albumEntry] = keepIter->first;
  //    }
  //
  //    // Update those to remove
  //    for(auto index : indicesToRemove)
  //    {
  //      const AlbumEntryID_t albumEntry = combinedEntries.at(index);
  //      keepIter->second.RemoveAlbumEntry( albumEntry );
  //
  //      PRINT_CH_INFO("FaceRecognizerDebug", "SelectiveMergeHelper.RemovingAlbumEntry",
  //                    "Index:%d AlbumEntry:%d from FaceID:%d",
  //                    index, albumEntry, _albumEntryToFaceID.at(albumEntry));
  //
  //      _albumEntryToFaceID.erase(albumEntry);
  //
  //      okaoResult = OKAO_FR_ClearUser(_okaoFaceAlbum, albumEntry);
  //      if(OKAO_NORMAL != okaoResult) {
  //        PRINT_NAMED_ERROR("FaceRecognizer.SelectiveMergeHelper.ClearFailed",
  //                          "Clearing AlbumEntry:%d. FaceLib result=%d",
  //                          albumEntry, okaoResult);
  //        return RESULT_FAIL;
  //      }
  //    }
  //
  //    ASSERT_NAMED_EVENT(keepIter->second.GetAlbumEntries().size() == kMaxAlbumEntriesPerFace,
  //                       "FaceRecognizer.SelectiveMergeHelper.UnexpectedNumAlbumEntries",
  //                       "NumAlbumEntries=%zu not %d",
  //                       keepIter->second.GetAlbumEntries().size(),
  //                       kMaxAlbumEntriesPerFace);
  //
  //    return RESULT_OK;
  //
  //  } // SelectiveMergeHelper()
  
} // namespace Vision
} // namespace Anki

#endif // #if FACE_TRACKER_PROVIDER == FACE_TRACKER_OKAO

