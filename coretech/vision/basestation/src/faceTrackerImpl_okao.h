/**
 * File: faceTrackerImpl_okao.h
 *
 * Author: Andrew Stein
 * Date:   11/30/2015
 *
 * Description: Wrapper for OKAO Vision face detection library.
 *
 * NOTE: This file should only be included by faceTracker.cpp
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/faceTracker.h"
#include "anki/vision/basestation/trackedFace.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/math/rotation.h"
#include "util/logging/logging.h"
#include "util/helpers/boundedWhile.h"

// Omron OKAO Vision
#include <OkaoAPI.h>
#include <OkaoDtAPI.h> // Face Detection
#include <OkaoPtAPI.h> // Face parts detection
#include <OkaoExAPI.h> // Expression recognition
#include <OkaoFrAPI.h> // Face Recognition
#include <CommonDef.h>
#include <DetectorComDef.h>

#include <list>
#include <ctime>
#include <thread>
#include <mutex>

#define SHOW_TIMING 1

#if SHOW_TIMING
#  include <chrono>
#  include <unordered_map>
#endif

namespace Anki {

#if SHOW_TIMING
  class Profiler
  {
  public:
    
    // Calls Print() on destruction
    ~Profiler();
    
    // Start named timer (create if first call, un-pause otherwise)
    void Tic(const char* timerName);
    
    // Pause named timer and return time since last tic in milliseconds
    double Toc(const char* timerName);
    
    // Pause named timer and return average time in milliseconds for all tic/toc
    // pairs up to now
    double AverageToc(const char* timerName);
    
    // Print average time per tic/toc pair for each registered timer
    void Print() const;
    
  private:
    
    struct Timer {
      std::chrono::time_point<std::chrono::system_clock> startTime;
      std::chrono::milliseconds currentTime;
      std::chrono::milliseconds totalTime = std::chrono::milliseconds(0);
      s32 count = 0;
      
      void Update();
    };
    
    using TimerContainer = std::unordered_map<const char*, Profiler::Timer>;
    
    TimerContainer _timers;
    
  }; // class Profiler
  
  void Profiler::Tic(const char* timerName)
  {
    // Note will construct timer if matching name doesn't already exist
    _timers[timerName].startTime = std::chrono::system_clock::now();
  }
  
  void Profiler::Timer::Update()
  {
    using namespace std::chrono;
    currentTime = duration_cast<milliseconds>(system_clock::now() - startTime);
    totalTime += currentTime;
    ++count;
  }
  
  double Profiler::Toc(const char* timerName)
  {
    auto timerIter = _timers.find(timerName);
    if(timerIter != _timers.end()) {
      Timer& timer = timerIter->second;
      timer.Update();
      return (u32)timer.currentTime.count();
    } else {
      return 0;
    }
  }
  
  double Profiler::AverageToc(const char* timerName)
  {
    auto timerIter = _timers.find(timerName);
    if(timerIter != _timers.end()) {
      Timer& timer = timerIter->second;
      timer.Update();
      if(timer.count > 0) {
        return (double)(timer.totalTime.count() / (double)timer.count);
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  }
  
  Profiler::~Profiler()
  {
    Print();
  }
  
  void Profiler::Print() const
  {
    for(auto & timerPair : _timers)
    {
      const Timer& timer = timerPair.second;
      PRINT_NAMED_INFO("Profiler", "%s averaged %.2fms over %d calls",
                       timerPair.first,
                       (timer.count > 0 ? (double)timer.totalTime.count() / (double)timer.count : 0),
                       timer.count);
    }
  }
#else 
  class Profiler {
  public:
    void Tic(const char*) { }
    void Toc(const char*) { }
  };
  
#endif // SHOW_TIMING
  
namespace Vision {

#pragma mark -
#pragma mark FaceRecognizer

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
    // otherwise.
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
    
    HFEATURE    _okaoRecognitionFeatureHandle  = NULL;
    HFEATURE    _okaoRecogMergeFeatureHandle   = NULL;
    HALBUM      _okaoFaceAlbum                 = NULL;
    
    std::mutex  _mutex;
    std::thread _recognitionThread;
    bool _recognitionRunning = false;
    void Run();
    
    INT32         _currentTrackerID = -1;
    Vision::Image _img;
    HPTRESULT     _okaoPartDetectionResultHandle = NULL;
    
    std::map<INT32, Vision::TrackedFace::ID_t> _trackingToFaceID;
    
    std::list<INT32> _trackerIDsToRemove;
    
    INT32 _lastRegisteredUserID = 0;
    bool _enrollNewFaces = true;
    
    // Track the index of the oldest data for each registered user, so we can
    // replace the oldest one each time
    struct EnrollmentData {
      INT32        oldestData = 0;
      time_t       enrollmentTime;
      std::string  name;
      INT32        lastScore = 0;
    };
    std::map<Vision::TrackedFace::ID_t,EnrollmentData> _enrollmentData;
    
    
  }; // class FaceRecognizer
  
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
      const Vision::TrackedFace::ID_t faceID = iter->second;
      auto const& enrollData = _enrollmentData.at(faceID);
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
  
#pragma mark -
#pragma mark FaceTracker::Impl
  
  class FaceTracker::Impl : public Profiler
  {
  public:
    Impl(const std::string& modelPath, FaceTracker::DetectionMode mode);
    ~Impl();
    
    Result Update(const Vision::Image& frameOrig);
    
    std::list<TrackedFace> GetFaces() const;
    
    void EnableDisplay(bool enabled) { }
    
    static bool IsRecognitionSupported() { return true; }
    
    void EnableNewFaceEnrollment(bool enable) { _recognizer.EnableNewFaceEnrollment(enable); }
    bool IsNewFaceEnrollmentEnabled() const   { return _recognizer.IsNewFaceEnrollmentEnabled(); }
    
    void EnableEmotionDetection(bool enable) { _detectEmotion = enable; }
    bool IsEmotionDetectionEnabled() const   { return _detectEmotion;  }

    void AssignNameToID(TrackedFace::ID_t faceID, const std::string& name);
    
    Result LoadAlbum(const std::string& albumName);
    Result SaveAlbum(const std::string& albumName);

  private:
    
    Result Init();

    bool   DetectFaceParts(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                           INT32 detectionIndex, Vision::TrackedFace& face);
    
    Result EstimateExpression(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                              Vision::TrackedFace& face);
  
    bool _isInitialized = false;
    FaceTracker::DetectionMode _detectionMode;
    bool _detectEmotion = true;
    
    static const s32   MaxFaces = 10; // detectable at once
    
    std::list<TrackedFace> _faces;
    
    // Mapping from tracking ID to recognition (identity) ID
    struct TrackingData {
      Vision::TrackedFace::ID_t assignedID = Vision::TrackedFace::UnknownFace;
    };
    std::map<INT32, TrackingData> _trackingData;
    
    //u8* _workingMemory = nullptr;
    //u8* _backupMemory  = nullptr;
    
    // Okao Vision Library "Handles"
    // Note that we have two actual handles for part detection so that we can
    // hand one to the FaceRecognizer and let it churn on that while we use the
    // other one for part detection in the mean time.
    HCOMMON     _okaoCommonHandle               = NULL;
    HDETECTION  _okaoDetectorHandle             = NULL;
    HDTRESULT   _okaoDetectionResultHandle      = NULL;
    HPOINTER    _okaoPartDetectorHandle         = NULL;
    HPTRESULT   _okaoPartDetectionResultHandle  = NULL;
    HPTRESULT   _okaoPartDetectionResultHandle2 = NULL;
    HEXPRESSION _okaoEstimateExpressionHandle   = NULL;
    HEXPRESSION _okaoExpressionResultHandle     = NULL;
    
    // Space for detected face parts / expressions
    POINT _facialParts[PT_POINT_KIND_MAX];
    INT32 _facialPartConfs[PT_POINT_KIND_MAX];
    INT32 _expressionValues[EX_EXPRESSION_KIND_MAX];
    
    // Runs on a separate thread
    FaceRecognizer _recognizer;
    
  }; // class FaceTracker::Impl
  
  
  FaceTracker::Impl::Impl(const std::string& modelPath, FaceTracker::DetectionMode mode)
  : _detectionMode(mode)
  {
    
  } // Impl Constructor()
  
  Result FaceTracker::Impl::Init()
  {
    // Get and pring Okao library version as a sanity check that we can even
    // talk to the library
    UINT8 okaoVersionMajor=0, okaoVersionMinor = 0;
    INT32 okaoResult = OKAO_CO_GetVersion(&okaoVersionMajor, &okaoVersionMinor);
    if(okaoResult != OKAO_NORMAL) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoVersionFail", "");
      return RESULT_FAIL;
    }
    PRINT_NAMED_INFO("FaceTrackerImpl.Init.OkaoVersion",
                     "Initializing with OkaoVision version %d.%d",
                     okaoVersionMajor, okaoVersionMinor);
    
    /*
    // Allocate memory for the Okao libraries
    _workingMemory = new u8[MaxFaces*800 + 60*1024]; // 0.7KB × [max detection count] + 60KB
    _backupMemory  = new u8[MaxFaces*4096 + (250+70)*1024];
    
    if(_workingMemory == nullptr || _backupMemory == nullptr) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.MemoryAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    */
    
    _okaoCommonHandle = OKAO_CO_CreateHandle();
    if(NULL == _okaoCommonHandle) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoCommonHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }

    switch(_detectionMode)
    {
      case FaceTracker::DetectionMode::Video:
      {
        _okaoDetectorHandle = OKAO_DT_CreateHandle(_okaoCommonHandle, DETECTION_MODE_MOVIE, MaxFaces);
        if(NULL == _okaoDetectorHandle) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoDetectionHandleAllocFail.VideoMode", "");
          return RESULT_FAIL_MEMORY;
        }
        
        // Adjust some detection parameters
        // TODO: Expose these for setting at runtime
        okaoResult = OKAO_DT_MV_SetDelayCount(_okaoDetectorHandle, 1); // have to see faces for more than one frame
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetDelayCountFailed", "");
          return RESULT_FAIL_INVALID_PARAMETER;
        }
        
        okaoResult = OKAO_DT_MV_SetSearchCycle(_okaoDetectorHandle, 2, 2, 5);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetSearchCycleFailed", "");
          return RESULT_FAIL_INVALID_PARAMETER;
        }

        okaoResult = OKAO_DT_MV_SetDirectionMask(_okaoDetectorHandle, false);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetDirectionMaskFailed", "");
          return RESULT_FAIL_INVALID_PARAMETER;
        }
        
        okaoResult = OKAO_DT_MV_SetPoseExtension(_okaoDetectorHandle, true, true);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetPoseExtensionFailed", "");
          return RESULT_FAIL_INVALID_PARAMETER;
        }
        
        okaoResult = OKAO_DT_MV_SetAccuracy(_okaoDetectorHandle, TRACKING_ACCURACY_HIGH);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetAccuracyFailed", "");
          return RESULT_FAIL_INVALID_PARAMETER;
        }
        
        break;
      }
        
      case FaceTracker::DetectionMode::SingleImage:
        _okaoDetectorHandle = OKAO_DT_CreateHandle(_okaoCommonHandle, DETECTION_MODE_STILL, MaxFaces);
        if(NULL == _okaoDetectorHandle) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoDetectionHandleAllocFail.StillMode", "");
          return RESULT_FAIL_MEMORY;
        }
        break;
        
      default:
        PRINT_NAMED_ERROR("FaceTrackerImpl.Init.UnknownDetectionMode",
                          "Requested mode = %hhu", _detectionMode);
        return RESULT_FAIL;
    }
    
    //okaoResult = OKAO_DT_SetAngle(_okaoDetectorHandle, POSE_ANGLE_HALF_PROFILE,
    //                              ROLL_ANGLE_U45 | ROLL_ANGLE_2 | ROLL_ANGLE_10);
    okaoResult = OKAO_DT_SetAngle(_okaoDetectorHandle, POSE_ANGLE_FRONT, ROLL_ANGLE_U45);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetAngleFailed", "");
      return RESULT_FAIL_INVALID_PARAMETER;
    }
    
    okaoResult = OKAO_DT_SetSizeRange(_okaoDetectorHandle, 60, 640);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetSizeRangeFailed", "");
      return RESULT_FAIL_INVALID_PARAMETER;
    }
    
    okaoResult = OKAO_DT_SetThreshold(_okaoDetectorHandle, 500);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetThresholdFailed", "");
      return RESULT_FAIL_INVALID_PARAMETER;
    }
    
    _okaoDetectionResultHandle = OKAO_DT_CreateResultHandle(_okaoCommonHandle);
    if(NULL == _okaoDetectionResultHandle) {
      PRINT_NAMED_ERROR("FacetrackerImpl.Init.OkaoDetectionResultHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoPartDetectorHandle = OKAO_PT_CreateHandle(_okaoCommonHandle);
    if(NULL == _okaoPartDetectorHandle) {
      PRINT_NAMED_ERROR("FacetrackerImpl.Init.OkaoPartDetectorHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoPartDetectionResultHandle = OKAO_PT_CreateResultHandle(_okaoCommonHandle);
    if(NULL == _okaoPartDetectionResultHandle) {
      PRINT_NAMED_ERROR("FacetrackerImpl.Init.OkaoPartDetectionResultHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }

    _okaoPartDetectionResultHandle2 = OKAO_PT_CreateResultHandle(_okaoCommonHandle);
    if(NULL == _okaoPartDetectionResultHandle2) {
      PRINT_NAMED_ERROR("FacetrackerImpl.Init.OkaoPartDetectionResultHandle2AllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoEstimateExpressionHandle = OKAO_EX_CreateHandle(_okaoCommonHandle);
    if(NULL == _okaoEstimateExpressionHandle) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoEstimateExpressionHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoExpressionResultHandle = OKAO_EX_CreateResultHandle(_okaoCommonHandle);
    if(NULL == _okaoExpressionResultHandle) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoExpressionResultHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }

    Result recognizerInitResult = _recognizer.Init(_okaoCommonHandle);
    
    if(RESULT_OK == recognizerInitResult) {
      
      _isInitialized = true;
      
      PRINT_NAMED_INFO("FaceTrackerImpl.Init.Success",
                       "OKAO Vision handles created successfully.");
    }
    
    return recognizerInitResult;
  } // Init()
  
  FaceTracker::Impl::~Impl()
  {
    //Util::SafeDeleteArray(_workingMemory);
    //Util::SafeDeleteArray(_backupMemory);

    
    if(NULL != _okaoExpressionResultHandle) {
      if(OKAO_NORMAL != OKAO_EX_DeleteResultHandle(_okaoExpressionResultHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoExpressionResultHandleDeleteFail", "");
      }
    }
    
    if(NULL != _okaoEstimateExpressionHandle) {
      if(OKAO_NORMAL != OKAO_EX_DeleteHandle(_okaoEstimateExpressionHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoEstimateExpressionHandleDeleteFail", "");
      }
    }
    
    if(NULL != _okaoPartDetectionResultHandle) {
      if(OKAO_NORMAL != OKAO_PT_DeleteResultHandle(_okaoPartDetectionResultHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoPartDetectionResultHandle1DeleteFail", "");
      }
    }
    
    if(NULL != _okaoPartDetectionResultHandle2) {
      if(OKAO_NORMAL != OKAO_PT_DeleteResultHandle(_okaoPartDetectionResultHandle2)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoPartDetectionResultHandle2DeleteFail", "");
      }
    }
    
    if(NULL != _okaoPartDetectorHandle) {
      if(OKAO_NORMAL != OKAO_PT_DeleteHandle(_okaoPartDetectorHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoPartDetectorHandleDeleteFail", "");
      }
    }

    if(NULL != _okaoDetectionResultHandle) {
      if(OKAO_NORMAL != OKAO_DT_DeleteResultHandle(_okaoDetectionResultHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoDetectionResultHandleDeleteFail", "");
      }
    }
    
    if(NULL != _okaoDetectorHandle) {
      if(OKAO_NORMAL != OKAO_DT_DeleteHandle(_okaoDetectorHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoDetectorHandleDeleteFail", "");
      }
      _okaoDetectorHandle = NULL;
    }
    
    if(NULL != _okaoCommonHandle) {
      if(OKAO_NORMAL != OKAO_CO_DeleteHandle(_okaoCommonHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoCommonHandleDeleteFail", "");
      }
      _okaoCommonHandle = NULL;
    }
    
    _isInitialized = false;
  } // ~Impl()
  
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
        Vision::TrackedFace::ID_t faceID = Vision::TrackedFace::UnknownFace;
        auto iter = _trackingToFaceID.find(_currentTrackerID);
        if(iter != _trackingToFaceID.end()) {
          faceID = iter->second;
        }
        _mutex.unlock();
        
        Vision::TrackedFace::ID_t recognizedID = Vision::TrackedFace::UnknownFace;
        Result result = RecognizeFace(nWidth, nHeight, dataPtr, recognizedID, score);
        
        if(RESULT_OK == result)
        {
          // TODO: Tic("FaceRecognition");
          
         if(Vision::TrackedFace::UnknownFace == recognizedID) {
            // We did not recognize the tracked face in its current position, just leave
            // the faceID alone
            
            // (Nothing to do)
            
          } else if(Vision::TrackedFace::UnknownFace == faceID) {
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
              
              Vision::TrackedFace::ID_t mergeTo, mergeFrom;
              
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
          if(Vision::TrackedFace::UnknownFace != faceID) {
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
  }
  
  static inline void SetFeatureHelper(const POINT* faceParts, std::vector<s32>&& indices,
                                      TrackedFace::FeatureName whichFeature,
                                      TrackedFace& face)
  {
    TrackedFace::Feature feature;
    bool allPointsPresent = true;
    for(auto index : indices) {
      if(faceParts[index].x == FEATURE_NO_POINT ||
         faceParts[index].y == FEATURE_NO_POINT)
      {
        allPointsPresent = false;
        break;
      }
      feature.emplace_back(faceParts[index].x, faceParts[index].y);
    }
    
    if(allPointsPresent) {
      face.SetFeature(whichFeature, std::move(feature));
    }
  } // SetFeatureHelper()
  

  Result FaceRecognizer::RegisterNewUser(HFEATURE& hFeature)
  {
    INT32 numUsersInAlbum = 0;
    INT32 okaoResult = OKAO_FR_GetRegisteredUserNum(_okaoFaceAlbum, &numUsersInAlbum);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.RegisterNewUser.OkaoGetNumUsersInAlbumFailed", "");
      return RESULT_FAIL;
    }
    
    if(numUsersInAlbum >= MaxAlbumFaces) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.RegisterNewUser.TooManyUsers",
                        "Already have %d users, could not add another", MaxAlbumFaces);
      return RESULT_FAIL;
    }

    // Find unused ID
    BOOL isRegistered = true;
    auto tries = 0;
    do {
      okaoResult = OKAO_FR_IsRegistered(_okaoFaceAlbum, _lastRegisteredUserID, 0, &isRegistered);
      
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.RegisterNewUser.IsRegisteredCheckFailed",
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
      PRINT_NAMED_ERROR("FaceTrackerImpl.RegisturNewUser.NoIDsAvailable",
                        "Could not find a free ID to use for new face");
      return RESULT_FAIL;
    }
    
    okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, hFeature, _lastRegisteredUserID, 0);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.RegisterNewUser.RegisterFailed",
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
  
  
  bool FaceTracker::Impl::DetectFaceParts(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                                          INT32 detectionIndex,
                                          Vision::TrackedFace& face)
  {
    INT32 okaoResult = OKAO_PT_SetPositionFromHandle(_okaoPartDetectorHandle, _okaoDetectionResultHandle, detectionIndex);
    
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoSetPositionFail",
                        "OKAO Result Code=%d", okaoResult);
      return false;
    }
    okaoResult = OKAO_PT_DetectPoint_GRAY(_okaoPartDetectorHandle, dataPtr,
                                          nWidth, nHeight, GRAY_ORDER_Y0Y1Y2Y3, _okaoPartDetectionResultHandle);
    
    if(OKAO_NORMAL != okaoResult) {
      if(OKAO_ERR_PROCESSCONDITION != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoPartDetectionFail",
                          "OKAO Result Code=%d", okaoResult);
      }
      return false;
    }
    
    okaoResult = OKAO_PT_GetResult(_okaoPartDetectionResultHandle, PT_POINT_KIND_MAX,
                                   _facialParts, _facialPartConfs);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetFacePartResultFail",
                        "OKAO Result Code=%d", okaoResult);
      return false;
    }
    
    // Set eye centers
    face.SetLeftEyeCenter(Point2f(_facialParts[PT_POINT_LEFT_EYE].x,
                                  _facialParts[PT_POINT_LEFT_EYE].y));
    
    face.SetRightEyeCenter(Point2f(_facialParts[PT_POINT_RIGHT_EYE].x,
                                   _facialParts[PT_POINT_RIGHT_EYE].y));
    
    // Set other facial features
    SetFeatureHelper(_facialParts, {
      PT_POINT_LEFT_EYE_OUT, PT_POINT_LEFT_EYE, PT_POINT_LEFT_EYE_IN
    }, TrackedFace::FeatureName::LeftEye, face);
    
    SetFeatureHelper(_facialParts, {
      PT_POINT_RIGHT_EYE_IN, PT_POINT_RIGHT_EYE, PT_POINT_RIGHT_EYE_OUT
    }, TrackedFace::FeatureName::RightEye, face);
    
    SetFeatureHelper(_facialParts, {
      PT_POINT_NOSE_LEFT, PT_POINT_NOSE_RIGHT
    }, TrackedFace::FeatureName::Nose, face);
    
    SetFeatureHelper(_facialParts, {
      PT_POINT_MOUTH_LEFT, PT_POINT_MOUTH_UP, PT_POINT_MOUTH_RIGHT,
      PT_POINT_MOUTH, PT_POINT_MOUTH_LEFT,
    }, TrackedFace::FeatureName::UpperLip, face);
    
    
    // Fill in head orientation
    INT32 roll_deg=0, pitch_deg=0, yaw_deg=0;
    okaoResult = OKAO_PT_GetFaceDirection(_okaoPartDetectionResultHandle, &pitch_deg, &yaw_deg, &roll_deg);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetFaceDirectionFail",
                        "OKAO Result Code=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    //PRINT_NAMED_INFO("FaceTrackerImpl.Update.HeadOrientation",
    //                 "Roll=%ddeg, Pitch=%ddeg, Yaw=%ddeg",
    //                 roll_deg, pitch_deg, yaw_deg);
    
    face.SetHeadOrientation(DEG_TO_RAD(roll_deg),
                            DEG_TO_RAD(pitch_deg),
                            DEG_TO_RAD(yaw_deg));
    
    return true;
  }
  
  Result FaceTracker::Impl::EstimateExpression(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                                               Vision::TrackedFace& face)
  {
    INT32 okaoResult = OKAO_EX_SetPointFromHandle(_okaoEstimateExpressionHandle, _okaoPartDetectionResultHandle);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoSetExpressionPointFail",
                        "OKAO Result Code=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    okaoResult = OKAO_EX_Estimate_GRAY(_okaoEstimateExpressionHandle, dataPtr, nWidth, nHeight,
                                       GRAY_ORDER_Y0Y1Y2Y3, _okaoExpressionResultHandle);
    if(OKAO_NORMAL != okaoResult) {
      if(OKAO_ERR_PROCESSCONDITION == okaoResult) {
        // This might happen, depending on face parts
        PRINT_NAMED_INFO("FaceTrackerImpl.Update.OkaoEstimateExpressionNotPossible", "");
      } else {
        // This should not happen
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoEstimateExpressionFail",
                          "OKAO Result Code=%d", okaoResult);
        return RESULT_FAIL;
      }
    } else {
      
      okaoResult = OKAO_EX_GetResult(_okaoExpressionResultHandle, EX_EXPRESSION_KIND_MAX, _expressionValues);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetExpressionResultFail",
                          "OKAO Result Code=%d", okaoResult);
        return RESULT_FAIL;
      }
      
      static const TrackedFace::Expression TrackedFaceExpressionLUT[EX_EXPRESSION_KIND_MAX] = {
        TrackedFace::Neutral,
        TrackedFace::Happiness,
        TrackedFace::Surprise,
        TrackedFace::Anger,
        TrackedFace::Sadness
      };
      
      for(INT32 okaoExpressionVal = 0; okaoExpressionVal < EX_EXPRESSION_KIND_MAX; ++okaoExpressionVal) {
        face.SetExpressionValue(TrackedFaceExpressionLUT[okaoExpressionVal],
                                _expressionValues[okaoExpressionVal]);
      }
      
    }

    return RESULT_OK;
  } // EstimateExpression()
  
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
                                       Vision::TrackedFace::ID_t& faceID, INT32& recognitionScore)
  {
    INT32 okaoResult = OKAO_FR_ExtractHandle_GRAY(_okaoRecognitionFeatureHandle,
                                                  dataPtr, nWidth, nHeight, GRAY_ORDER_Y0Y1Y2Y3,
                                                  _okaoPartDetectionResultHandle);
    
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
      okaoResult = OKAO_FR_Identify(_okaoRecognitionFeatureHandle, _okaoFaceAlbum, MaxResults, userIDs, scores, &resultNum);
      
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
  
  Result FaceRecognizer::MergeFaces(Vision::TrackedFace::ID_t keepID,
                                    Vision::TrackedFace::ID_t mergeID)
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
  
  Result FaceTracker::Impl::Update(const Vision::Image& frameOrig)
  {
    // Initialize on first use
    if(!_isInitialized) {
      Result initResult = Init();
      
      if(!_isInitialized || initResult != RESULT_OK) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.InitFailed", "");
        return RESULT_FAIL;
      }
    }
    
    ASSERT_NAMED(frameOrig.IsContinuous(),
                 "Image must be continuous to pass straight into OKAO Detector");
    
    INT32 okaoResult = OKAO_NORMAL;
    //TIC;
    Tic("FaceDetect");
    const INT32 nWidth  = frameOrig.GetNumCols();
    const INT32 nHeight = frameOrig.GetNumRows();
    RAWIMAGE* dataPtr = const_cast<UINT8*>(frameOrig.GetDataPointer());
    okaoResult = OKAO_DT_Detect_GRAY(_okaoDetectorHandle, dataPtr, nWidth, nHeight,
                                     GRAY_ORDER_Y0Y1Y2Y3, _okaoDetectionResultHandle);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoDetectFail", "OKAO Result Code=%d",
                        okaoResult);
      return RESULT_FAIL;
    }
    
    INT32 numDetections = 0;
    okaoResult = OKAO_DT_GetResultCount(_okaoDetectionResultHandle, &numDetections);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetResultCountFail",
                        "OKAO Result Code=%d", okaoResult);
      return RESULT_FAIL;
    }
    Toc("FaceDetect");
    
    _faces.clear();
    
    for(INT32 detectionIndex=0; detectionIndex<numDetections; ++detectionIndex)
    {
      DETECTION_INFO detectionInfo;
      okaoResult = OKAO_DT_GetRawResultInfo(_okaoDetectionResultHandle, detectionIndex,
                                            &detectionInfo);

      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetResultInfoFail",
                          "Detection index %d of %d. OKAO Result Code=%d",
                          detectionIndex, numDetections, okaoResult);
        return RESULT_FAIL;
      }
      
      // Add a new face to the list
      _faces.emplace_back();
      
      TrackedFace& face = _faces.back();

      const INT32 trackerID = detectionInfo.nID;
      if(detectionInfo.nDetectionMethod == DET_METHOD_DETECTED_HIGH) {
        // This face was found via detection
        auto iter = _trackingData.find(trackerID);
        if(iter != _trackingData.end()) {
          // The detector seems to be reusing an existing tracking ID we've seen
          // before. Remove it from the recognizer so we don't merge things inappropriately
          _recognizer.RemoveTrackingID(trackerID);
        }
        _trackingData[trackerID].assignedID = Vision::TrackedFace::UnknownFace;
        face.SetIsBeingTracked(false);
      } else {
        // This face was found via tracking: keep its existing recognition ID
        ASSERT_NAMED(_trackingData.find(trackerID) != _trackingData.end(),
                     "A tracked face should already have a recognition ID entry");
        face.SetIsBeingTracked(true);
      }
 
      POINT ptLeftTop, ptRightTop, ptLeftBottom, ptRightBottom;
      okaoResult = OKAO_CO_ConvertCenterToSquare(detectionInfo.ptCenter,
                                                 detectionInfo.nHeight,
                                                 0, &ptLeftTop, &ptRightTop,
                                                 &ptLeftBottom, &ptRightBottom);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoCenterToSquareFail",
                          "Detection index %d of %d. OKAO Result Code=%d",
                          detectionIndex, numDetections, okaoResult);
        return RESULT_FAIL;
      }
      
      face.SetRect(Rectangle<f32>(ptLeftTop.x, ptLeftTop.y,
                                  ptRightBottom.x-ptLeftTop.x,
                                  ptRightBottom.y-ptLeftTop.y));
      
      face.SetTimeStamp(frameOrig.GetTimestamp());
      
      // Try finding face parts
      Tic("FacePartDetection");
      bool facePartsFound = DetectFaceParts(nWidth, nHeight, dataPtr, detectionIndex, face);
      Toc("FacePartDetection");
      
      if(_detectEmotion && facePartsFound)
      {
        // Expression detection
        Tic("ExpressionRecognition");
        Result expResult = EstimateExpression(nWidth, nHeight, dataPtr, face);
        Toc("ExpressionRecognition");
        if(RESULT_OK != expResult) {
          PRINT_NAMED_WARNING("FaceTrackerImpl.Update.EstimateExpressiongFailed",
                              "Detection index %d of %d.",
                              detectionIndex, numDetections);
        }
      } // if(_detectEmotion)
      
      // Face Recognition:
      if(facePartsFound)
      {
        bool recognizing = _recognizer.SetNextFaceToRecognize(frameOrig, trackerID, _okaoPartDetectionResultHandle);
        if(recognizing) {
          // The FaceRecognizer is now using whatever the partDetectionResultHandle is pointing to.
          // Switch to using the other handle so we don't step on its toes.
          std::swap(_okaoPartDetectionResultHandle, _okaoPartDetectionResultHandle2);
        }
      }
      
      // Get whatever is the latest recognition information for the current tracker ID
      auto recognitionData = _recognizer.GetRecognitionData(trackerID);
      
      /*
      TrackingData& trackingData = _trackingData[trackerID];
      Vision::TrackedFace::ID_t faceID = trackingData.assignedID;
      INT32 recognitionScore = 0;
      if(facePartsFound)
      {
       
        // We've now assigned an identity to this tracker ID so we don't have to
        // keep trying to recognize it while we're successfully tracking!
        trackingData.assignedID = faceID;
        
        Toc("FaceRecognition");
      } // if(facePartsFound)
      */
      
      face.SetID(recognitionData.faceID); // could be unknown!
      face.SetScore(recognitionData.score); // could still be zero!
      if(TrackedFace::UnknownFace == recognitionData.faceID) {
        face.SetName("Unknown");
      } else {
        _trackingData[trackerID].assignedID = recognitionData.faceID;
        if(recognitionData.name.empty()) {
          // Known, unnamed face
          face.SetName("KnownFace" + std::to_string(recognitionData.faceID));
        } else {
          face.SetName(recognitionData.name);
        }
      }
      
    } // FOR each face
    
    return RESULT_OK;
  } // Update()
  
  std::list<TrackedFace> FaceTracker::Impl::GetFaces() const
  {
    return _faces;
  }
  
  
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
  

  void FaceTracker::Impl::AssignNameToID(TrackedFace::ID_t faceID, const std::string& name)
  {
    _recognizer.AssignNameToID(faceID, name);
  }
  
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
  
  Result FaceTracker::Impl::SaveAlbum(const std::string& albumName)
  {
    return _recognizer.SaveAlbum(albumName);
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
  
  Result FaceTracker::Impl::LoadAlbum(const std::string& albumName)
  {
    // Initialize on first use
    if(!_isInitialized) {
      Result initResult = Init();
      
      if(!_isInitialized || initResult != RESULT_OK) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.SetSerializedAlbum.InitFailed", "");
        return RESULT_FAIL;
      }
    }
    
    return _recognizer.LoadAlbum(_okaoCommonHandle, albumName);
  }
  
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
              Vision::TrackedFace::ID_t faceID = std::stol(idStr);
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

