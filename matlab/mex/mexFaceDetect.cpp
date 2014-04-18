#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <vector>

#include "mex.h"
#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/constantsAndMacros.h"

cv::CascadeClassifier *face_cascade = NULL;
cv::CascadeClassifier *eyes_cascade = NULL;

void closeHelper(void) {
  if(face_cascade != NULL) {
    delete face_cascade;
  }

  if(eyes_cascade != NULL) {
    delete eyes_cascade;
  }
}

#ifndef OPENCV_ROOT_PATH
#error OPENCV_ROOT_PATH should be defined. You may need to re-run cmake.
#endif

//int main(int argc, const char** argv)
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  mexAtExit(closeHelper);

  if(face_cascade == NULL) {
    face_cascade = new cv::CascadeClassifier();

    if(! face_cascade->load(QUOTE(OPENCV_ROOT_PATH)
      "/data/lbpcascades/lbpcascade_frontalface.xml"))
      //                              "/data/haarcascades/haarcascade_frontalface_alt.xml"))
    {
      mexErrMsgTxt("Could not load face cascade XML data. Check path.");
    }
    mexPrintf("Using LBP cascade.\n");
  }

  if(eyes_cascade == NULL) {
    eyes_cascade = new cv::CascadeClassifier();
    if(! eyes_cascade->load(QUOTE(OPENCV_ROOT_PATH)
      "/data/haarcascades/haarcascade_eye_tree_eyeglasses.xml"))
    {
      mexErrMsgTxt("Could not load eyes cascade XML data. Check path.");
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
  face_cascade->detectMultiScale(grayscaleFrame, faces, 1.1, 3,
    CV_HAAR_FIND_BIGGEST_OBJECT|CV_HAAR_SCALE_IMAGE, cv::Size(30,30));

  //mexPrintf("Found %d faces.\n", faces.size());

  size_t numFaces = faces.size();

  plhs[0] = mxCreateDoubleMatrix(faces.size(), 12, mxREAL);
  double *x = mxGetPr(plhs[0]);
  double *y = x + numFaces;
  double *w = y + numFaces;
  double *h = w + numFaces;

  double *eye1_x = h + numFaces;
  double *eye1_y = eye1_x + numFaces;
  double *eye1_w = eye1_y + numFaces;
  double *eye1_h = eye1_w + numFaces;

  double *eye2_x = eye1_h + numFaces;
  double *eye2_y = eye2_x + numFaces;
  double *eye2_w = eye2_y + numFaces;
  double *eye2_h = eye2_w + numFaces;

  for(int i = 0; i < faces.size(); i++)
  {
    // Record the face:
    x[i] = faces[i].x;
    y[i] = faces[i].y;
    w[i] = faces[i].width;
    h[i] = faces[i].height;

    // Try to find eyes:
    cv::Mat faceROI = grayscaleFrame( faces[i] );
    std::vector<cv::Rect> eyes;

    eyes_cascade->detectMultiScale( faceROI, eyes, 1.1, 2,
      0 |CV_HAAR_SCALE_IMAGE, cv::Size(30, 30) );

    size_t numEyes = eyes.size();
    if(numEyes < 2) {
      eye2_x[i] = -1.; eye2_y[i] = -1.; eye2_w[i] = -1.; eye2_h[i] = -1.;
    }
    else {
      eye2_x[i] = x[i] + eyes[1].x;
      eye2_y[i] = y[i] + eyes[1].y;
      eye2_w[i] = eyes[1].width;
      eye2_h[i] = eyes[1].height;
    }

    if(numEyes < 1) {
      eye1_x[i] = -1.; eye1_y[i] = -1.; eye1_w[i] = -1.; eye1_h[i] = -1.;
    }
    else {
      eye1_x[i] = x[i] + eyes[0].x;
      eye1_y[i] = y[i] + eyes[0].y;
      eye1_w[i] = eyes[0].width;
      eye1_h[i] = eyes[0].height;
    }
  } // FOR each face

  return;
}