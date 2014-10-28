
// Usage example: [faces,eyes] = mexFaceDetect(im);

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <vector>

#include "mex.h"
#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/constantsAndMacros.h"

#include "anki/common/shared/utilities_shared.h"

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

  plhs[0] = mxCreateDoubleMatrix(faces.size(), 4, mxREAL);
  
  std::vector<Eye> storedEyes;
  
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

  return;
}





