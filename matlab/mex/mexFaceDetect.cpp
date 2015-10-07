
// Usage example: [faces,eyes,hands] = mexFaceDetect(im);

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <vector>
#include <string.h>
#include <cstring>

#include "mex.h"
#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/constantsAndMacros.h"
#include "anki/common/robot/utilities.h"

#include "anki/common/shared/utilities_shared.h"

cv::CascadeClassifier *face_cascade = NULL;
cv::CascadeClassifier *eyes_cascade = NULL;
cv::CascadeClassifier *hand_cascade = NULL;
std::string faceCascadeFilename = "";
std::string eyeCascadeFilename = "";
std::string handCascadeFilename = "";

void closeHelper(void) {
  if(face_cascade != NULL) {
    delete face_cascade;
  }

  if(eyes_cascade != NULL) {
    delete eyes_cascade;
  }
  
  if(hand_cascade != NULL) {
    delete hand_cascade;
  }
}

#ifndef OPENCV_ROOT_PATH
#error OPENCV_ROOT_PATH should be defined. You may need to re-run cmake/configure.
#endif

typedef struct
{
  double index;
  double x;
  double y;
  double w;
  double h;
} Eye;

//int main(int argc, const char** argv)
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  mexAtExit(closeHelper);

  if(! (nlhs == 3 && nrhs >= 1 && nrhs <= 6))
  {
    mexErrMsgTxt("Usage: [faces,eyes,hands] = mexFaceDetect(im, <faceCascadeFilename>, <eyeCascadeFilename>, <handCascadeFilename>, <scaleFactor>, <minNeighbors>);\nTo use the default cascade for face or eyes, use filename ''. To not use the eye cascade, use filename 'none'.\n");
    return;
  }
  
  std::string newFaceCascadeFilename = QUOTE(OPENCV_ROOT_PATH) "/data/lbpcascades/lbpcascade_frontalface.xml";
  std::string newEyeCascadeFilename = QUOTE(OPENCV_ROOT_PATH) "/data/haarcascades/haarcascade_eye_tree_eyeglasses.xml";
  std::string newHandCascadeFilename = QUOTE(OPENCV_ROOT_PATH) "/data/haarcascades/hands/hand.xml";
  
  double scaleFactor = 1.1;
  int minNeighbors = 3;
  bool useEyeCascade = true;
  bool useHandCascade = false; // doesn't work well, so disabled by default
  
  if(nrhs >= 2)
  {
    std::string tmpFaceCascadeFilename = mxArrayToString(prhs[1]);
    
    if(tmpFaceCascadeFilename.length() > 3) {
      newFaceCascadeFilename = tmpFaceCascadeFilename;
    }
  }
  
  if(nrhs >= 3)
  {
    std::string tmpEyeCascadeFilename = mxArrayToString(prhs[2]);
    
    if(tmpEyeCascadeFilename ==  "none") {
      useEyeCascade = false;
    }
    
    if(tmpEyeCascadeFilename.length() > 3) {
      newEyeCascadeFilename = tmpEyeCascadeFilename;
    }
  }
  
  if(nrhs >= 4)
  {
    std::string tmpHandCascadeFilename = mxArrayToString(prhs[3]);
    if(tmpHandCascadeFilename == "none") {
      useHandCascade = false;
    }
    
    if(tmpHandCascadeFilename.length() > 3) {
      newHandCascadeFilename = tmpHandCascadeFilename;
      useHandCascade = true;
    }
  }
  
  if(nrhs >= 5)
  {
    const double tmpScaleFactor = Anki::Embedded::saturate_cast<f64>(mxGetScalar(prhs[4]));
    
    if(tmpScaleFactor >= 1.0)
      scaleFactor = tmpScaleFactor;
  }
  
  if(nrhs >=6)
  {
    const int tmpMinNeighbors = Anki::Embedded::saturate_cast<s32>(mxGetScalar(prhs[5]));
    
    if(tmpMinNeighbors >= 1.0)
      minNeighbors = tmpMinNeighbors;
  }
  
  if(newFaceCascadeFilename != faceCascadeFilename)
  {
    if(face_cascade) {
      delete(face_cascade);
      face_cascade = NULL;
    }
    
    faceCascadeFilename = newFaceCascadeFilename;
    
    face_cascade = new cv::CascadeClassifier();

    if(! face_cascade->load(faceCascadeFilename))
    {
      mexErrMsgTxt("Could not load face cascade XML data. Check path.");
    } else
    {
      mexPrintf("Loaded %s\n", faceCascadeFilename.data());
    }
  }

  if(useEyeCascade && newEyeCascadeFilename != eyeCascadeFilename)
  {
    if(eyes_cascade) {
      delete(eyes_cascade);
      eyes_cascade = NULL;
    }
    
    eyeCascadeFilename = newEyeCascadeFilename;
    
    eyes_cascade = new cv::CascadeClassifier();
    if(! eyes_cascade->load(eyeCascadeFilename))
    {
      mexErrMsgTxt("Could not load eyes cascade XML data. Check path.");
    } else
    {
      mexPrintf("Loaded %s\n", eyeCascadeFilename.data());
    }
  }
  
  if(useHandCascade && newHandCascadeFilename != handCascadeFilename) {
    if(hand_cascade) {
      delete(hand_cascade);
      hand_cascade = NULL;
    }
    
    handCascadeFilename = newHandCascadeFilename;
    hand_cascade = new cv::CascadeClassifier();
    if(! hand_cascade->load(handCascadeFilename))
    {
      mexErrMsgTxt("Could not load hand cascade XML data. Check path.");
    } else {
      mexPrintf("Loaded %s\n", handCascadeFilename.c_str());
    }
  }
  
  cv::Mat grayscaleFrame;
  mxArray2cvMat(prhs[0], grayscaleFrame);

  if(mxGetDimensions(prhs[0])[3] > 1) {
    cv::cvtColor(grayscaleFrame, grayscaleFrame, CV_RGB2GRAY);
  }

  cv::equalizeHist(grayscaleFrame, grayscaleFrame);
  if(nlhs > 1) {
    plhs[1] = cvMat2mxArray(grayscaleFrame);
  }

  //create a vector array to store the face found
  std::vector<cv::Rect> faces;

  //find faces and store them in the vector array
  face_cascade->detectMultiScale(grayscaleFrame, faces, scaleFactor, minNeighbors, CV_HAAR_FIND_BIGGEST_OBJECT|CV_HAAR_SCALE_IMAGE, cv::Size(30,30));

  //mexPrintf("Found %d faces.\n", faces.size());

  size_t numFaces = faces.size();

  plhs[0] = mxCreateDoubleMatrix(faces.size(), 4, mxREAL);
  
  std::vector<Eye> storedEyes;
  storedEyes.clear();
  
  double *x = mxGetPr(plhs[0]);
  double *y = x + numFaces;
  double *w = y + numFaces;
  double *h = w + numFaces;
  
  for(int iFace = 0; iFace < faces.size(); iFace++)
  {
    // Record the face:
    x[iFace] = faces[iFace].x;
    y[iFace] = faces[iFace].y;
    w[iFace] = faces[iFace].width;
    h[iFace] = faces[iFace].height;

    if(useEyeCascade) {
      // Try to find eyes:
      cv::Mat faceROI = grayscaleFrame( faces[iFace] );
      std::vector<cv::Rect> eyes;

      eyes_cascade->detectMultiScale( faceROI, eyes, 1.1, 2,
        0 |CV_HAAR_SCALE_IMAGE, cv::Size(30, 30) );

      const size_t numEyes = eyes.size();
      for(int iEye=0; iEye<numEyes; iEye++)
      {
        Eye eye;
        eye.index = double(iFace + 1);
        eye.x = double(x[iFace] + eyes[iEye].x);
        eye.y = double(y[iFace] + eyes[iEye].y);
        eye.w = double(eyes[iEye].width);
        eye.h = double(eyes[iEye].height);
        
        storedEyes.push_back(eye);
      }
    } // if(useEyeCascade)
  } // FOR each face

  plhs[1] = mxCreateDoubleMatrix(storedEyes.size(), 5, mxREAL); // [faceIndex, left, top, width, height]

  double *eye_index = mxGetPr(plhs[1]);
  double *eye_x = eye_index + storedEyes.size();
  double *eye_y = eye_x + storedEyes.size();
  double *eye_w = eye_y + storedEyes.size();
  double *eye_h = eye_w + storedEyes.size();
  for(int iEye=0; iEye<storedEyes.size(); iEye++)
  {
    eye_index[iEye] =storedEyes[iEye].index;
    eye_x[iEye] = storedEyes[iEye].x;
    eye_y[iEye] = storedEyes[iEye].y;
    eye_w[iEye] = storedEyes[iEye].w;
    eye_h[iEye] = storedEyes[iEye].h;
  }

  std::vector<cv::Rect> hands;

  if(hand_cascade)
  {
    hand_cascade->detectMultiScale(grayscaleFrame, hands, 1.2, 2,
                                   cv::CASCADE_DO_CANNY_PRUNING, cv::Size(100,100));
    
  }
  
  plhs[2] = mxCreateDoubleMatrix(hands.size(), 4, mxREAL);

  if(hand_cascade)
  {
    const size_t numHands = hands.size();
    double *x = mxGetPr(plhs[2]);
    double *y = x + numHands;
    double *w = y + numHands;
    double *h = w + numHands;
    for(int iHand = 0; iHand < hands.size(); ++iHand)
    {
      // Record the face:
      x[iHand] = hands[iHand].x;
      y[iHand] = hands[iHand].y;
      w[iHand] = hands[iHand].width;
      h[iHand] = hands[iHand].height;
    }
  }

  return;
}





