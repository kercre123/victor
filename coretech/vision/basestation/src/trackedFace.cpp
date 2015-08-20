/**
 * File: trackedFace.cpp
 *
 * Author: Andrew Stein
 * Date:   8/20/2015
 *
 * Description: A container for a tracked face and any features (e.g. eyes, mouth, ...)
 *              related to it.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/vision/basestation/trackedFace.h"

namespace Anki {
namespace Vision {
  
  TrackedFace::TrackedFace()
  : _id(-1)
  , _isBeingTracked(false)
  {
    
  }
  
  #if 0
  static const cv::Scalar BLUE(255, 0, 0); ///< Definition of the blue color
  static const cv::Scalar GREEN(0, 255, 0); ///< Definition of the green color
  static const cv::Scalar RED(0, 0, 255); ///< Definition of the red color
  static const cv::Scalar FACECOLOR(50, 255, 50); ///< Definition of the face color
  static const cv::Scalar GAZECOLOR(255, 0, 255); ///< Definition of the gaze color


  class TrackedFace::Impl
  {
  public:
    
    TrackedFaceImpl()
    : _isBeingTracked(false)
    , _id(-1)
    {
      
    }
    
    bool  _isBeingTracked;
    float _score;
    TimeStamp_t _timestamp;
    
    Rectangle<f32> _rect;
    cv::Mat  _landmarks;
    
    // Variables that store, the information related to irises, head pose and eye gaze
    facio::Irisesf  _irises;
    facio::HeadPose _headPose;
    facio::EyeGazes _gazes;
    
    int _id;
    

    
  }; // class TrackedFaceImpl


  TrackedFace::TrackedFace()
  : _pImpl(new TrackedFaceImpl())
  {
    
  }

  TrackedFace::TrackedFace(const TrackedFaceImpl& impl)
  : _pImpl(new TrackedFaceImpl(impl))
  {
    
  }

  TrackedFace::TrackedFace(const TrackedFace& other)
  : _pImpl(new TrackedFaceImpl(*other._pImpl))
  {
    
  }

  TrackedFace& TrackedFace::operator=(const TrackedFace& other)
  {
    if(&other != this) {
      _pImpl.reset(new TrackedFaceImpl(*other._pImpl));
    }
    return *this;
  }

  // Use default destrictor, move constructor, and move assignment operators:
  TrackedFace::~TrackedFace() = default;
  TrackedFace::TrackedFace(TrackedFace&&) = default;
  TrackedFace& TrackedFace::operator=(TrackedFace&&) = default;


  const Rectangle<f32>& TrackedFace::GetRect() const
  {
    return _pImpl->_rect;
  }

  TimeStamp_t TrackedFace::GetTimeStamp() const
  {
    return _pImpl->_timestamp;
  }

  bool TrackedFace::IsBeingTracked() const
  {
    return _pImpl->_isBeingTracked;
  }

  float TrackedFace::GetScore() const
  {
    return _pImpl->_score;
  }

  int TrackedFace::GetID() const
  {
    return _pImpl->_id;
  }

  static inline void AddFeature(const float* x, const float* y,
                                const int fromIndex, const int toIndex,
                                std::vector<Point2f>& pointVec)
  {
    pointVec.reserve(toIndex-fromIndex+1);
    for(int i=fromIndex; i<=toIndex; ++i) {
      pointVec.emplace_back(x[i],y[i]);
    }
  }

  static inline void AddLineFeature(const float* x, const float* y,
                                    const int fromIndex, const int toIndex,
                                    std::vector<Point2f>& pointVec)
  {
    pointVec.emplace_back(x[fromIndex],y[fromIndex]);
    pointVec.emplace_back(x[toIndex],  y[toIndex]);
  }

  TrackedFace::LandmarkPolygons TrackedFace::GetLandmarkPolygons() const
  {
    LandmarkPolygons landmarksOut;
    
    // X coordinates on the first row of the cv::Mat, Y on the second
    const float *x = _pImpl->_landmarks.ptr<float>(0);
    const float *y = _pImpl->_landmarks.ptr<float>(1);
    
    const bool drawFaceContour = _pImpl->_landmarks.cols > 49;
    
    // Number of features
    landmarksOut.resize(drawFaceContour ? 14 : 13);
    
    // 1. left eyebrow
    AddFeature(x, y, 0, 4, landmarksOut[0].first);
    
    // 2. right eyebrow
    AddFeature(x, y, 5, 9, landmarksOut[1].first);
    
    // 3. nose
    AddFeature(x, y, 10, 13, landmarksOut[2].first);
    
    // 4. under nose
    AddFeature(x, y, 14, 18, landmarksOut[3].first);
    
    // 5. left eye
    AddFeature(x, y, 19, 24, landmarksOut[4].first);
    landmarksOut[4].second = true; // close the poly
    
    // 6. right eye
    AddFeature(x, y, 25, 30, landmarksOut[5].first);
    landmarksOut[5].second = true; // close the poly
    
    // 7. mouth contour
    AddFeature(x, y, 31, 42, landmarksOut[6].first);
    landmarksOut[6].second = true; // close the poly
    
    // 8-13. inner mouth
    AddFeature(x, y, 43, 45, landmarksOut[7].first);
    AddFeature(x, y, 46, 48, landmarksOut[8].first);
    AddLineFeature(x, y, 31, 43, landmarksOut[9].first);
    AddLineFeature(x, y, 31, 48, landmarksOut[10].first);
    AddLineFeature(x, y, 37, 45, landmarksOut[11].first);
    AddLineFeature(x, y, 37, 46, landmarksOut[12].first);
    
    // contour
    if (drawFaceContour) {
      AddFeature(x, y, 49, 65, landmarksOut[13].first);
    }
    
    return landmarksOut;
  }



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
  void FaceTrackerImpl::drawFace(cv::Mat& img, const cv::Mat& X)
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
  void FaceTrackerImpl::drawGaze(cv::Mat& img, const facio::Irisesf& irises, const facio::EyeGazes& gazes)
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
  void FaceTrackerImpl::drawPose(cv::Mat& img, const cv::Mat& rot)
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

  #pragma mark -
  #pragma mark Drawing Functions


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
#endif
  
} // namespace Vision
} // namespace Anki


