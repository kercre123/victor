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

#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/eyeContact.h"
#include "coretech/vision/engine/faceNormalDirectedAtRobot.h"
#include "coretech/vision/engine/faceNormalDirectedAtRobot3d.h"
#include "coretech/vision/engine/trackedFace.h"
#include "coretech/vision/engine/profiler.h"

#include "clad/types/loadedKnownFace.h"

#include "faceRecognizer_okao.h"

// Omron OKAO Vision
#include "OkaoAPI.h"
#include "OkaoDtAPI.h" // Face Detection
#include "OkaoPtAPI.h" // Face parts detection
#include "OkaoExAPI.h" // Expression recognition
#include "OkaoSmAPI.h" // Smile estimation
#include "OkaoGbAPI.h" // Gaze & blink estimation
#include "CommonDef.h"
#include "DetectorComDef.h"

#include "json/json.h"

#include <list>
#include <ctime>

namespace Anki {
  
namespace Util {
  class RandomGenerator;
}
  
namespace Vision {

  class FaceTracker::Impl : public Profiler
  {
  public:
    Impl(const Camera&        camera,
         const std::string&   modelPath,
         const Json::Value&   config);
    ~Impl();
    
    void SetRecognitionIsSynchronous(bool isSynchronous);
    
    Result Update(const Vision::Image&        frameOrig,
                  std::list<TrackedFace>&     faces,
                  std::list<UpdatedFaceID>&   updatedIDs);
    
    // These methods allow to add or clear the contents of the set that
    // contains the face id's we're allowed to track. All other
    // face id's we should drop on the floor. We should also not perform
    // any face recognition when this set is populated. If this set is
    // empty we should proceed to track and recognize faces as usual.
    void AddAllowedTrackedFace(const FaceID_t faceID);
    bool HaveAllowedTrackedFaces() const { return !_allowedTrackedFaceID.empty(); }
    // This method calls Reset which clears all the allowed tracked
    // faces and also resets the face tracker as well
    void ClearAllowedTrackedFaces();
    
    void EnableDisplay(bool enabled) { }
    
    static bool IsRecognitionSupported() { return true; }
    static float GetMinEyeDistanceForEnrollment();
    
    void SetFaceEnrollmentMode(Vision::FaceEnrollmentPose pose,
                               Vision::FaceID_t forFaceID,
                               s32 numEnrollments);
																						      
    void EnableEmotionDetection(bool enable) { _detectEmotion = enable; }
    void EnableSmileDetection(bool enable)   { _detectSmiling = enable; }
    void EnableGazeDetection(bool enable)    { _detectGaze    = enable; }
    void EnableBlinkDetection(bool enable)   { _detectBlinks  = enable; }
    
    bool IsEmotionDetectionEnabled() const   { return _detectEmotion;  }
    bool IsSmileDetectionEnabled()   const   { return _detectSmiling;  }
    bool IsGazeDetectionEnabled()    const   { return _detectGaze;     }
    bool IsBlinkDetectionEnabled()   const   { return _detectBlinks;   }
    
    bool     CanAddNamedFace() const;
    Result   AssignNameToID(FaceID_t faceID, const std::string& name, FaceID_t mergeWithID);
    Result   EraseFace(FaceID_t faceID);
    void     EraseAllFaces();
    Result   RenameFace(FaceID_t faceID, const std::string& oldName, const std::string& newName,
                        Vision::RobotRenamedEnrolledFace& renamedFace);
    
    std::vector<LoadedKnownFace> GetEnrolledNames() const;

    Result LoadAlbum(const std::string& albumName, std::list<LoadedKnownFace>& loadedFaces);
    Result SaveAlbum(const std::string& albumName);

    Result GetSerializedData(std::vector<u8>& albumData,
                             std::vector<u8>& enrollData);

    Result SetSerializedData(const std::vector<u8>& albumData,
                             const std::vector<u8>& enrollData,
                             std::list<LoadedKnownFace>& loadedFaces);

  private:
    
    // Creates new face detectors using current parameters
    Result Init();

    // Destructs current face detectors
    void Deinit();

    bool   DetectFaceParts(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                           INT32 detectionIndex, Vision::TrackedFace& face);
    
    Result EstimateExpression(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                              TrackedFace& face);
  
    Result DetectSmile(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                       Vision::TrackedFace& face);
    
    Result DetectGazeAndBlink(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                              Vision::TrackedFace& face);

    bool DetectEyeContact(const TrackedFace& face,
                          const TimeStamp_t& frameOrig);
  
    void FaceDirection(TrackedFace& face, const TimeStamp_t& frameOrig);
    void FaceDirection3d(const TrackedFace& face, const TimeStamp_t& frameOrig);

    bool IsEnrollable(const DETECTION_INFO& detectionInfo, const TrackedFace& face, const f32 intraEyeDist);

    void Reset();
    
    // Setting the pose of the face uses the camera's pose as its parent
    Result SetFacePoseFromParts(const s32 nrows, const s32 ncols, TrackedFace& face, f32& intraEyeDist);
    Result SetFacePoseWithoutParts(const s32 nrows, const s32 ncols, TrackedFace& face, f32& intraEyeDist);
    
    bool _isInitialized = false;
    bool _detectEmotion = false;
    bool _detectSmiling = false;
    bool _detectGaze    = false;
    bool _detectBlinks  = false;
    
    Json::Value _config;

    const Camera& _camera;
    
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
    HSMILE      _okaoSmileDetectHandle          = NULL;
    HSMRESULT   _okaoSmileResultHandle          = NULL;
    HGAZEBLINK  _okaoGazeBlinkDetectHandle      = NULL;
    HGBRESULT   _okaoGazeBlinkResultHandle      = NULL;
    
    // Space for detected face parts / expressions
    POINT _facialParts[PT_POINT_KIND_MAX];
    INT32 _facialPartConfs[PT_POINT_KIND_MAX];
    INT32 _expressionValues[EX_EXPRESSION_KIND_MAX];
    
    FaceEnrollmentPose _enrollPose = FaceEnrollmentPose::LookingStraight;
    
    // Runs on a separate thread
    FaceRecognizer _recognizer;
    
    std::unique_ptr<Util::RandomGenerator> _rng;
    
    std::map<FaceID_t, EyeContact> _facesEyeContact;
    std::map<FaceID_t, FaceNormalDirectedAtRobot> _facesDirectedAtRobot;
    std::map<FaceID_t, FaceNormalDirectedAtRobot3d> _facesDirectedAtRobot3d;

    std::set<FaceID_t> _allowedTrackedFaceID;
  }; // class FaceTracker::Impl
  
} // namespace Vision
} // namespace Anki

