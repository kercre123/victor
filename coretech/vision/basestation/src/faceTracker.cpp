/**
 * File: faceTracker.cpp
 *
 * Author: Andrew Stein
 * Date:   8/18/2015
 *
 * Description: Wrapper for FacioMetric face detector / tracker and emotion estimator.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/vision/basestation/faceTracker.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/trackedFace.h"

#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/math/rotation.h"

#include "util/logging/logging.h"


#if FACE_TRACKER_PROVIDER == FACE_TRACKER_FACIOMETRIC
// FacioMetric
#  include <intraface/gaze/gaze.h>
#  include <intraface/core/LocalManager.h>
#  include <intraface/emo/EmoDet.h>

#elif FACE_TRACKER_PROVIDER == FACE_TRACKER_FACESDK
// Luxand FaceSDK
#  include <LuxandFaceSDK.h>

#else 
#  error Unknown FACE_TRACKER_PROVIDER set!
#endif

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <list>

namespace Anki {
namespace Vision {
  
#if FACE_TRACKER_PROVIDER == FACE_TRACKER_FACIOMETRIC
  
  static const cv::Scalar BLUE(255, 0, 0); ///< Definition of the blue color
  static const cv::Scalar GREEN(0, 255, 0); ///< Definition of the green color
  static const cv::Scalar RED(0, 0, 255); ///< Definition of the red color
  static const cv::Scalar FACECOLOR(50, 255, 50); ///< Definition of the face color
  static const cv::Scalar GAZECOLOR(255, 0, 255); ///< Definition of the gaze color
  
  class FaceTracker::Impl
  {
  public:
    
    Impl(const std::string& modelPath);
    ~Impl();
    
    Result Update(const Vision::Image& frame);
    
    std::list<TrackedFace> GetFaces() const;
    
  private:
    using SDM = facio::FaceAlignmentSDM<facio::AlignmentFeature, facio::NO_CONTOUR, facio::LocalManager>;
    using IE  = facio::IrisEstimator<facio::IrisFeature, facio::LocalManager>;
    using HPE = facio::HPEstimatorProcrustes<facio::LocalManager>;
    using GE  = facio::DMGazeEstimation<facio::LocalManager>;
    using ED  = facio::EmoDet<facio::LocalManager>;
    
    //const std::string _modelPath;
    
    // License manager:
    static facio::LocalManager _lm;
    
    cv::CascadeClassifier _faceCascade;
    
    static SDM* _sdm;
    //std::unique_ptr<SDM> _sdm;
    IE*  _ie;
    HPE* _hpe;
    GE*  _ge;
    ED*  _ed;
    
    facio::emoScores _Emo; // Emotions to show
    
    static int _faceCtr;
    
    // Store TrackedFaces paired with their landmarks in cv::Mat format so that
    // we can feed the previous landmarks to the FacioMetric tracker. This is
    // not memory efficient since we are also storing the features inside the
    // TrackedFace, but it prevents translated back and forth (and instead just
    // translates _from_ cv::Mat _to_ TrackedFace::Features each udpate.
    std::list<std::pair<TrackedFace,cv::Mat> > _faces;
    
  }; // class FaceTrackerImpl
  
  // Static initializations:
  int FaceTracker::Impl::_faceCtr = 0;
  FaceTracker::Impl::SDM* FaceTracker::Impl::_sdm = nullptr;
  facio::LocalManager FaceTracker::Impl::_lm;
  
  
  FaceTracker::Impl::Impl(const std::string& modelPath)
  //: _displayEnabled(false)
  {
    std::string pathSep = "";
    if(modelPath.back() != '/') {
      pathSep = "/";
    }
    
    const std::string cascadeFilename = modelPath + pathSep + "haarcascade_frontalface_alt2.xml";
    
    const bool loadSuccess = _faceCascade.load(cascadeFilename);
    if(!loadSuccess) {
      PRINT_NAMED_ERROR("VisionSystem.Update.LoadFaceCascade",
                        "Failed to load face cascade from %s\n",
                        cascadeFilename.c_str());
    }

    // Tracker object, we are using a unique pointer(http://en.cppreference.com/w/cpp/memory/unique_ptr)
    // Please notice that the SDM class does not have an explicit constructor.
    // To get an instance of the SDM class, please use the function SDM::getInstance
    if(_sdm == nullptr) { // Only initialize once to avoid LicenseManager complaining about multiple instances
      PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateSDM", "");
      _sdm = SDM::getInstance((modelPath + pathSep + "tracker_model49.bin").c_str(), &_lm);
    }
    
    // Create an instance of the class IrisEstimator
    PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateIE", "");
    _ie  = new IE((modelPath + pathSep + "iris_model.bin").c_str(), &_lm);
    
    // Create an instance of the class GazeEstimator
    PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateGE", "");
    _ge  = new GE((modelPath + pathSep + "gaze_model.bin").c_str(), &_lm);
    
    // Create an instance of the class HPEstimatorProcrustes
    PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateHPE", "");
    _hpe = new HPE((modelPath + pathSep + "hp_model.bin").c_str(), &_lm);
    
    // Create an instance of the class EmoDet
    PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateED", "");
    _ed  = new ED((modelPath + pathSep + "emo_model.bin").c_str(), &_lm);
    
  }
  

  
  FaceTracker::Impl::~Impl()
  {
    //if(_sdm) delete _sdm; // points to a singleton, so don't delete, call ~SDM();
    //_sdm->~SDM();
    
    if(_ie)  delete _ie;
    if(_hpe) delete _hpe;
    if(_ge)  delete _ge;
    if(_ed)  delete _ed;
  }
  
  // For arbitrary indices
  static inline TrackedFace::Feature GetFeatureHelper(const float* x, const float* y,
                                                      std::vector<int>&& indices)
  {
    TrackedFace::Feature pointVec;
    pointVec.reserve(indices.size());
    
    for(auto index : indices) {
      pointVec.emplace_back(x[index],y[index]);
    }
    
    return pointVec;
  }
  
  // For indices in order
  static inline TrackedFace::Feature GetFeatureHelper(const float* x, const float* y,
                                                      const int fromIndex, const int toIndex,
                                                      bool closed = false)
  {
    TrackedFace::Feature pointVec;
    
    pointVec.reserve(toIndex-fromIndex+1 + (closed ? 1 : 0));
    
    for(int i=fromIndex; i<=toIndex; ++i) {
      pointVec.emplace_back(x[i],y[i]);
    }
    
    if(closed) {
      pointVec.emplace_back(x[fromIndex], y[fromIndex]);
    }

    return pointVec;
  }
  
  static inline void UpdateFaceFeatures(const cv::Mat& landmarks,
                                        TrackedFace& face)
  {
    // X coordinates on the first row of the cv::Mat, Y on the second
    const float *x = landmarks.ptr<float>(0);
    const float *y = landmarks.ptr<float>(1);
    
    const bool drawFaceContour = landmarks.cols > 49;
    
    face.SetFeature(TrackedFace::LeftEyebrow,  GetFeatureHelper(x, y, 0,  4));
    face.SetFeature(TrackedFace::RightEyebrow, GetFeatureHelper(x, y, 5,  9));
    face.SetFeature(TrackedFace::NoseBridge,   GetFeatureHelper(x, y, 10, 13));
    face.SetFeature(TrackedFace::Nose,         GetFeatureHelper(x, y, 14, 18));
    face.SetFeature(TrackedFace::LeftEye,      GetFeatureHelper(x, y, 19, 24, true));
    face.SetFeature(TrackedFace::RightEye,     GetFeatureHelper(x, y, 25, 30, true));
    
    face.SetFeature(TrackedFace::UpperLip, GetFeatureHelper(x, y, {
      31,32,33,34,35,36,37,45,44,43,31}));
    
    face.SetFeature(TrackedFace::LowerLip, GetFeatureHelper(x, y, {
      31,48,47,46,37,38,39,40,41,42,31}));
    
    // contour
    if (drawFaceContour) {
      face.SetFeature(TrackedFace::Contour, GetFeatureHelper(x, y, 49, 65));
    }
  }
  
  
  Result FaceTracker::Impl::Update(const Vision::Image& frameOrig)
  {
    const float minScoreThreshold = 0.1f;
    
    cv::Mat frame;
    cv::Mat landmarksNext;
    float score=0.f;
    frameOrig.get_CvMat_().copyTo(frame);
    
    // Update existing faces
    for(auto faceIter = _faces.begin(); faceIter != _faces.end(); /* increment in loop */)
    {
      TrackedFace& face  = faceIter->first;
      cv::Mat& landmarks = faceIter->second;
      
      // From the landmarks in the previous frame, we predict the landmark in the current iteration
      _sdm->track(frame, landmarks, landmarksNext, score);
      
      if(score < minScoreThreshold) {
        // Delete this face (and update iterator accordingly)
        faceIter = _faces.erase(faceIter);
      } else {
        // We copy the current estimation ('landmarksNext'), to the previous
        // frame ('landmarks') for the next iteration
        landmarksNext.copyTo(landmarks);
        
        // Update the rectangle based on the landmarks, since we aren't using
        // the opencv detector for faces we're already tracking:
        double xmin, xmax, ymin, ymax;
        cv::minMaxLoc(landmarks.row(0), &xmin, &xmax);
        cv::minMaxLoc(landmarks.row(1), &ymin, &ymax);
        face.SetRect(Rectangle<f32>(xmin, ymin, xmax-xmin, ymax-ymin));
        
        // Black out face, so we don't re-detect them as new faces
        // (Make sure the rectangle doesn't extend outside the image)
        cv::Rect_<float> roi = (face.GetRect().get_CvRect_() &
                                cv::Rect_<float>(0,0,frame.cols-1, frame.rows-1));
        frame(roi).setTo(0);
        
        face.SetScore(score);
        face.SetIsBeingTracked(true);
        
        // Move to next known face
        ++faceIter;
      }
    }
    
    // Detecting new faces in the image
    // TODO: Expose these parameters
    std::vector<cv::Rect> newFaceRects;
    _faceCascade.detectMultiScale(frame, newFaceRects, 1.1, 2, 0, cv::Size(15,15));
    
    // TODO: See if we recognize this face
    
    for(auto & newFaceRect : newFaceRects) {
      // Initializing the tracker, detecting landmarks from the output of the face detector
      cv::Mat landmarks;
      float score=0.f;
      _sdm->detect(frame, newFaceRect, landmarks, score);
      
      if(score > minScoreThreshold)
      {
        TrackedFace newFace;
        
        newFace.SetRect(Rectangle<f32>(newFaceRect));
        newFace.SetID(_faceCtr++);
        
        _faces.emplace_back(newFace, landmarks);
      }
    }
    
    for(auto & facePair : _faces)
    {
      TrackedFace& face = facePair.first;
      cv::Mat& landmarks = facePair.second;
      
      // Update the timestamp for all new/updated faces
      face.SetTimeStamp(frameOrig.GetTimestamp());
      
      // Update the TrackedFace::Features from the FacioMetric landmarks:
      UpdateFaceFeatures(landmarks, face);
      
      // Computing the Emotion information & Predicting the emotion information
      _ed->predict(frame, landmarks, &_Emo);
     
      // Estimating the headpose
      facio::HeadPose headPose;
      _hpe->estimateHP(landmarks, headPose);
      RotationMatrix3d Rmat;
      Rmat.get_CvMatx_() = headPose.rot;
      Pose3d pose(Rmat, {});
      face.SetHeadPose(pose);
      // TODO: Hook up pose parent to camera?
     
      // Estimating the irises
      facio::Irisesf irises;
      _ie->detect(frame, landmarks, irises);
      face.SetLeftEyeCenter(irises.left.center);
      face.SetRightEyeCenter(irises.right.center);
      
      // Estimating the gaze, the gaze information is stored in the struct 'egs'
      //facio::EyeGazes gazes;
      //_ge->compute(landmarks, irises, headPose, gazes);
    }
    
    return RESULT_OK;
  } // Update()
 
  std::list<TrackedFace> FaceTracker::Impl::GetFaces() const
  {
    std::list<TrackedFace> retVector;
    //retVector.reserve(_faces.size());
    for(auto & face : _faces) {
      retVector.emplace_back(face.first);
    }
    return retVector;
  }

#elif FACE_TRACKER_PROVIDER == FACE_TRACKER_FACESDK
  
  class FaceTracker::Impl
  {
  public:
    Impl(const std::string& modelPath);
    ~Impl();
    
    Result Update(const Vision::Image& frameOrig);
    
    std::list<TrackedFace> GetFaces() const;
    
    void EnableDisplay(bool enabled);
    
  private:
    
    bool _isInitialized;
    
    HTracker _tracker;
    
    std::map<TrackedFace::ID_t, TrackedFace> _faces;
    
  }; // class FaceTracker::Impl
  
  
  FaceTracker::Impl::Impl(const std::string& modelPath)
  : _isInitialized(false)
  {
    int result = FSDK_ActivateLibrary("kGo498FByonCaniP29B7pwzGcHBIZJP5D9N0jcF90UhsR8gEBGyGNEmAB7XzcJxFOm9WpqBEgy6hBahqk/Eot0P1zaoWFoEa2vwitt0S7fukLfPixcwBZxnaCBmoR5NGFBbZYHX1I9vXWASt+MjqfCgpwfjZKI/oWLcGZ3AiFpA=");
    
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
        TrackedFace::ID_t reassignedID;
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

  
#else
#  error Unknown FACE_TRACKER_PROVIDER!
  
#endif // FACE_TRACKER_PROVIDER type
  
  
#pragma mark FaceTracker Implementations
  
  FaceTracker::FaceTracker(const std::string& modelPath)
  : _pImpl(new Impl(modelPath))
  {
    
  }
  
  FaceTracker::~FaceTracker()
  {

  }
  
  Result FaceTracker::Update(const Vision::Image &frameOrig)
  {
    return _pImpl->Update(frameOrig);
  }
 
  //void FaceTracker::GetFaces(std::vector<cv::Rect>& faceRects) const
  std::list<TrackedFace> FaceTracker::GetFaces() const
  {
    return _pImpl->GetFaces();
  }
  
  /*
  void FaceTracker::EnableDisplay(bool enabled) {
    _pImpl->EnableDisplay(enabled);
  }
   */
  
} // namespace Vision
} // namespace Anki
