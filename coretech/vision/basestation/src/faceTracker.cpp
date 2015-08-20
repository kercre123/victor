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

#include "util/logging/logging.h"

// FacioMetric SDK
#if FACE_TRACKER_PROVIDER == FACE_TRACKER_FACIOMETRIC
#  include <intraface/gaze/gaze.h>
#  include <intraface/core/LocalManager.h>
#  include <intraface/emo/EmoDet.h>

#elif FACE_TRACKER_PROVIDER == FACE_TRACKER_FACESDK
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
    
    //void GetFaces(std::vector<cv::Rect>& faceRects) const;
    //void GetFaceLandmarks(std::vector<FaceLandmarks>& landmarks) const;
    
    /*
    void GetDisplayImage(cv::Mat& image) const;
    
    void EnableDisplay(bool enable) {
      _displayEnabled = enable;
    }
     */
    
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
    
    //bool _displayEnabled;
    
    static void drawEmotionState(cv::Mat& img, const facio::emoScores& Emo);
    
    //! This function draws lines and points using the computed landmarks.
    /*!
     \param img  Input image where the line is drawn.
     \param X  Matrix that contains the facial landmarks (2 x 49) or (2 x 66).
     */
    static void drawFace(cv::Mat& img, const cv::Mat& X);
    
    
    //! This function plots the gaze rays in the image. It takes the irises of the eyes and the gaze of each eye.
    /*!
     \param img Input image where the gaze is going to be ploted.
     \param irises It contains the coordinates of the center of the eyes.
     \param gazes Object that contains the gaze information. The field 'dir' contains the 3D direction of the gaze (x, y, z).
     */
    static void drawGaze(cv::Mat& img, const facio::Irisesf& irises, const facio::EyeGazes& gazes);
    
    //! This function draws the head pose in the image.
    //!
    //!  Each column in the rotation matrix 'rot' rotation contains the coordinates of the rotated
    //! axes in the canonical coordinate system.
    //!
    //! (x->First column, y->Second column, z->Third column).
    //!
    //! In order to draw the head pose, this method plots the columns in 'rot'. To plot
    //! the rotated axes in an clear way, we scale the columns in 'rot' by a scale factor.
    //!
    /*!
     \param img  Input image where the head pose is drawn.
     \param rot  Rotation matrix that contains the head pose.
     */
    static void drawPose(cv::Mat& img, const cv::Mat& rot);
  }; // class FaceTrackerImpl
  
  // Static initializations:
  int FaceTracker::Impl::_faceCtr = 0;
  FaceTracker::Impl::SDM* FaceTracker::Impl::_sdm = nullptr;
  facio::LocalManager FaceTracker::Impl::_lm;
  
  
  FaceTracker::Impl::Impl(const std::string& modelPath)
  //: _displayEnabled(false)
  {
    const std::string cascadeFilename = modelPath + "haarcascade_frontalface_alt2.xml";
    
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
      _sdm = SDM::getInstance((modelPath + "tracker_model49.bin").c_str(), &_lm);
    }
    
    // Create an instance of the class IrisEstimator
    PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateIE", "");
    _ie  = new IE((modelPath + "iris_model.bin").c_str(), &_lm);
    
    // Create an instance of the class GazeEstimator
    PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateGE", "");
    _ge  = new GE((modelPath + "gaze_model.bin").c_str(), &_lm);
    
    // Create an instance of the class HPEstimatorProcrustes
    PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateHPE", "");
    _hpe = new HPE((modelPath + "hp_model.bin").c_str(), &_lm);
    
    // Create an instance of the class EmoDet
    PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateED", "");
    _ed  = new ED((modelPath + "emo_model.bin").c_str(), &_lm);
    
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
      face.SetHeadPose(headPose.YAW, headPose.PITCH, headPose.ROLL);
     
      // Estimating the irises
      facio::Irisesf irises;
      _ie->detect(frame, landmarks, irises);
      face.SetLeftEyeCenter(irises.left.center);
      face.SetRightEyeCenter(irises.right.center);
      
      // Estimating the gaze, the gaze information is stored in the struct 'egs'
      //facio::EyeGazes gazes;
      //_ge->compute(landmarks, irises, headPose, gazes);
    }
    
    /*
    if(_displayEnabled) {
      cv::Mat frame2show = frame.clone();
      GetDisplayImage(frame2show);
      cv::imshow("FaceTracker", frame2show);
      cv::waitKey(5);
    }
     */
    
    return RESULT_OK;
  } // Update()
  
  /*
  void FaceTracker::Impl::GetDisplayImage(cv::Mat& frame2Show) const
  {
    for(auto & face : _faces)
    {
      // Show emotional state on the displayed frame
      //drawEmotionState(frame2Show, _Emo);
      
      // Ploting the landmarks in the image
      drawFace(frame2Show, face._landmarks);
      
      // Plot the head pose, using the rotation matrix of the head pose
      drawPose(frame2Show, face._headPose.rot);
      
      // Ploting the irises and the gaze
      drawGaze(frame2Show, face._irises, face._gazes);
    }
  }
   */
  
  std::list<TrackedFace> FaceTracker::Impl::GetFaces() const
  {
    std::list<TrackedFace> retVector;
    //retVector.reserve(_faces.size());
    for(auto & face : _faces) {
      retVector.emplace_back(face.first);
    }
    return retVector;
  }
  
#pragma mark Drawing Functions
  
  void FaceTracker::Impl::drawEmotionState(cv::Mat& img, const facio::emoScores& Emo)
  {
    cv::Point emoLoc(100, 50);
    cv::Point emoLocS(101, 51);
    
    // Plotting the emotion score for each one of the emotions, the higher score the more confident the prediction
    // Please, check the function 'getEmoStr'
    auto putTextHelper = [&emoLoc,&emoLocS](cv::Mat& img, const char* emoName, float value) {
      char emoStr[50];
      sprintf(emoStr, emoName, value);
      cv::putText(img, emoStr, emoLocS, cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(100,100,100,0.3), 1);
      cv::putText(img, emoStr, emoLoc,  cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(255, 50, 50), 1);
      emoLoc.y  += 20;
      emoLocS.y += 20;
    };
    
    putTextHelper(img, "Neutral:   %.3f", Emo.emoNeutral);
    putTextHelper(img, "Angry:     %.3f", Emo.emoAngry);
    putTextHelper(img, "Disgusted: %.3f", Emo.emoDisgust);
    putTextHelper(img, "Happy:     %.3f", Emo.emoHappy);
    putTextHelper(img, "Sad:       %.3f", Emo.emoSadness);
    putTextHelper(img, "Surprised: %.3f", Emo.emoSurprise);
  }
  
  //! This function draws  a line using several landmarks.
  /*!
   \param img  Input image where the line is drawn.
   \param X  Matrix that contains the facial landmarks (2 x 49) or (2 x 66).
   \param start Index of the first landmark.
   \param end  Index of the last landmark.
   \param circle  Plot a circle in the first and last landmark.
   */
  static void drawFaceHelper(cv::Mat& img, const cv::Mat& X, int start, int end, bool circle = false)
  {
    int thickness = 1;
    int lineType = CV_AA;
    
    for (int i = start; i < end; i++)
      cv::line(img, cv::Point((int)X.at<float>(0, i), (int)X.at<float>(1, i)), cv::Point((int)X.at<float>(0, i + 1), (int)X.at<float>(1, i + 1)), FACECOLOR, thickness, lineType);
    
    if (circle)
      cv::line(img, cv::Point((int)X.at<float>(0, end), (int)X.at<float>(1, end)), cv::Point((int)X.at<float>(0, start), (int)X.at<float>(1, start)), FACECOLOR, thickness, lineType);
  }
  
  //! This function draws a line from the landmark 'ind1' to 'ind2' .
  /*!
   \param img  Input image where the line is drawn.
   \param X  Matrix that contains the facial landmarks (2 x 49) or (2 x 66).
   \param ind1  Index of the first landmark.
   \param ind2  Index of the second landmark.
   */
  static void drawFaceHelperLine(cv::Mat& img, const cv::Mat& X, int ind1, int ind2)
  {
    int thickness = 1;
    int lineType = CV_AA;
    cv::line(img, cv::Point((int)X.at<float>(0, ind1), (int)X.at<float>(1, ind1)), cv::Point((int)X.at<float>(0, ind2), (int)X.at<float>(1, ind2)), FACECOLOR, thickness, lineType);
  }
  
  
  //! This function draws lines and points using the computed landmarks.
  /*!
   \param img  Input image where the line is drawn.
   \param X  Matrix that contains the facial landmarks (2 x 49) or (2 x 66).
   */
  void FaceTracker::Impl::drawFace(cv::Mat& img, const cv::Mat& X)
  {
    // left eyebrow
    drawFaceHelper(img, X, 0, 4);
    // right eyebrow
    drawFaceHelper(img, X, 5, 9);
    // nose
    drawFaceHelper(img, X, 10, 13);
    // under nose
    drawFaceHelper(img, X, 14, 18);
    // left eye
    drawFaceHelper(img, X, 19, 24, true);
    // right eye
    drawFaceHelper(img, X, 25, 30, true);
    // mouth contour
    drawFaceHelper(img, X, 31, 42, true);
    // inner mouth
    drawFaceHelper(img, X, 43, 45, false);
    drawFaceHelper(img, X, 46, 48, false);
    drawFaceHelperLine(img, X, 31, 43);
    drawFaceHelperLine(img, X, 31, 48);
    drawFaceHelperLine(img, X, 37, 45);
    drawFaceHelperLine(img, X, 37, 46);
    
    // contour
    if (X.cols > 49)
      drawFaceHelper(img, X, 49, 65);
  }
  
  
  //! This function plots the gaze rays in the image. It takes the irises of the eyes and the gaze of each eye.
  /*!
   \param img Input image where the gaze is going to be ploted.
   \param irises It contains the coordinates of the center of the eyes.
   \param gazes Object that contains the gaze information. The field 'dir' contains the 3D direction of the gaze (x, y, z).
   */
  void FaceTracker::Impl::drawGaze(cv::Mat& img, const facio::Irisesf& irises, const facio::EyeGazes& gazes)
  {
    int thickness = 1; // Thickness of the line
    int lineType = CV_AA;  // Type of line, this is an opencv parameter
    float lineL = 20.f; // Length of the line
    
    // Points that contains the center of the eyes, it is the initial point of the line
    cv::Point p0_left(irises.left.center.x, irises.left.center.y);
    cv::Point p0_right(irises.right.center.x, irises.right.center.y);
    
    // Final point of the line, Is computed using the direction of the gaze
    cv::Point p1_left(p0_left.x + gazes.left.dir(0)*lineL, p0_left.y + gazes.left.dir(1)*lineL);
    cv::Point p1_right(p0_right.x + gazes.right.dir(0)*lineL, p0_right.y + gazes.right.dir(1)*lineL);
    
    // Ploting circles in the eyes
    circle(img, p0_left, 1, GREEN);
    circle(img, p0_right, 1, GREEN);
    // Plotting the line that represents the gaze
    line(img, p0_left, p1_left, GAZECOLOR, thickness, lineType);
    line(img, p0_right, p1_right, GAZECOLOR, thickness, lineType);
    
  }
  
  //! This function draws the head pose in the image.
  //!
  //!  Each column in the rotation matrix 'rot' rotation contains the coordinates of the rotated
  //! axes in the canonical coordinate system.
  //!
  //! (x->First column, y->Second column, z->Third column).
  //!
  //! In order to draw the head pose, this method plots the columns in 'rot'. To plot
  //! the rotated axes in an clear way, we scale the columns in 'rot' by a scale factor.
  //!
  /*!
   \param img  Input image where the head pose is drawn.
   \param rot  Rotation matrix that contains the head pose.
   */
  void FaceTracker::Impl::drawPose(cv::Mat& img, const cv::Mat& rot)
  {
    // Position of the head pose in the image, Pixel (70, 70)
    cv::Point positionCoord = cv::Point(70, 70);
    // Thickness of the line
    int thickness = 2;
    // Line type
    int lineType = CV_AA;
    // Scaling factor
    float scale = 50.f;
    // Scaled rotated matrix
    cv::Mat scaledRot;
    // Projected rotated matrix
    cv::Mat scaledRot2D;
    
    // Scaling matrix, This matrix is used to scale the columns of the rotation matrix.
    cv::Mat scalingMatrix = (cv::Mat_<float>(3, 3) << scale, 0, 0, 0, scale, 0, 0, 0, scale);
    // Projection to the image plane, using orthognal projection from 3D to 2D
    cv::Mat orthogonalProjMatrix = (cv::Mat_<float>(2, 3) << 1, 0, 0, 0, 1, 0);
    
    // Scaling the rotation matrix
    scaledRot = scalingMatrix*rot;
    
    // Projecting the rotation matrix from 3D to 2D
    scaledRot2D = orthogonalProjMatrix*scaledRot;
    
    // Ploting the Head Pose
    line(img, positionCoord, positionCoord + cv::Point(scaledRot2D.at<float>(0, 0), scaledRot2D.at<float>(1, 0)), BLUE,  thickness, lineType);
    line(img, positionCoord, positionCoord + cv::Point(scaledRot2D.at<float>(0, 1), scaledRot2D.at<float>(1, 1)), GREEN, thickness, lineType);
    line(img, positionCoord, positionCoord + cv::Point(scaledRot2D.at<float>(0, 2), scaledRot2D.at<float>(1, 2)), RED,   thickness, lineType);
    
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
      PRINT_NAMED_ERROR("FaceTracker.Impl.ActivationFailure", "Failed to activate Luxand FaceSDK Library.");
      return;
    }
    
    char licenseInfo[1024];
    FSDK_GetLicenseInfo(licenseInfo);
    PRINT_NAMED_INFO("FaceTracker.Impl.LicenseInfo", "Luxand FaceSDK license info: %s", licenseInfo);
    
    result = FSDK_Initialize((char*)"");
    if(result != FSDKE_OK) {
      PRINT_NAMED_ERROR("FaceTracker.Impl.InitializeFailure", "Failed to initialize Luxand FaceSDK Library.");
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
    
    // FSDK_SetTrackerParameter(_tracker, <#const char *ParameterName#>, <#const char *ParameterValue#>)
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
    
    const int MAX_FACES = 8;
    
    long long detectedCount;
    long long IDs[MAX_FACES];
    res = FSDK_FeedFrame(_tracker, 0, fsdk_img, &detectedCount, IDs, sizeof(IDs));
    if (res != FSDKE_OK) {
      PRINT_NAMED_WARNING("FaceTracker.Impl.Update.TrackerFailed", "With result=%d",res);
      return RESULT_FAIL;
    }
    
    /*
    // Detect multiple faces
    int detectedCount=0;
    TFacePosition facePositions[MAX_FACES];
    //res = FSDK_DetectFace(fsdk_img, &facepos);
    res = FSDK_DetectMultipleFaces(fsdk_img, &detectedCount, facePositions, sizeof(facePositions));
    
    if (res == FSDKE_OK) {
      assert(detectedCount > 0);
      PRINT_NAMED_INFO("FaceTracker.Impl.Update.DetectedFaces", "Detected %d faces", detectedCount);
    }
    */
    
    // Detect features for each face
    for(int iFace=0; iFace<detectedCount; ++iFace)
    {
      auto result = _faces.emplace(IDs[iFace], TrackedFace());
      
      TrackedFace& face = result.first->second;
      
      face.SetTimeStamp(frameOrig.GetTimestamp());
      face.SetID(IDs[iFace]);

      if(result.second) {
        // New face
        face.SetIsBeingTracked(false);
      } else {
        // Existing face
        face.SetIsBeingTracked(true);
      }
      
      TFacePosition facePos;
      res = FSDK_GetTrackerFacePosition(_tracker, 0, IDs[iFace], &facePos);
      if (res != FSDKE_OK) {
        PRINT_NAMED_WARNING("FaceTracker.Impl.Update.FacePositionFailure",
                            "Failed finding position for face %d of %lld",
                            iFace, detectedCount);
      } else {
        // Fill in (axis-aligned) rectangle
        const float cosAngle = std::cos(DEG_TO_RAD(facePos.angle));
        const float sinAngle = std::sin(DEG_TO_RAD(facePos.angle));
        const float halfWidth = 0.5f*facePos.w;
        face.SetRect(Rectangle<f32>(facePos.xc - halfWidth*cosAngle,
                                       facePos.yc - halfWidth*sinAngle,
                                       facePos.w*cosAngle,
                                       facePos.w*sinAngle));
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
