/**
 * File: faceTrackerImpl_facesdk.h
 *
 * Author: Andrew Stein
 * Date:   11/30/2015
 *
 * Description: Wrapper for Luxand FaceSDK face detector / tracker.
 *
* NOTE: This file should only be included by faceTracker.cpp
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/trackedFace.h"

#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/common/engine/math/rotation.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"

#include <LuxandFaceSDK.h>

#include <list>

namespace Anki {
namespace Vision {

  class FaceTracker::Impl
  {
  public:
    Impl(const Camera&        camera,
         const std::string&   modelPath,
         const Json::Value&   config);
    ~Impl();
    
    Result Update(const Vision::Image& frameOrig);
    
    std::list<TrackedFace> GetFaces() const;
    
    void EnableDisplay(bool enabled);
    
    static bool IsRecognitionSupported() { return true; }
    
    // TODO: Support new face enrollment settings
    void EnableNewFaceEnrollment(bool enable);
    bool IsNewFaceEnrollmentEnabled() const;
    
  private:
    
    bool _isInitialized;
    bool _enrollNewFaces = true;
    
    HTracker _tracker;
    
    std::map<FaceID_t, TrackedFace> _faces;
    
  }; // class FaceTracker::Impl
  
  
  FaceTracker::Impl::Impl(const std::string& modelPath, DetectionMode mode)
  : _isInitialized(false)
  {
    int result = FSDK_ActivateLibrary("gDULw5eSsCx78rRhbS4hRLR/m0wk+AzQgwrt0mM2PbFg6NJg5ETN+xDKuN0cA/DQApWO1ySzm+WNTgwoGFoIkbPMRqqEAPF7TkhskEqs4q9uogNDziY1MW7Asj+fS6IAo9tE7ItR2goUifoNVuAlG8R3brcbum4yA/vfsvrgQ2s=");
    
    if(result != FSDKE_OK) {
      PRINT_NAMED_ERROR("FaceTracker.Impl.ActivationFailure",
                        "Failed to activate Luxand FaceSDK Library.");
      return;
    }
    
    char licenseInfo[1024];
    FSDK_GetLicenseInfo(licenseInfo);
    PRINT_NAMED_INFO("FaceTracker.Impl.LicenseInfo", "Luxand FaceSDK license info: %s", licenseInfo);
    
    result = FSDK_Initialize((char*)"");
    if(result != FSDKE_OK) {
      PRINT_NAMED_ERROR("FaceTracker.Impl.InitializeFailure",
                        "Failed to initialize Luxand FaceSDK Library.");
      return;
    }
    
    //FSDK_SetFaceDetectionParameters(false, false, 120);
    //FSDK_SetFaceDetectionThreshold(5);
    
    result = FSDK_CreateTracker(&_tracker);
    if(result != FSDKE_OK) {
      PRINT_NAMED_ERROR("FaceTracker.Impl.CreateTrackerFailure",
                        "Failed to create Luxand FaceSDK Tracker.");
      return;
    }
    
    int errorPosition;
    result = FSDK_SetTrackerMultipleParameters(_tracker,
                                               "DetermineFaceRotationAngle=false;"
                                               "HandleArbitraryRotations=true;"
                                               "DetectFacialFeatures=true;"
                                               "DetectEyes=true;",
                                               &errorPosition);
    if(result != FSDKE_OK) {
      PRINT_NAMED_ERROR("FaceTracker.Impl.SetTrackerParametersFail",
                        "Failed setting parameter %d.",
                        errorPosition);
      return;
    }
    
    _isInitialized = true;
  }
  
  FaceTracker::Impl::~Impl()
  {
    FSDK_FreeTracker(_tracker);
  }
  
  void FaceTracker::Impl::EnableNewFaceEnrollment(bool enable)
  {
    if(enable != _enrollNewFaces)
    {
      int result = FSDK_SetTrackerParameter(_tracker, "Learning", (enable ? "true" : "false"));
      if(result != FSDKE_OK) {
        PRINT_NAMED_ERROR("FaceTracker.Impl.EnableNewFaceEnrollmentFail",
                          "Failed setting 'Learning' parameter");
      } else {
        _enrollNewFaces = enable;
      }
    }
  }
  
  bool FaceTracker::Impl::IsNewFaceEnrollmentEnabled() const
  {
    return _enrollNewFaces;
  }
  
  static inline TrackedFace::Feature GetFeatureHelper(const FSDK_Features& fsdk_features,
                                                      std::vector<int>&& indices)
  {
    TrackedFace::Feature pointVec;
    pointVec.reserve(indices.size());
    
    for(auto index : indices) {
      pointVec.emplace_back(fsdk_features[index].x, fsdk_features[index].y);
    }
    
    return pointVec;
  }

  
  Result FaceTracker::Impl::Update(const Vision::Image &frameOrig)
  {
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("FaceTracker.Impl.Update.NotInitialized",
                        "FaceTracker must be initialied before calling Update().");
      return RESULT_FAIL;
    }
    
    // Load image to FaceSDK
    HImage fsdk_img;
    int res = FSDK_LoadImageFromBuffer(&fsdk_img, frameOrig.GetDataPointer(),
                                       frameOrig.GetNumCols(), frameOrig.GetNumRows(),
                                       frameOrig.GetNumCols(), FSDK_IMAGE_GRAYSCALE_8BIT);
    if (res != FSDKE_OK) {
      PRINT_NAMED_ERROR("FaceTracker.Impl.Update.LoadImageFail",
                        "Failed to load image to FaceSDK with result=%d", res);
      return RESULT_FAIL;
    }
    
    const int MAX_FACES = 16;
    
    long long detectedCount;
    long long IDs[MAX_FACES];
    res = FSDK_FeedFrame(_tracker, 0, fsdk_img, &detectedCount, IDs, sizeof(IDs));
    if (res != FSDKE_OK) {
      PRINT_NAMED_WARNING("FaceTracker.Impl.Update.TrackerFailed", "With result=%d",res);
      return RESULT_FAIL;
    }
    
    // Detect features for each face
    for(int iFace=0; iFace<detectedCount; ++iFace)
    {
      // Add a new face or find the existing face with matching ID:
      auto result = _faces.emplace(IDs[iFace], TrackedFace());
      TrackedFace& face = result.first->second;
      
      if(result.second) {
        // New face
        face.SetIsBeingTracked(false);
        face.SetID(IDs[iFace]);
      } else {
        // Existing face
        assert(face.GetID() == IDs[iFace]);
        face.SetIsBeingTracked(true);
      }
      
      // Update the timestamp
      face.SetTimeStamp(frameOrig.GetTimestamp());
      
      TFacePosition facePos;
      res = FSDK_GetTrackerFacePosition(_tracker, 0, IDs[iFace], &facePos);
      if (res != FSDKE_OK) {
        PRINT_NAMED_WARNING("FaceTracker.Impl.Update.FacePositionFailure",
                            "Failed finding position for face %d of %lld",
                            iFace, detectedCount);
      } else {
        // Fill in (axis-aligned) rectangle
        const float halfWidth = 0.5f*facePos.w;
        Point2f upperLeft(facePos.xc - halfWidth, facePos.yc - halfWidth);
        Point2f lowerRight(facePos.xc + halfWidth, facePos.yc + halfWidth);
        RotationMatrix2d R(DEG_TO_RAD(facePos.angle));
        upperLeft = R*upperLeft;
        lowerRight = R*lowerRight;
        face.SetRect(Rectangle<f32>(upperLeft, lowerRight));
      }
      
      FSDK_Features features;
      res = FSDK_GetTrackerFacialFeatures(_tracker, 0, IDs[iFace], &features);
      if(res != FSDKE_OK) {
        PRINT_NAMED_WARNING("FaceTracker.Impl.Update.FaceFeatureFailure",
                            "Failed finding features for face %d of %lld",
                            iFace, detectedCount);
      } else {
        face.SetLeftEyeCenter(Point2f(features[FSDKP_LEFT_EYE].x,
                                         features[FSDKP_LEFT_EYE].y));
        
        face.SetRightEyeCenter(Point2f(features[FSDKP_RIGHT_EYE].x,
                                          features[FSDKP_RIGHT_EYE].y));
        
        // Set the observed head orientation
        // NOTE: FaceSDK doesn't have head pose estimation, so just use angle of the
        // line connecting the eyes as a proxy for roll
        Point2f eyeLine(face.GetRightEyeCenter());
        eyeLine -= face.GetLeftEyeCenter();
        face.SetHeadOrientation(std::atan2f(eyeLine.y(), eyeLine.x()), 0, 0);
        
        face.SetFeature(TrackedFace::LeftEyebrow, GetFeatureHelper(features, {
          FSDKP_LEFT_EYEBROW_OUTER_CORNER,
          FSDKP_LEFT_EYEBROW_MIDDLE_LEFT,
          FSDKP_LEFT_EYEBROW_MIDDLE,
          FSDKP_LEFT_EYEBROW_MIDDLE_RIGHT,
          FSDKP_LEFT_EYEBROW_INNER_CORNER}));

        face.SetFeature(TrackedFace::RightEyebrow, GetFeatureHelper(features, {
         FSDKP_RIGHT_EYEBROW_INNER_CORNER,
         FSDKP_RIGHT_EYEBROW_MIDDLE_LEFT,
         FSDKP_RIGHT_EYEBROW_MIDDLE,
         FSDKP_RIGHT_EYEBROW_MIDDLE_RIGHT,
         FSDKP_RIGHT_EYEBROW_OUTER_CORNER}));
        
        face.SetFeature(TrackedFace::LeftEye, GetFeatureHelper(features, {
          FSDKP_LEFT_EYE_OUTER_CORNER,
          FSDKP_LEFT_EYE_UPPER_LINE1,
          FSDKP_LEFT_EYE_UPPER_LINE2,
          FSDKP_LEFT_EYE_UPPER_LINE3,
          FSDKP_LEFT_EYE_INNER_CORNER,
          FSDKP_LEFT_EYE_LOWER_LINE1,
          FSDKP_LEFT_EYE_LOWER_LINE2,
          FSDKP_LEFT_EYE_LOWER_LINE3,
          FSDKP_LEFT_EYE_OUTER_CORNER}));
        
        face.SetFeature(TrackedFace::RightEye, GetFeatureHelper(features, {
          FSDKP_RIGHT_EYE_OUTER_CORNER,
          FSDKP_RIGHT_EYE_UPPER_LINE1,
          FSDKP_RIGHT_EYE_UPPER_LINE2,
          FSDKP_RIGHT_EYE_UPPER_LINE3,
          FSDKP_RIGHT_EYE_INNER_CORNER,
          FSDKP_RIGHT_EYE_LOWER_LINE1,
          FSDKP_RIGHT_EYE_LOWER_LINE2,
          FSDKP_RIGHT_EYE_LOWER_LINE3,
          FSDKP_RIGHT_EYE_OUTER_CORNER}));
        
        face.SetFeature(TrackedFace::Nose, GetFeatureHelper(features, {
          FSDKP_NOSE_BRIDGE,
          FSDKP_NOSE_RIGHT_WING,
          FSDKP_NOSE_RIGHT_WING_OUTER,
          FSDKP_NOSE_RIGHT_WING_LOWER,
          FSDKP_NOSE_BOTTOM,
          FSDKP_NOSE_LEFT_WING_LOWER,
          FSDKP_NOSE_LEFT_WING_OUTER,
          FSDKP_NOSE_LEFT_WING,
          FSDKP_NOSE_BRIDGE}));
        
        face.SetFeature(TrackedFace::UpperLip, GetFeatureHelper(features, {
          FSDKP_MOUTH_RIGHT_CORNER, // FaceSDK L/R reversed!
          FSDKP_MOUTH_LEFT_TOP,
          FSDKP_MOUTH_TOP,
          FSDKP_MOUTH_RIGHT_TOP,
          FSDKP_MOUTH_LEFT_CORNER, // FaceSDK L/R reversed!
          FSDKP_MOUTH_RIGHT_TOP_INNER,
          FSDKP_MOUTH_TOP_INNER,
          FSDKP_MOUTH_LEFT_TOP_INNER,
          FSDKP_MOUTH_RIGHT_CORNER})); // FaceSDK L/R reversed!
        
        face.SetFeature(TrackedFace::LowerLip, GetFeatureHelper(features, {
          FSDKP_MOUTH_RIGHT_CORNER, // FaceSDK L/R reversed!
          FSDKP_MOUTH_LEFT_BOTTOM,
          FSDKP_MOUTH_BOTTOM,
          FSDKP_MOUTH_RIGHT_BOTTOM,
          FSDKP_MOUTH_LEFT_CORNER, // FaceSDK L/R reversed!
          FSDKP_MOUTH_RIGHT_BOTTOM_INNER,
          FSDKP_MOUTH_BOTTOM_INNER,
          FSDKP_MOUTH_LEFT_BOTTOM_INNER,
          FSDKP_MOUTH_RIGHT_CORNER})); // FaceSDK L/R reversed!
        
        face.SetFeature(TrackedFace::Contour, GetFeatureHelper(features, {
          FSDKP_FACE_CONTOUR2,
          FSDKP_FACE_CONTOUR1,
          FSDKP_CHIN_LEFT,
          FSDKP_CHIN_BOTTOM,
          FSDKP_CHIN_RIGHT,
          FSDKP_FACE_CONTOUR13,
          FSDKP_FACE_CONTOUR12}));
      }
       
    }
    
    // Remove any faces that were not observed
    for(auto faceIter = _faces.begin(); faceIter != _faces.end(); /* increment in loop */)
    {
      if(faceIter->second.GetTimeStamp() < frameOrig.GetTimestamp()) {
        faceIter = _faces.erase(faceIter);
      } else {
        // Didn't erase: increment normally
        ++faceIter;
      }
    }
    
#if 0
    // Remove any faces whose IDs got reassigned
    for(auto faceIter = _faces.begin(); faceIter != _faces.end(); /* increment in loop */)
    {
      TrackedFace& face = faceIter->second;
      
      // Only check faces that weren't updated this timestamp
      if(face.GetTimeStamp() < frameOrig.GetTimestamp()) {
        FaceID_t reassignedID;
        FSDK_GetIDReassignment(_tracker, face.GetID(), &reassignedID);
        if(reassignedID != face.GetID()) {
          // Face's ID got reassigned!
          assert(_faces.find(reassignedID) != _faces.end()); // Reassigned ID should exist!
          faceIter = _faces.erase(faceIter);

        } else {
          // Didn't erase: increment normally
          ++faceIter;
        }
      } else {
        // Didn't erase: increment normally
        ++faceIter;
      }
    }
#endif

    
    // Unload image from FaceSDK
    FSDK_FreeImage(fsdk_img);
    
    return RESULT_OK;
  }
  
  std::list<TrackedFace> FaceTracker::Impl::GetFaces() const
  {
    std::list<TrackedFace> retList;
    for(auto & facePair : _faces) {
      retList.emplace_back(facePair.second);
    }
    return retList;
  }
  
  void FaceTracker::Impl::EnableDisplay(bool enabled)
  {
    
  }
  
} // namespace Vision
} // namespace Anki
