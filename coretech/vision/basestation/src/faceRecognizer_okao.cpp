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

#include "json/json.h"
#include "anki/common/basestation/jsonTools.h"

#include "OkaoCoAPI.h"

#include <fstream>

namespace Anki {
namespace Vision {
  
  FaceRecognizer::FaceRecognizer(const Json::Value& config)
  {
    // Macro to prevent typos: name in json must match member variable name (w/ preceding underscore)
#   define SetParam(__NAME__) JsonTools::GetValueOptional(recognitionConfig, QUOTE(__NAME__), _##__NAME__)
    if(config.isMember("faceRecognition"))
    {
      // TODO: Use string constants
      const Json::Value& recognitionConfig = config["faceRecognition"];
      
      SetParam(recognitionThreshold);
      SetParam(mergeThreshold);
      SetParam(timeBetweenEnrollmentUpdates_sec);
      SetParam(timeBetweenInitialEnrollmentUpdates_sec);
      
      std::string runModeStr;
      if(JsonTools::GetValueOptional(recognitionConfig, "runMode", runModeStr)) {
        if(runModeStr == "asynchronous") {
          _isRunningAsync = true;
        } else if(runModeStr == "synchronous") {
          _isRunningAsync = false;
        } else {
          ASSERT_NAMED(false, "Unrecognized FaceRecognizer run mode. Should be 'synchronous' or 'asynchronous'");
        }
      }
    } else {
      PRINT_NAMED_WARNING("FaceRecognizer.Constructor.NoFaceRecParameters",
                          "Did not find 'faceRecognition' field in config");
    }
#   undef SetParam
  }
  
  FaceRecognizer::~FaceRecognizer()
  {
    // Wait for recognition thread to die before destructing since we gave it a
    // reference to *this
    _recognitionRunning = false;
    if(_isRunningAsync && _recognitionThread.joinable()) {
      _recognitionThread.join();
    }
    
    if(NULL != _okaoFaceAlbum) {
      
      // TODO: Save album first!
      
      if(OKAO_NORMAL != OKAO_FR_DeleteAlbumHandle(_okaoFaceAlbum)) {
        PRINT_NAMED_ERROR("FaceRecognizer.Destructor.OkaoAlbumHandleDeleteFail", "");
      }
    }
    
    if(NULL != _okaoRecogMergeFeatureHandle) {
      if(OKAO_NORMAL != OKAO_FR_DeleteFeatureHandle(_okaoRecogMergeFeatureHandle)) {
        PRINT_NAMED_ERROR("FaceRecognizer.Destructor.OkaoRecognitionMergeFeatureHandleDeleteFail", "");
      }
    }
    
    if(NULL != _okaoRecognitionFeatureHandle) {
      if(OKAO_NORMAL != OKAO_FR_DeleteFeatureHandle(_okaoRecognitionFeatureHandle)) {
        PRINT_NAMED_ERROR("FaceRecognizer.Destructor.OkaoRecognitionFeatureHandleDeleteFail", "");
      }
    }
  }
  
  Result FaceRecognizer::Init(HCOMMON okaoCommonHandle)
  {
    _okaoRecognitionFeatureHandle = OKAO_FR_CreateFeatureHandle(okaoCommonHandle);
    if(NULL == _okaoRecognitionFeatureHandle) {
      PRINT_NAMED_ERROR("FaceRecognizer.Init.OkaoFeatureHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoRecogMergeFeatureHandle = OKAO_FR_CreateFeatureHandle(okaoCommonHandle);
    if(NULL == _okaoRecogMergeFeatureHandle) {
      PRINT_NAMED_ERROR("FaceRecognizer.Init.OkaoMergeFeatureHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoFaceAlbum = OKAO_FR_CreateAlbumHandle(okaoCommonHandle, MaxAlbumFaces, MaxAlbumDataPerFace);
    if(NULL == _okaoFaceAlbum) {
      PRINT_NAMED_ERROR("FaceRecognizer.Init.OkaoAlbumHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _detectionInfo.nID = -1;
    
    _recognitionRunning = true;
    if(_isRunningAsync) {
      _recognitionThread = std::thread(&FaceRecognizer::Run, this);
    }
    
    _isInitialized = true;
    
    return RESULT_OK;
  }
  
  EnrolledFaceEntry FaceRecognizer::GetRecognitionData(INT32 forTrackingID)
  {
    EnrolledFaceEntry entry;
    
    if(ProcessingState::FeaturesReady == _state)
    {
      // Feature extraction thread is done: finish the rest of the recognition
      // process so we can start accepting new requests to recognize
      TrackedFace::ID_t recognizedID = TrackedFace::UnknownFace;
      s32 score = 0;
      Result result = RecognizeFace(recognizedID, score);
      
      // For simulating slow processing (e.g. on a device)
      //std::this_thread::sleep_for(std::chrono::milliseconds(250));
      
      if(RESULT_OK == result)
      {
        // See who we're currently recognizing this tracker ID as
        TrackedFace::ID_t faceID = TrackedFace::UnknownFace;
        auto iter = _trackingToFaceID.find(_detectionInfo.nID);
        if(iter != _trackingToFaceID.end()) {
          faceID = iter->second;
        }
        
        if(TrackedFace::UnknownFace == recognizedID) {
          // We did not recognize the tracked face in its current position, just leave
          // the faceID alone
          
          // (Nothing to do)
          
        } else if(TrackedFace::UnknownFace == faceID) {
          // We have not yet assigned a recognition ID to this tracker ID. Use the
          // one we just found via recognition.
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
            ASSERT_NAMED(recIDenrollData  != _enrollmentData.end(),
                         "RecognizedID should have enrollment data by now");
            
            TrackedFace::ID_t mergeTo, mergeFrom;
            
            if(faceIDenrollData->second.enrollmentTime <= recIDenrollData->second.enrollmentTime)
            {
              mergeFrom = recognizedID;
              mergeTo   = faceID;
            } else {
              mergeFrom = faceID;
              mergeTo   = recognizedID;
              
              faceID = mergeTo;
            }
            
            PRINT_NAMED_INFO("FaceRecognizer.Run.MergingFaces",
                             "Tracking %d: merging ID=%d into ID=%d",
                             _detectionInfo.nID, mergeFrom, mergeTo);
            Result mergeResult = MergeFaces(mergeTo, mergeFrom);
            if(RESULT_OK != mergeResult) {
              PRINT_NAMED_WARNING("FaceRecognizer.Run.MergeFail",
                                  "Trying to merge %d with %d", faceID, recognizedID);
            }
          }
          
          
        } else {
          // We recognized this person as the same ID already assigned to its tracking ID
          ASSERT_NAMED(faceID == recognizedID, "Expecting faceID to match recognizedID");
        }
        
        // Update the stored faceID assigned to this trackerID
        // (This creates an entry for the current trackerID if there wasn't one already,
        //  and an entry for this faceID if there wasn't one already)
        if(TrackedFace::UnknownFace != faceID) {
          _trackingToFaceID[_detectionInfo.nID] = faceID;
          _enrollmentData[faceID].score = score;
        }
        
      } else {
        PRINT_NAMED_ERROR("FaceRecognizer.Run.RecognitionFailed", "");
      }
      
      // Signify we're done and ready for next face
      //PRINT_NAMED_DEBUG("FaceRecognizer.Run.DoneWithFace",
      //                  "Finished recognizing tracked ID %d as recognized ID %d",
      //                  -_detectionInfo.nID, faceID);
      _state = ProcessingState::Idle;
      
    } // if(ProcessingState::FeaturesReady == _state)
    
    auto iter = _trackingToFaceID.find(forTrackingID);
    if(iter != _trackingToFaceID.end()) {
      const TrackedFace::ID_t faceID = iter->second;
      ASSERT_NAMED(faceID != TrackedFace::UnknownFace,
                   "Enrollment entry should not have unknown face ID");
      auto & enrolledEntry = _enrollmentData.at(faceID);
      entry = enrolledEntry;
      enrolledEntry.prevID = enrolledEntry.faceID; // no longer "new" or "updated"
    }
    
    return entry;
  } // GetRecognitionData()

  
  void FaceRecognizer::RemoveTrackingID(INT32 trackerID)
  {
    _trackingToFaceID.erase(trackerID);
  }
  
  void FaceRecognizer::Run()
  {
    while(_recognitionRunning)
    {
      _mutex.lock();
      const bool anythingToDo = ProcessingState::HasNewImage == _state;
      _mutex.unlock();
      
      if(anythingToDo) {
        
        // Note: this puts us in FeaturesReady state when done (or Idle if failure)
        ExtractFeatures();
        
      } else {
        // Nothing to do: just sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }
  } // Run()
  
  
  void FaceRecognizer::ExtractFeatures()
  {
    ASSERT_NAMED(ProcessingState::HasNewImage == _state,
                 "Should be in 'HasNewImage' state to start extracting features.");
    
    // If we're not allowed to enroll new faces and there's nobody already
    // enrolled, we have nothing to do
    _mutex.lock();
    _state = ProcessingState::ExtractingFeatures;
    _mutex.unlock();
    
    //PRINT_NAMED_DEBUG("FaceRecognizer.Run.Starting",
    //                  "Starting to recognize tracked ID %d", -_detectionInfo.nID);
    
    INT32 nWidth  = _img.GetNumCols();
    INT32 nHeight = _img.GetNumRows();
    RAWIMAGE* dataPtr = _img.GetDataPointer();
    
    Tic("OkaoFeatureExtraction");
    INT32 okaoResult = OKAO_FR_ExtractHandle_GRAY(_okaoRecognitionFeatureHandle,
                                                  dataPtr, nWidth, nHeight, GRAY_ORDER_Y0Y1Y2Y3,
                                                  _okaoPartDetectionResultHandle);
    Toc("OkaoFeatureExtraction");
    
    ProcessingState newState = ProcessingState::FeaturesReady;
    
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_WARNING("FaceRecognizer.ExtractFeatures.OkaoFailure",
                          "Going back to Idle state. Okao Result=%d", okaoResult);
      newState = ProcessingState::Idle;
    }
    
    _mutex.lock();
    _state = newState;
    _mutex.unlock();
    
  } // ExtractFeatures()
  
  
  Result FaceRecognizer::RegisterNewUser(HFEATURE& hFeature)
  {
    ASSERT_NAMED(ProcessingState::FeaturesReady == _state,
                 "Should only be registering new user if features are ready");
    
    INT32 numUsersInAlbum = 0;
    INT32 okaoResult = OKAO_FR_GetRegisteredUserNum(_okaoFaceAlbum, &numUsersInAlbum);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceRecognizer.RegisterNewUser.OkaoGetNumUsersInAlbumFailed", "");
      return RESULT_FAIL;
    }
    
    if(numUsersInAlbum >= MaxAlbumFaces) {
      PRINT_NAMED_ERROR("FaceRecognizer.RegisterNewUser.TooManyUsers",
                        "Already have %d users, could not add another", MaxAlbumFaces);
      return RESULT_FAIL;
    }
    
    // Find unused ID
    BOOL isRegistered = true;
    auto tries = 0;
    do {
      okaoResult = OKAO_FR_IsRegistered(_okaoFaceAlbum, _lastRegisteredUserID-1, 0, &isRegistered);
      
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceRecognizer.RegisterNewUser.IsRegisteredCheckFailed",
                          "Failed to determine if user ID %d is already registered",
                          numUsersInAlbum);
        return RESULT_FAIL;
      }
      
      if(isRegistered) {
        // Try next ID
        ++_lastRegisteredUserID;
        if(_lastRegisteredUserID > MaxAlbumFaces) {
          // Roll over
          _lastRegisteredUserID = 1;
        }
      }
      
      ASSERT_NAMED(_lastRegisteredUserID != Vision::TrackedFace::UnknownFace,
                   "LastRegisteredID should not be the UnknownFace ID");
      
    } while(isRegistered && tries++ < MaxAlbumFaces);
    
    if(tries >= MaxAlbumFaces) {
      PRINT_NAMED_ERROR("FaceRecognizer.RegisturNewUser.NoIDsAvailable",
                        "Could not find a free ID to use for new face");
      return RESULT_FAIL;
    }
    
    okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, hFeature, _lastRegisteredUserID-1, 0);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceRecognizer.RegisterNewUser.RegisterFailed",
                        "Failed trying to register user %d", _lastRegisteredUserID);
      return RESULT_FAIL;
    }

    // Create new enrollment entry (defaults set by constructor)
    // NOTE: enrollData.prevID == UnknownID now, which indicates this is "new"
    EnrolledFaceEntry enrollData(_lastRegisteredUserID);

    // TODO: Populate enrollment image
    /*
    POINT ptLeftTop, ptRightTop, ptLeftBottom, ptRightBottom;
    okaoResult = OKAO_CO_ConvertCenterToSquare(_detectionInfo.ptCenter,
                                               _detectionInfo.nHeight,
                                               0, &ptLeftTop, &ptRightTop,
                                               &ptLeftBottom, &ptRightBottom);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceRecognizer.RegisterNewUser.OkaoCenterToSquareFail", "");
      return RESULT_FAIL;
    }
    
    Rectangle<s32> roi(ptLeftTop.x, ptLeftTop.y,
                       ptRightBottom.x-ptLeftTop.x,
                       ptRightBottom.y-ptLeftTop.y);

    enrollData.image = _img.GetROI(roi);
    */
    _enrollmentData.emplace(_lastRegisteredUserID, std::move(enrollData));
    --_numToEnroll;
    
    PRINT_NAMED_INFO("FaceRecognizer.RegisterNewUser.Success",
                     "Added user number %d to album", _lastRegisteredUserID);
    
    return RESULT_OK;
  } // RegisterNewUser()
  
  bool FaceRecognizer::IsEnrollable()
  {
    bool retVal = false;
    // TODO: Add checks for eyes open and smiling?
    if(_numToEnroll != 0 &&
       _detectionInfo.nPose == POSE_YAW_FRONT &&
       _detectionInfo.nWidth*_detectionInfo.nHeight > 1000)
    {
      return true;
    }
    return retVal;
  }
  
  void FaceRecognizer::EnableNewFaceEnrollment(s32 numToEnroll)
  {
    _numToEnroll = numToEnroll;
  }
  
  bool FaceRecognizer::SetNextFaceToRecognize(const Image& img,
                                              const DETECTION_INFO& detectionInfo,
                                              HPTRESULT okaoPartDetectionResultHandle)
  {
    // Nothing to do if we aren't allowed to enroll anyone and there's nobody
    // in the album yet to match this face to
    const bool anythingToDo = (_numToEnroll != 0 || !_enrollmentData.empty());
    
    if(ProcessingState::Idle == _state && anythingToDo)
    {
      // Not currently busy: copy in the given data and start working on it
      
      _mutex.lock();
      img.CopyTo(_img);
      _okaoPartDetectionResultHandle = okaoPartDetectionResultHandle;
      _detectionInfo = detectionInfo;
      
      //PRINT_NAMED_DEBUG("FaceRecognizer.SetNextFaceToRecognize.SetNextFace",
      //                  "Setting next face to recognize: tracked ID %d", -_detectionInfo.nID);

      _state = ProcessingState::HasNewImage;
      _mutex.unlock();
      
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
  
  
  Result FaceRecognizer::UpdateExistingUser(INT32 userID, HFEATURE& hFeature)
  {
    ASSERT_NAMED(ProcessingState::FeaturesReady == _state,
                 "Should be in 'FeaturesReady' state if updating user record");
    
    Result result = RESULT_OK;
    
    const time_t updateTime = time(0);
    
    auto enrollDataIter = _enrollmentData.find(userID);
    ASSERT_NAMED(enrollDataIter != _enrollmentData.end(), "Missing enrollment status");
    auto & enrollData = enrollDataIter->second;
    
    INT32 numDataStored = 0;
    INT32 okaoResult = OKAO_FR_GetRegisteredUsrDataNum(_okaoFaceAlbum, userID-1, &numDataStored);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceRecognizer.UpdateExistingUser.GetNumDataFailed",
                        "Failed to trying to get num data registered for existing user %d",
                        userID);
      result = RESULT_FAIL;
    }
    
    ASSERT_NAMED(numDataStored > 0, "Num data for user should be > 0 if we are updating!");
    
    // Only update if it has been long enough (where "long enough" can depend on
    // whether we've already filled up all data slots yet)
    auto const timeSinceLastUpdate = updateTime - enrollData.lastDataUpdateTime;
    if(timeSinceLastUpdate > _timeBetweenEnrollmentUpdates_sec ||
       ((numDataStored <= MaxAlbumDataPerFace) &&  timeSinceLastUpdate > _timeBetweenInitialEnrollmentUpdates_sec))
    {
      okaoResult=  OKAO_FR_RegisterData(_okaoFaceAlbum, hFeature, userID-1, enrollData.oldestData);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceRecognizer.UpdateExistingUser.RegisterFailed",
                          "Failed to trying to register data %d for existing user %d",
                          enrollData.oldestData, userID);
        result = RESULT_FAIL;
      } else {
        // Update the enrollment data
        enrollData.lastDataUpdateTime = updateTime;
        enrollData.oldestData++;
        if(enrollData.oldestData == MaxAlbumDataPerFace) {
          enrollData.oldestData = 0;
        }
      }
    }
    
    return result;
  } // UpdateExistingUser()
  
  
  Result FaceRecognizer::RemoveUser(INT32 userID)
  {
    INT32 okaoResult = OKAO_FR_ClearUser(_okaoFaceAlbum, userID-1);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceRecognizer.RemoveUser.ClearUserFailed",
                        "OKAO result=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    _enrollmentData.erase(userID);
    
    // TODO: Keep a reference to which trackerIDs are pointing to each faceID to avoid this search
    for(auto iter = _trackingToFaceID.begin(); iter != _trackingToFaceID.end(); )
    {
      if(iter->second == userID) {
        iter = _trackingToFaceID.erase(iter);
      } else {
        ++iter;
      }
    }
    
    return RESULT_OK;
  }
  

  Result FaceRecognizer::RecognizeFace(TrackedFace::ID_t& faceID, INT32& recognitionScore)
  {
    ASSERT_NAMED(ProcessingState::FeaturesReady == _state,
                 "Must be in 'FeaturesReady' state to call RecognizeFace");
    
    INT32 numUsersInAlbum = 0;
    INT32 okaoResult = OKAO_FR_GetRegisteredUserNum(_okaoFaceAlbum, &numUsersInAlbum);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceRecognizer.RecognizeFace.OkaoGetNumUsersInAlbumFailed", "");
      return RESULT_FAIL;
    }
    
    const INT32 MaxScore = 1000;
    
    const bool isEnrollable = IsEnrollable();
    
    if(numUsersInAlbum == 0 && isEnrollable) {
      // Nobody in album yet, add this person
      PRINT_NAMED_INFO("FaceRecognizer.RecognizeFace.AddingFirstUser",
                       "Adding first user to empty album");
      Result lastResult = RegisterNewUser(_okaoRecognitionFeatureHandle);
      if(RESULT_OK != lastResult) {
        return lastResult;
      }
      faceID = _lastRegisteredUserID;
      recognitionScore = MaxScore;
    } else {
      INT32 resultNum = 0;
      const s32 MaxResults = 2; // TODO: Increase this to allow merging or more than two results below?
      INT32 userIDs[MaxResults], scores[MaxResults];
      Tic("OkaoIdentify");
      okaoResult = OKAO_FR_Identify(_okaoRecognitionFeatureHandle, _okaoFaceAlbum, MaxResults, userIDs, scores, &resultNum);
      Toc("OkaoIdentify");
      
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_WARNING("FaceRecognizer.RecognizeFace.OkaoFaceRecognitionIdentifyFailed",
                            "OKAO Result Code=%d", okaoResult);
      } else {
        //const f32   RelativeRecogntionThreshold = 1.5; // Score of top result must be this times the score of the second best result
        
        if(resultNum > 0 && scores[0] > _recognitionThreshold)
        {
          // Our ID numbering starts at 1, Okao's starts at 0:
          for(s32 iResult=0; iResult<resultNum; ++iResult) {
            userIDs[iResult] += 1;
          }
          
          INT32 oldestID = userIDs[0];
          
          auto enrollStatusIter = _enrollmentData.find(oldestID);
          auto oldestEnrollIter = enrollStatusIter;
          
          ASSERT_NAMED(enrollStatusIter != _enrollmentData.end(),
                       "ID should have enrollment status entry");
          
          auto earliestEnrollmentTime = enrollStatusIter->second.enrollmentTime;
          
          // Merge all results that are above threshold together, keeping the one enrolled earliest
          INT32 lastResult = 1;
          while(lastResult < resultNum && scores[lastResult] > _mergeThreshold)
          {
            enrollStatusIter = _enrollmentData.find(userIDs[lastResult]);
            
            ASSERT_NAMED(enrollStatusIter != _enrollmentData.end(),
                         "ID should have enrollment status entry");
            if(enrollStatusIter->second.enrollmentTime < earliestEnrollmentTime)
            {
              oldestID = userIDs[lastResult];
              earliestEnrollmentTime = enrollStatusIter->second.enrollmentTime;
              oldestEnrollIter = enrollStatusIter;
            }
            
            ++lastResult;
          }
          
          for(INT32 iResult = 0; iResult < lastResult; ++iResult)
          {
            if(userIDs[iResult] != oldestID)
            {
              Result result = MergeFaces(oldestID, userIDs[iResult]);
              if(RESULT_OK != result) {
                PRINT_NAMED_INFO("FaceRecognizer.RecognizeFace.MergeFail",
                                 "Failed to merge frace %d with %d",
                                 userIDs[iResult], oldestID);
                return result;
              } else {
                PRINT_NAMED_INFO("FaceRecognizer.RecognizeFace.MergingRecords",
                                 "Merging face %d with %d b/c its score %d is > %d",
                                 userIDs[iResult], oldestID, scores[iResult], _mergeThreshold);
              }
            }
          }
          
          Result result = UpdateExistingUser(oldestID, _okaoRecognitionFeatureHandle);
          if(RESULT_OK != result) {
            return result;
          }
          
          faceID = oldestID;
          recognitionScore = scores[0];
          
        } else if(isEnrollable) {
          // No match found, add new user
          PRINT_NAMED_INFO("FaceRecognizer.RecognizeFace.AddingNewUser",
                           "Observed new person. Adding to album.");
          Result lastResult = RegisterNewUser(_okaoRecognitionFeatureHandle);
          if(RESULT_OK != lastResult) {
            return lastResult;
          }
          faceID = _lastRegisteredUserID;
          recognitionScore = MaxScore;
        }
      }
    }
    
    return RESULT_OK;
  } // RecognizeFace()
  
  Result FaceRecognizer::MergeFaces(TrackedFace::ID_t keepID,
                                    TrackedFace::ID_t mergeID)
  {
    ASSERT_NAMED(ProcessingState::FeaturesReady == _state,
                 "Should be in 'FeaturesReady' state if merging faces");
    
    INT32 okaoResult = OKAO_NORMAL;
    
    INT32 numKeepData = 0;
    okaoResult = OKAO_FR_GetRegisteredUsrDataNum(_okaoFaceAlbum, keepID-1, &numKeepData);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceRecognizer.MergeFaces.GetNumKeepDataFail",
                        "OKAO result=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    INT32 numMergeData = 0;
    okaoResult = OKAO_FR_GetRegisteredUsrDataNum(_okaoFaceAlbum, mergeID-1, &numMergeData);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceRecognizer.MergeFaces.GetNumMergeDataFail",
                        "OKAO result=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    // TODO: Better way of deciding which (not just how many) registered data to keep
    
    while((numMergeData+numKeepData) >= MaxAlbumDataPerFace)
    {
      --numKeepData;
      if((numMergeData+numKeepData) < MaxAlbumDataPerFace) {
        break;
      }
      --numMergeData;
    }
    ASSERT_NAMED((numMergeData+numKeepData) < MaxAlbumDataPerFace,
                 "Total merge and keep data should be less than max data per face");
    
    
    for(s32 iMerge=0; iMerge < numMergeData; ++iMerge) {
      
      okaoResult = OKAO_FR_GetFeatureFromAlbum(_okaoFaceAlbum, mergeID-1, iMerge,
                                               _okaoRecogMergeFeatureHandle);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceRecognizer.MergeFaces.GetFeatureFail",
                          "OKAO result=%d", okaoResult);
        return RESULT_FAIL;
      }
      
      okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, _okaoRecogMergeFeatureHandle,
                                        keepID-1, numKeepData + iMerge);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceRecognizer.MergeFaces.RegisterDataFail",
                          "OKAO result=%d", okaoResult);
        return RESULT_FAIL;
      }
    }
    
    
    // Merge the enrollment data
    auto keepEnrollIter = _enrollmentData.find(keepID);
    auto mergeEnrollIter = _enrollmentData.find(mergeID);
    ASSERT_NAMED(keepEnrollIter != _enrollmentData.end(),
                 "ID to keep should have enrollment data entry");
    ASSERT_NAMED(mergeEnrollIter != _enrollmentData.end(),
                 "ID to merge should have enrollment data entry");
    
    keepEnrollIter->second.MergeWith(mergeEnrollIter->second);
    
    
    // After merging it, remove the old user that just got merged
    Result removeResult = RemoveUser((INT32)mergeID);
    
    return removeResult;
    
  } // MergeFaces()
  
  
  std::vector<u8> FaceRecognizer::GetSerializedAlbum()
  {
    std::vector<u8> serializedAlbum;
    
    if(_isInitialized)
    {
      UINT32 albumSize = 0;
      INT32 okaoResult = OKAO_FR_GetSerializedAlbumSize(_okaoFaceAlbum, &albumSize);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceRecognizer.GetSerializedAlbum.GetSizeFail",
                          "OKAO Result=%d", okaoResult);
      } else {
        serializedAlbum.resize(albumSize);
        okaoResult = OKAO_FR_SerializeAlbum(_okaoFaceAlbum, &(serializedAlbum[0]), albumSize);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_ERROR("FaceRecognizer.GetSerializedAlbum.SerializeFail",
                            "OKAO Result=%d", okaoResult);
        }
      }
    } else {
      PRINT_NAMED_ERROR("FaceRecognizer.GetSerializedAlbum.NotInitialized", "");
    }
    
    return serializedAlbum;
  } // GetSerializedAlbum()
  
  
  Result FaceRecognizer::SetSerializedAlbum(HCOMMON okaoCommonHandle, const std::vector<u8>&serializedAlbum)
  {
    FR_ERROR error;
    HALBUM restoredAlbum = OKAO_FR_RestoreAlbum(okaoCommonHandle, const_cast<UINT8*>(&(serializedAlbum[0])), (UINT32)serializedAlbum.size(), &error);
    if(NULL == restoredAlbum) {
      PRINT_NAMED_ERROR("FaceRecognizer.SetSerializedAlbum.RestoreFail",
                        "OKAO Result=%d", error);
      return RESULT_FAIL;
    }
    
    INT32 okaoResult = OKAO_FR_DeleteAlbumHandle(_okaoFaceAlbum);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceRecognizer.SetSerializedAlbum.DeleteOldAlbumFail",
                        "OKAO Result=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    _okaoFaceAlbum = restoredAlbum;
    for(INT32 iUser=0; iUser<MaxAlbumFaces; ++iUser) {
      BOOL isRegistered = false;
      okaoResult = OKAO_FR_IsRegistered(_okaoFaceAlbum, iUser, 0, &isRegistered);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceRecognizer.SetSerializedAlbum.IsRegisteredCheckFail",
                          "OKAO Result=%d", okaoResult);
        return RESULT_FAIL;
      }
    }
    
    return RESULT_OK;
  } // SetSerializedAlbum()
  
  
  void FaceRecognizer::AssignNameToID(TrackedFace::ID_t faceID, const std::string& name)
  {
    _mutex.lock();
    auto iter = _enrollmentData.find(faceID);
    if(iter != _enrollmentData.end()) {
      iter->second.name = name;
      
      // Check for duplicate name
      for(auto dupIter = _enrollmentData.begin(); dupIter != _enrollmentData.end(); ++dupIter)
      {
        if(dupIter->second.name == name && dupIter->first != faceID) {
          PRINT_NAMED_WARNING("FaceRecognizer.AssignNameToID.DuplicateName",
                              "Name %s already assigned to ID %d. Merging!",
                              name.c_str(), faceID);
          MergeFaces(faceID, dupIter->first);
          break;
        }
      }

    } else {
      PRINT_NAMED_ERROR("FaceRecognizer.AssignNameToID.InvalidID",
                        "Unknown ID %d, ignoring name %s", faceID, name.c_str());
    }
    _mutex.unlock();
  }
  
  
  Result FaceRecognizer::SaveAlbum(const std::string &albumName)
  {
    Result result = RESULT_OK;
    
    std::vector<u8> serializedAlbum = GetSerializedAlbum();
    if(serializedAlbum.empty()) {
      PRINT_NAMED_ERROR("FaceRecognizer.SaveAlbum.EmptyAlbum",
                        "No serialized data returned from private implementation");
      result = RESULT_FAIL;
    } else {
      
      if(false == Util::FileUtils::CreateDirectory(albumName, false, true)) {
        PRINT_NAMED_ERROR("FaceRecognizer.SaveAlbum.DirCreationFail",
                          "Tried to create: %s", albumName.c_str());
        result = RESULT_FAIL;
      } else {
        
        const std::string dataFilename(albumName + "/data.bin");
        std::fstream fs;
        fs.open(dataFilename, std::ios::binary | std::ios::out);
        if(!fs.is_open()) {
          PRINT_NAMED_ERROR("FaceRecognizer.SaveAlbum.FileOpenFail", "Filename: %s", dataFilename.c_str());
          result = RESULT_FAIL;
        } else {
          
          fs.write((const char*)&(serializedAlbum[0]), serializedAlbum.size());
          fs.close();
          
          if((fs.rdstate() & std::ios::badbit) || (fs.rdstate() & std::ios::failbit)) {
            PRINT_NAMED_ERROR("FaceRecognizer.SaveAlbum.FileWriteFail", "Filename: %s", dataFilename.c_str());
            result = RESULT_FAIL;
          } else {
            Json::Value json;
            for(auto & enrollData : _enrollmentData)
            {
              Json::Value entry;
              enrollData.second.FillJson(entry);
              json[std::to_string(enrollData.first)] = std::move(entry);
              
              // TODO: Save enrollment image?? Privacy issues??
            }
            
            const std::string enrollDataFilename(albumName + "/enrollData.json");
            Json::FastWriter writer;
            fs.open(enrollDataFilename, std::ios::out);
            if (!fs.is_open()) {
              PRINT_NAMED_ERROR("FaceRecognizer.SaveAlbum.EnrollDataFileOpenFail", "");
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
  
  
  Result FaceRecognizer::LoadAlbum(HCOMMON okaoCommonHandle, const std::string& albumName)
  {
    Result result = RESULT_OK;
    
    const std::string dataFilename(albumName + "/data.bin");
    std::ifstream fs(dataFilename, std::ios::in | std::ios::binary);
    if(!fs.is_open()) {
      PRINT_NAMED_ERROR("FaceRecognizer.LoadAlbum.FileOpenFail", "Filename: %s", dataFilename.c_str());
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
        PRINT_NAMED_ERROR("FaceRecognizer.LoadAlbum.FileReadFail", "Filename: %s", dataFilename.c_str());
        result = RESULT_FAIL;
      } else {
        
        result = SetSerializedAlbum(okaoCommonHandle, serializedAlbum);
        
        // Now try to read the names data
        if(RESULT_OK == result) {
          Json::Value json;
          const std::string namesFilename(albumName + "/enrollData.json");
          std::ifstream jsonFile(namesFilename);
          Json::Reader reader;
          bool success = reader.parse(jsonFile, json);
          jsonFile.close();
          if(! success) {
            PRINT_NAMED_ERROR("FaceRecognizer.LoadAlbum.EnrollDataFileReadFail", "");
            result = RESULT_FAIL;
          } else {
            _enrollmentData.clear();
            for(auto & idStr : json.getMemberNames()) {
              TrackedFace::ID_t faceID = std::stoi(idStr);
              if(!json.isMember(idStr)) {
                PRINT_NAMED_ERROR("FaceRecognizer.LoadAlbum.BadFaceIdString",
                                  "Could not find member for string %s with value %d",
                                  idStr.c_str(), faceID);
                result = RESULT_FAIL;
              } else {
                _enrollmentData[faceID] = EnrolledFaceEntry(faceID, json[idStr]);
                
                // TODO: Load enrollment image??
                
                PRINT_NAMED_INFO("FaceRecognizer.LoadAlbum.LoadedEnrollmentData", "ID=%d", faceID);
              }
            }
          }
        }
      }
    }
    
    return result;
  } // LoadAlbum()
  
} // namespace Vision
} // namespace Anki

#endif // #if FACE_TRACKER_PROVIDER == FACE_TRACKER_OKAO

