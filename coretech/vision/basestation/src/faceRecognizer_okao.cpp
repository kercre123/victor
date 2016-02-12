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

#include <fstream>

namespace Anki {
namespace Vision {
  
  FaceRecognizer::~FaceRecognizer()
  {
    // Wait for recognition thread to die before destructing since we gave it a
    // reference to *this
    _recognitionRunning = false;
    if(_recognitionThread.joinable()) {
      _recognitionThread.join();
    }
    
    if(NULL != _okaoFaceAlbum) {
      
      // TODO: Save album first!
      
      if(OKAO_NORMAL != OKAO_FR_DeleteAlbumHandle(_okaoFaceAlbum)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoAlbumHandleDeleteFail", "");
      }
    }
    
    if(NULL != _okaoRecogMergeFeatureHandle) {
      if(OKAO_NORMAL != OKAO_FR_DeleteFeatureHandle(_okaoRecogMergeFeatureHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoRecognitionMergeFeatureHandleDeleteFail", "");
      }
    }
    
    if(NULL != _okaoRecognitionFeatureHandle) {
      if(OKAO_NORMAL != OKAO_FR_DeleteFeatureHandle(_okaoRecognitionFeatureHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoRecognitionFeatureHandleDeleteFail", "");
      }
    }
  }
  
  Result FaceRecognizer::Init(HCOMMON okaoCommonHandle)
  {
    _okaoRecognitionFeatureHandle = OKAO_FR_CreateFeatureHandle(okaoCommonHandle);
    if(NULL == _okaoRecognitionFeatureHandle) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoFeatureHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoRecogMergeFeatureHandle = OKAO_FR_CreateFeatureHandle(okaoCommonHandle);
    if(NULL == _okaoRecognitionFeatureHandle) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoMergeFeatureHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoFaceAlbum = OKAO_FR_CreateAlbumHandle(okaoCommonHandle, MaxAlbumFaces, MaxAlbumDataPerFace);
    if(NULL == _okaoFaceAlbum) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoAlbumHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _recognitionRunning = true;
    _recognitionThread = std::thread(&FaceRecognizer::Run, this);
    
    _isInitialized = true;
    
    return RESULT_OK;
  }
  
  FaceRecognizer::Entry FaceRecognizer::GetRecognitionData(INT32 forTrackingID)
  {
    Entry entry;
    
    _mutex.lock();
    auto iter = _trackingToFaceID.find(forTrackingID);
    if(iter != _trackingToFaceID.end()) {
      auto const& enrollData = _enrollmentData.at(faceID);
      const TrackedFace::ID_t faceID = iter->second;
      entry.faceID = faceID;
      entry.name   = enrollData.name;
      entry.score  = enrollData.lastScore;
    }
    _mutex.unlock();
    
    return entry;
  }
  
  void FaceRecognizer::RemoveTrackingID(INT32 trackerID)
  {
    _mutex.lock();
    _trackerIDsToRemove.push_back(trackerID);
    _mutex.unlock();
  }
  
  void FaceRecognizer::Run()
  {
    while(_recognitionRunning)
    {
      if(_currentTrackerID >= 0)
      {
        INT32 nWidth  = _img.GetNumCols();
        INT32 nHeight = _img.GetNumRows();
        RAWIMAGE* dataPtr = _img.GetDataPointer();
        INT32 score = 0;
        
        // Get the faceID currently assigned to this tracker ID
        _mutex.lock();
        TrackedFace::ID_t faceID = TrackedFace::UnknownFace;
        auto iter = _trackingToFaceID.find(_currentTrackerID);
        if(iter != _trackingToFaceID.end()) {
          faceID = iter->second;
        }
        _mutex.unlock();
        
        TrackedFace::ID_t recognizedID = TrackedFace::UnknownFace;
        Result result = RecognizeFace(nWidth, nHeight, dataPtr, recognizedID, score);
        
        // For simulating slow processing (e.g. on a device)
        //std::this_thread::sleep_for(std::chrono::milliseconds(250));
        
        if(RESULT_OK == result)
        {
          // TODO: Tic("FaceRecognition");
          
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
            // fact the same and merge the two, keeping the original (minimum ID)
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
                               "Tracking %d: merging ID=%llu into ID=%llu",
                               _currentTrackerID, mergeFrom, mergeTo);
              Result mergeResult = MergeFaces(mergeTo, mergeFrom);
              if(RESULT_OK != mergeResult) {
                PRINT_NAMED_WARNING("FaceRecognizer.Run.MergeFail",
                                    "Trying to merge %llu with %llu", faceID, recognizedID);
              }
            }
            // TODO: Notify the world somehow that an ID we were tracking got merged?
          } else {
            // We recognized this person as the same ID already assigned to its tracking ID
            ASSERT_NAMED(faceID == recognizedID, "Expecting faceID to match recognizedID");
          }
          
          // Update the stored faceID assigned to this trackerID
          // (This creates an entry for the current trackerID if there wasn't one already,
          //  and an entry for this faceID if there wasn't one already)
          if(TrackedFace::UnknownFace != faceID) {
            _mutex.lock();
            _trackingToFaceID[_currentTrackerID] = faceID;
            _enrollmentData[faceID].lastScore = score;
            _mutex.unlock();
          }
          
        } else {
          PRINT_NAMED_ERROR("FaceRecognizer.Run.RecognitionFailed", "");
        }
        
        // Signify we're done and ready for next face
        _currentTrackerID = -1;
        
        // Clear any tracker IDs queuend up for removal
        _mutex.lock();
        if(!_trackerIDsToRemove.empty())
        {
          for(auto trackerID : _trackerIDsToRemove) {
            _trackingToFaceID.erase(trackerID);
          }
          _trackerIDsToRemove.clear();
        }
        _mutex.unlock();
        
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
      /*
       INT32 resultNum = 0;
       const s32 MaxResults = 2;
       INT32 userIDs[MaxResults], scores[MaxResults];
       INT32 okaoResult = OKAO_FR_Identify(_okaoRecognitionFeatureHandle, _okaoFaceAlbum,
       MaxResults, userIDs, scores, &resultNum);
       */
    }
  } // Run()
  
  
  Result FaceRecognizer::RegisterNewUser(HFEATURE& hFeature)
  {
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
      okaoResult = OKAO_FR_IsRegistered(_okaoFaceAlbum, _lastRegisteredUserID, 0, &isRegistered);
      
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceRecognizer.RegisterNewUser.IsRegisteredCheckFailed",
                          "Failed to determine if user ID %d is already registered",
                          numUsersInAlbum);
        return RESULT_FAIL;
      }
      
      if(isRegistered) {
        // Try next ID
        ++_lastRegisteredUserID;
        if(_lastRegisteredUserID >= MaxAlbumFaces) {
          // Roll over
          _lastRegisteredUserID = 0;
        }
      }
      
    } while(isRegistered && tries++ < 2*MaxAlbumFaces);
    
    if(tries >= 2*MaxAlbumFaces) {
      PRINT_NAMED_ERROR("FaceRecognizer.RegisturNewUser.NoIDsAvailable",
                        "Could not find a free ID to use for new face");
      return RESULT_FAIL;
    }
    
    okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, hFeature, _lastRegisteredUserID, 0);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceRecognizer.RegisterNewUser.RegisterFailed",
                        "Failed trying to register user %d", _lastRegisteredUserID);
      return RESULT_FAIL;
    }
    
    EnrollmentData enrollData;
    enrollData.enrollmentTime = time(0);
    enrollData.lastScore = 1000;
    enrollData.oldestData = 0;
    
    _mutex.lock();
    _enrollmentData.emplace(_lastRegisteredUserID, std::move(enrollData));
    _mutex.unlock();
    
    PRINT_NAMED_INFO("FaceTrackerImpl.RegisterNewUser.Success",
                     "Added user number %d to album", _lastRegisteredUserID);
    
    return RESULT_OK;
  } // RegisterNewUser()
  
  bool FaceRecognizer::SetNextFaceToRecognize(const Vision::Image& img,
                                              INT32 trackerID,
                                              HPTRESULT okaoPartDetectionResultHandle)
  {
    if(_currentTrackerID == -1)
    {
      _mutex.lock();
      // Not currently busy: copy in the given data and start working on it
      
      img.CopyTo(_img);
      _okaoPartDetectionResultHandle = okaoPartDetectionResultHandle;
      _currentTrackerID = trackerID;
      
      _mutex.unlock();
      return true;
      
    } else {
      // Busy: tell the caller no
      return false;
    }
  }
  
  
  Result FaceRecognizer::UpdateExistingUser(INT32 userID, HFEATURE& hFeature)
  {
    Result result = RESULT_OK;
    
    _mutex.lock();
    
    auto enrollDataIter = _enrollmentData.find(userID);
    ASSERT_NAMED(enrollDataIter != _enrollmentData.end(), "Missing enrollment status");
    auto & enrollData = enrollDataIter->second;
    
    INT32 okaoResult=  OKAO_FR_RegisterData(_okaoFaceAlbum, hFeature, userID, enrollData.oldestData);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceRecognizer.UpdateExistingUser.RegisterFailed",
                        "Failed to trying to register data %d for existing user %d",
                        enrollData.oldestData, userID);
      result = RESULT_FAIL;
    } else {
      // Update the enrollment data
      enrollData.enrollmentTime = time(0);
      enrollData.oldestData++;
      if(enrollData.oldestData == MaxAlbumDataPerFace) {
        enrollData.oldestData = 0;
      }
    }
    
    _mutex.unlock();
    return result;
  } // UpdateExistingUser()
  
  
  Result FaceRecognizer::RemoveUser(INT32 userID)
  {
    INT32 okaoResult = OKAO_FR_ClearUser(_okaoFaceAlbum, userID);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.RemoveUser.ClearUserFailed",
                        "OKAO result=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    _mutex.lock();
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
    _mutex.unlock();
    
    return RESULT_OK;
  }
  
  Result FaceRecognizer::RecognizeFace(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                                       TrackedFace::ID_t& faceID, INT32& recognitionScore)
  {
    // NOTE: This is the "slow" call: extracting the features for recognition, given the
    //  facial part locations. This is the majority of the reason we are using a
    //  separate thread in the first place. We aren't touching any of the enrollment/tracking
    //  data structures, so we do not lock the mutex, meaning those data structures
    //  can be modified while this runs.
    Tic("OkaoFeatureExtraction");
    INT32 okaoResult = OKAO_FR_ExtractHandle_GRAY(_okaoRecognitionFeatureHandle,
                                                  dataPtr, nWidth, nHeight, GRAY_ORDER_Y0Y1Y2Y3,
                                                  _okaoPartDetectionResultHandle);
    Toc("OkaoFeatureExtraction");
    
    INT32 numUsersInAlbum = 0;
    okaoResult = OKAO_FR_GetRegisteredUserNum(_okaoFaceAlbum, &numUsersInAlbum);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.RecognizeFace.OkaoGetNumUsersInAlbumFailed", "");
      return RESULT_FAIL;
    }
    
    const INT32 MaxScore = 1000;
    
    if(numUsersInAlbum == 0 && _enrollNewFaces) {
      // Nobody in album yet, add this person
      PRINT_NAMED_INFO("FaceTrackerImpl.RecognizeFace.AddingFirstUser",
                       "Adding first user to empty album");
      Result lastResult = RegisterNewUser(_okaoRecognitionFeatureHandle);
      if(RESULT_OK != lastResult) {
        return lastResult;
      }
      faceID = _lastRegisteredUserID;
      recognitionScore = MaxScore;
    } else {
      INT32 resultNum = 0;
      const s32 MaxResults = 2;
      INT32 userIDs[MaxResults], scores[MaxResults];
      Tic("OkaoIdentify");
      okaoResult = OKAO_FR_Identify(_okaoRecognitionFeatureHandle, _okaoFaceAlbum, MaxResults, userIDs, scores, &resultNum);
      Toc("OkaoIdentify");
      
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_WARNING("FaceTrackerImpl.RecognizeFace.OkaoFaceRecognitionIdentifyFailed",
                            "OKAO Result Code=%d", okaoResult);
      } else {
        const INT32 RecognitionThreshold = 750; // TODO: Expose this parameter
        const INT32 MergeThreshold       = 500; // TODO: Expose this parameter
        //const f32   RelativeRecogntionThreshold = 1.5; // Score of top result must be this times the score of the second best result
        
        if(resultNum > 0 && scores[0] > RecognitionThreshold)
        {
          INT32 oldestID = userIDs[0];
          
          auto enrollStatusIter = _enrollmentData.find(oldestID);
          
          ASSERT_NAMED(enrollStatusIter != _enrollmentData.end(),
                       "ID should have enrollment status entry");
          
          auto earliestEnrollmentTime = enrollStatusIter->second.enrollmentTime;
          
          // Merge all results that are above threshold together, keeping the one enrolled earliest
          INT32 lastResult = 1;
          while(lastResult < resultNum && scores[lastResult] > MergeThreshold)
          {
            enrollStatusIter = _enrollmentData.find(userIDs[lastResult]);
            
            ASSERT_NAMED(enrollStatusIter != _enrollmentData.end(),
                         "ID should have enrollment status entry");
            if(enrollStatusIter->second.enrollmentTime < earliestEnrollmentTime)
            {
              oldestID = userIDs[lastResult];
              earliestEnrollmentTime = enrollStatusIter->second.enrollmentTime;
            }
            
            ++lastResult;
          }
          
          Result result = UpdateExistingUser(oldestID, _okaoRecognitionFeatureHandle);
          if(RESULT_OK != result) {
            return result;
          }
          
          for(INT32 iResult = 0; iResult < lastResult; ++iResult)
          {
            if(userIDs[iResult] != oldestID)
            {
              Result result = MergeFaces(oldestID, userIDs[iResult]);
              if(RESULT_OK != result) {
                PRINT_NAMED_INFO("FaceTrackerImpl.RecognizeFace.MergeFail",
                                 "Failed to merge frace %d with %d",
                                 userIDs[iResult], oldestID);
                return result;
              } else {
                PRINT_NAMED_INFO("FaceTrackerImpl.RecognizeFace.MergingRecords",
                                 "Merging face %d with %d b/c its score %d is > %d",
                                 userIDs[iResult], oldestID, scores[iResult], MergeThreshold);
              }
            }
            ++iResult;
          }
          
          faceID = oldestID;
          recognitionScore = scores[0];
          
        } else if(_enrollNewFaces) {
          // No match found, add new user
          PRINT_NAMED_INFO("FaceTrackerImpl.RecognizeFace.AddingNewUser",
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
    INT32 okaoResult = OKAO_NORMAL;
    
    INT32 numKeepData = 0;
    okaoResult = OKAO_FR_GetRegisteredUsrDataNum(_okaoFaceAlbum, (INT32)keepID, &numKeepData);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.MergeFaces.GetNumKeepDataFail",
                        "OKAO result=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    INT32 numMergeData = 0;
    okaoResult = OKAO_FR_GetRegisteredUsrDataNum(_okaoFaceAlbum, (INT32)mergeID, &numMergeData);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.MergeFaces.GetNumMergeDataFail",
                        "OKAO result=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    // TODO: Better way of deciding which (not just how many) registered data to keep
    
    while((numMergeData+numKeepData) >= MaxAlbumDataPerFace)
    {
      --numMergeData;
      if((numMergeData+numKeepData) < MaxAlbumDataPerFace) {
        break;
      }
      --numKeepData;
    }
    ASSERT_NAMED((numMergeData+numKeepData) < MaxAlbumDataPerFace,
                 "Total merge and keep data should be less than max data per face");
    
    
    for(s32 iMerge=0; iMerge < numMergeData; ++iMerge) {
      
      okaoResult = OKAO_FR_GetFeatureFromAlbum(_okaoFaceAlbum, (INT32)mergeID, iMerge,
                                               _okaoRecogMergeFeatureHandle);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.MergeFaces.GetFeatureFail",
                          "OKAO result=%d", okaoResult);
        return RESULT_FAIL;
      }
      
      okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, _okaoRecogMergeFeatureHandle,
                                        (INT32)keepID, numKeepData + iMerge);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.MergeFaces.RegisterDataFail",
                          "OKAO result=%d", okaoResult);
        return RESULT_FAIL;
      }
    }
    
    
    // Merge the enrollment data
    _mutex.lock();
    auto keepEnrollIter = _enrollmentData.find(keepID);
    auto mergeEnrollIter = _enrollmentData.find(mergeID);
    ASSERT_NAMED(keepEnrollIter != _enrollmentData.end(),
                 "ID to keep should have enrollment data entry");
    ASSERT_NAMED(mergeEnrollIter != _enrollmentData.end(),
                 "ID to merge should have enrollment data entry");
    if(!mergeEnrollIter->second.name.empty())
    {
      if(keepEnrollIter->second.name.empty()) {
        // The record we are merging into has no name set, but the one we're
        // merging from does. Use the latter.
        keepEnrollIter->second.name = mergeEnrollIter->second.name;
      } else {
        // Both records have a name! Issue a message.
        PRINT_NAMED_WARNING("FaceRecognizer.MergeUsers.OverwritingNamedUser",
                            "Removing name '%s' and merging its records into '%s'",
                            mergeEnrollIter->second.name.c_str(),
                            keepEnrollIter->second.name.c_str());
      }
    }
    keepEnrollIter->second.oldestData = std::min(keepEnrollIter->second.oldestData,
                                                 mergeEnrollIter->second.oldestData);
    
    keepEnrollIter->second.enrollmentTime = std::min(keepEnrollIter->second.enrollmentTime,
                                                     mergeEnrollIter->second.enrollmentTime);
    
    keepEnrollIter->second.lastScore = std::max(keepEnrollIter->second.lastScore,
                                                mergeEnrollIter->second.lastScore);
    _mutex.unlock();
    
    
    // After merging it, remove the old user that just got merged
    // TODO: check return val
    RemoveUser((INT32)mergeID);
    
    return RESULT_OK;
    
  } // MergeFaces()
  
  
  std::vector<u8> FaceRecognizer::GetSerializedAlbum()
  {
    std::vector<u8> serializedAlbum;
    
    if(_isInitialized)
    {
      UINT32 albumSize = 0;
      INT32 okaoResult = OKAO_FR_GetSerializedAlbumSize(_okaoFaceAlbum, &albumSize);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.GetSerializedAlbum.GetSizeFail",
                          "OKAO Result=%d", okaoResult);
      } else {
        serializedAlbum.resize(albumSize);
        okaoResult = OKAO_FR_SerializeAlbum(_okaoFaceAlbum, &(serializedAlbum[0]), albumSize);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.GetSerializedAlbum.SerializeFail",
                            "OKAO Result=%d", okaoResult);
        }
      }
    } else {
      PRINT_NAMED_ERROR("FaceTrackerImpl.GetSerializedAlbum.NotInitialized", "");
    }
    
    return serializedAlbum;
  } // GetSerializedAlbum()
  
  
  Result FaceRecognizer::SetSerializedAlbum(HCOMMON okaoCommonHandle, const std::vector<u8>&serializedAlbum)
  {
    FR_ERROR error;
    HALBUM restoredAlbum = OKAO_FR_RestoreAlbum(okaoCommonHandle, const_cast<UINT8*>(&(serializedAlbum[0])), (UINT32)serializedAlbum.size(), &error);
    if(NULL == restoredAlbum) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.SetSerializedAlbum.RestoreFail",
                        "OKAO Result=%d", error);
      return RESULT_FAIL;
    }
    
    INT32 okaoResult = OKAO_FR_DeleteAlbumHandle(_okaoFaceAlbum);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.SetSerializedAlbum.DeleteOldAlbumFail",
                        "OKAO Result=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    _okaoFaceAlbum = restoredAlbum;
    for(INT32 iUser=0; iUser<MaxAlbumFaces; ++iUser) {
      BOOL isRegistered = false;
      okaoResult = OKAO_FR_IsRegistered(_okaoFaceAlbum, iUser, 0, &isRegistered);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.SetSerializedAlbum.IsRegisteredCheckFail",
                          "OKAO Result=%d", okaoResult);
        return RESULT_FAIL;
      }
      //if(isRegistered) {
      //  _enrollmentStatus[iUser].loadedFromFile = true;
      //}
    }
    
    return RESULT_OK;
  } // SetSerializedAlbum()
  
  
  void FaceRecognizer::AssignNameToID(TrackedFace::ID_t faceID, const std::string& name)
  {
    // TODO: Queue the faceID / name pair for setting when next available, so we don't hang waiting to get mutex lock if we're in the middle of recognition
    
    _mutex.lock();
    auto iter = _enrollmentData.find(faceID);
    if(iter != _enrollmentData.end()) {
      iter->second.name = name;
    } else {
      PRINT_NAMED_ERROR("FaceTrackerImpl.AssignNameToID.InvalidID",
                        "Unknown ID %llu, ignoring name %s", faceID, name.c_str());
    }
    _mutex.unlock();
  }
  
  
  Result FaceRecognizer::SaveAlbum(const std::string &albumName)
  {
    Result result = RESULT_OK;
    _mutex.lock();
    
    std::vector<u8> serializedAlbum = GetSerializedAlbum();
    if(serializedAlbum.empty()) {
      PRINT_NAMED_ERROR("FaceTracker.SaveAlbum.EmptyAlbum",
                        "No serialized data returned from private implementation");
      result = RESULT_FAIL;
    } else {
      
      if(false == Util::FileUtils::CreateDirectory(albumName, false, true)) {
        PRINT_NAMED_ERROR("FaceTracker.SaveAlbum.DirCreationFail",
                          "Tried to create: %s", albumName.c_str());
        result = RESULT_FAIL;
      } else {
        
        const std::string dataFilename(albumName + "/data.bin");
        std::fstream fs;
        fs.open(dataFilename, std::ios::binary | std::ios::out);
        if(!fs.is_open()) {
          PRINT_NAMED_ERROR("FaceTracker.SaveAlbum.FileOpenFail", "Filename: %s", dataFilename.c_str());
          result = RESULT_FAIL;
        } else {
          
          fs.write((const char*)&(serializedAlbum[0]), serializedAlbum.size());
          fs.close();
          
          if((fs.rdstate() & std::ios::badbit) || (fs.rdstate() & std::ios::failbit)) {
            PRINT_NAMED_ERROR("FaceTracker.SaveAlbum.FileWriteFail", "Filename: %s", dataFilename.c_str());
            result = RESULT_FAIL;
          } else {
            Json::Value json;
            for(auto & enrollData : _enrollmentData)
            {
              Json::Value entry;
              entry["name"]              = enrollData.second.name;
              entry["oldestDataIndex"]   = enrollData.second.oldestData;
              entry["enrollmentTime"]    = (Json::LargestInt)enrollData.second.enrollmentTime;
              
              json[std::to_string(enrollData.first)] = entry;
            }
            
            const std::string enrollDataFilename(albumName + "/enrollData.json");
            Json::FastWriter writer;
            fs.open(enrollDataFilename, std::ios::out);
            if (!fs.is_open()) {
              PRINT_NAMED_ERROR("FaceTracker.SaveAlbum.EnrollDataFileOpenFail", "");
              result = RESULT_FAIL;
            } else {
              fs << writer.write(json);
              fs.close();
            }
          }
        }
      }
    }
    
    _mutex.unlock();
    return result;
  } // SaveAlbum()
  
  
  Result FaceRecognizer::LoadAlbum(HCOMMON okaoCommonHandle, const std::string& albumName)
  {
    Result result = RESULT_OK;
    
    _mutex.lock();
    
    const std::string dataFilename(albumName + "/data.bin");
    std::ifstream fs(dataFilename, std::ios::in | std::ios::binary);
    if(!fs.is_open()) {
      PRINT_NAMED_ERROR("FaceTracker.LoadAlbum.FileOpenFail", "Filename: %s", dataFilename.c_str());
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
        PRINT_NAMED_ERROR("FaceTracker.LoadAlbum.FileReadFail", "Filename: %s", dataFilename.c_str());
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
            PRINT_NAMED_ERROR("FaceTracker.LoadAlbum.EnrollDataFileReadFail", "");
            result = RESULT_FAIL;
          } else {
            _enrollmentData.clear();
            for(auto & idStr : json.getMemberNames()) {
              TrackedFace::ID_t faceID = std::stol(idStr);
              if(!json.isMember(idStr)) {
                PRINT_NAMED_ERROR("FaceTrackerImpl.LoadAlbum.BadFaceIdString",
                                  "Could not find member for string %s with value %llu",
                                  idStr.c_str(), faceID);
                _mutex.unlock();
                result = RESULT_FAIL;
              } else {
                const Json::Value& entry = json[idStr];
                EnrollmentData enrollData;
                enrollData.enrollmentTime    = (time_t)entry["enrollmentTime"].asLargestInt();
                enrollData.oldestData        = entry["oldestData"].asInt();
                enrollData.name              = entry["name"].asString();
                
                _enrollmentData[faceID] = enrollData;
                
                PRINT_NAMED_INFO("FaceTracker.LoadAlbum.LoadedEnrollmentData", "ID=%llu, Name: '%s'",
                                 faceID, enrollData.name.c_str());
              }
            }
          }
        }
      }
    }
    
    _mutex.unlock();
    return result;
  }
  
} // namespace Vision
} // namespace Anki

#endif // #if FACE_TRACKER_PROVIDER == FACE_TRACKER_OKAO

