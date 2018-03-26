#ifndef MEXWRAPPERS_H
#define MEXWRAPPERS_H

#include "mex.h"
#include <vector>
#include <string>
#include <sstream>

#include "anki/common/shared/utilities_shared.h"

#define USE_OPENCV 1
#define USE_ANKI_CORE_LIBRARIES 1

#if USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

#ifdef NDEBUG
#define DEBUG_MSG(...)
#else
#define DEBUG_MSG(__verbosity__, ...) if(VERBOSITY >= (__verbosity__)) { mexPrintf(__VA_ARGS__); }
#endif

///////////////////////////////////////////////////////////////////////////
// Prototypes:
//

// Convert an image stored as a simple array stored to an mxArray,
// switching from C-style row-major ordering to Matlab's column-major.
// The output mxArray will be class DOUBLE.  If nbands>1, the input image
// is assumed to have interleaved bands data and the output mxArray will
// have one band per "plane".
//    (e.g. [R G B R G B ... ] --> [R R R ... G G G ... B B B])
template <class T>
mxArray * image2mxArray(const T *img,
                        const int nrows, const int ncols, const int nbands);

// Reverse of the above.
// Output array is assumed to already be allocated to the right size
// (nrows x ncols x nbands)
template <class T>
void mxArray2image(const mxArray *array, T *img);

#if USE_OPENCV
// Convert from a templated cv::Mat_ of cv::Vec's to an mxArray
template <class T, int BANDS>
mxArray * cvMatVec2mxArray(const cv::Mat_<cv::Vec<T, BANDS> > &Img);

template <class T, int BANDS>
void mxArray2cvMatVec(const mxArray *array, cv::Mat_<cv::Vec<T, BANDS> > &Img);

// Convert from a templated cv::Mat of scalar primitives to an mxArray
template <class T>
mxArray * cvMatScalar2mxArray(const cv::Mat_<T> &Img);

// Convert to/from a generic cv::Mat from/to a mxArray
mxArray * cvMat2mxArray(const cv::Mat &mat);
void mxArray2cvMat(const mxArray *array, cv::Mat &mat);

// Convert from a 2D (DOUBLE) mxArray to a cv::Mat_<T>, where T is a scalar
// type.
template <class T>
void mxArray2cvMatScalar(const mxArray *array, cv::Mat_<T> &img);
#endif

// Convert from a simple C-style 1D array to a mxArray.  The returned
// mxArray will be a column-vector of DOUBLEs.
template <class T>
mxArray * simpleArray2mxArray(const T *array, const unsigned long N);

// Convert from a std::vector<T> to a column vector of DOUBLEs. Type T
// should be castable to double.
template <class T>
mxArray * stdVector2mxArray(const std::vector<T> &vec);

// Convert from a mxArray to a simple C-style 1D array.  Input mxArray
// must be DOUBLE.  If NULL, output array will be allocated to the correct
// size with "new", meaning it is the caller's job to delete[] it.  If it
// is non-NULL, the array must have already been allocated to be the right
// size (numel(array)) by the caller.  Note that data will be copied in
// Matlab's column-major format.  Returns number of elements in outputArray.
template <class T>
unsigned int mxArray2simpleArray(const mxArray *array, T* &outputArray);

// Convert from a mxArray to a std::vector.  Input mxArray must be DOUBLE.
// Output vector will be resized to the correct size.  Note that data will
// be copied in Matlab's column-major format.
template <class T>
void mxArray2stdVector(const mxArray *array, std::vector<T> &vec);

// After converting your variable to an mxArray (e.g. with one of the
// functions above), use this to assign it in the Matlab workspace
// with the specified name.  Workspace should be either "base" or "caller";
// if unspecified (NULL), "base" is used.
void assignInMatlab(mxArray *var, const char *name, const char *workspace=NULL);

// Retrieve the variable "name" as a mxArray from Matlab's "base" or
// "caller" workspace.  If unspecified or NULL, workspace defaults to "base".
// If "name" is not found in the workspace, NULL will be returned.
mxArray *getFromMatlab(const char *name, const char *workspace=NULL);

// Retrieve the value of the scalar variable "name" from Matlab's "base" or
// "caller" workspace.  If unspecified or NULL, workspace defaults to "base".
// When handleUndefined is 2, an error is thrown if "name" is not found in
// the workspace.  If 1, a warning is thrown, and 0.0 is returned. If 0, then
// 0.0 is silently returned.
double getScalarDoubleFromMatlab(const char *name,
                                 const char *workspace=NULL,
                                 const unsigned int handleUndefined = 2);

// Pause Matlab (with optional duration in seconds).
void pauseMatlab(double time = 0.0);

mxClassID convertToMatlabType(const char *typeName, size_t byteDepth);

std::string convertToMatlabTypeString(const char *typeName, size_t byteDepth);

// A wrapper for mexPrintf that work with SetCoreTechPrintFunctionPtr()
int mexPrintf(const char *format, va_list vaList);

///////////////////////////////////////////////////////////////////////////
// Implementations:

template <class T>
void mxArray2image(const mxArray *array, T *img)
{
  int _nbands;

  if(mxGetClassID(array) != mxDOUBLE_CLASS) {
    mexErrMsgTxt("Input mxArray must be DOUBLE.");
  }

  if(mxGetNumberOfDimensions(array) == 2)  {
    _nbands = 1;
  }
  else if(mxGetNumberOfDimensions(array) == 3) {
    _nbands = static_cast<int>(mxGetDimensions(array)[2]);
  }
  else {
    mexErrMsgTxt("Input mxArray must have 2 or 3 dimensions.");
  }

  const int nrows  = static_cast<int>(mxGetDimensions(array)[0]);
  const int ncols  = static_cast<int>(mxGetDimensions(array)[1]);
  const int nbands = _nbands;

  // Copy the data out of the Matlab array into a cv::Mat, transposing
  // as we go
  const int npixels = nrows*ncols;
  int index =0;

  for(int i=0; i<nrows; i++) {
    // Get pointers for this row, in all bands:
    double *InputData_i[nbands];
    InputData_i[0] = mxGetPr(array) + i;
    for(int band=1; band < nbands; band++)
      InputData_i[band] = InputData_i[band-1] + npixels;

    for(int j=0; j<ncols; j++) {
      for(int band=0; band < nbands; band++) {
        img[index++] = (T) (*InputData_i[band]);

        // Shift over a column:
        InputData_i[band] += nrows;
      }
    }
  }

  // Sanity check:
  if(index != nrows*ncols*nbands)
    mexErrMsgTxt("Unexpected error translating data.");

  return;
} // mxArray2image()

#if USE_OPENCV
template <class T>
void mxArray2cvMatScalar(const mxArray *array, cv::Mat_<T> &img)
{
  if(mxGetClassID(array) != mxDOUBLE_CLASS) {
    mexErrMsgTxt("Input mxArray must be DOUBLE.");
  }

  if(mxGetNumberOfDimensions(array) > 2)  {
    mexErrMsgTxt("Input mxArray must be 2D.");
  }

  const int nrows  = (int) mxGetM(array);
  const int ncols  = (int) mxGetN(array);

  img = cv::Mat_<T>(nrows, ncols);

  // Copy the data out of the Matlab array into a cv::Mat, transposing
  // as we go
  for(int i=0; i<nrows; i++) {
    // Get pointers for this row:
    double *InputData_i = mxGetPr(array) + i;
    T *img_i = (T *) img.ptr(i);

    for(int j=0; j<ncols; j++) {
      img_i[j] = (T) (*InputData_i);

      // Shift over a column:
      InputData_i += nrows;
    }
  }
} // mxArray2cvMatScalar()
#endif

/*
* TODO: would like a function like this, but can't template based on
*       return type.
*
template<class T>
TImage<T> mxArray2Image<T>(const mxArray *array)
{
if(mxGetNumberOfDimensions(array) == 3 &&
mxGetDimensions(array)[2] == 3)
{
return mxArray2RGBImage(array);
}
else if (mxGetNumberOfDimensions(array) == 2)
{
return mxArray2GrayscaleImage(array);
}
else {
mexErrMsgTxt("Input array must have 1 or 3 bands.");
}
} // mxArray2Image()
*/

template <class T>
mxArray * image2mxArray(const T *img,
                        const int nrows, const int ncols, const int nbands)
{
  // Seems to be faster to copy the data in single precision and
  // transposed and to do the permute and double cast at the end.

  const mwSize outputDims[3] = {static_cast<mwSize>(ncols),
    static_cast<mwSize>(nrows), static_cast<mwSize>(nbands)};

  mxArray *outputArray = mxCreateNumericArray(3, outputDims,
    mxSINGLE_CLASS, mxREAL);

  const int npixels = nrows*ncols;

  // Get a pointer for each band:
  float *OutputData_[nbands];
  OutputData_[0] = (float *) mxGetData(outputArray);
  for(int band=1; band < nbands; band++)
    OutputData_[band] = OutputData_[band-1] + npixels;

  for(int i=0, i_out=0; i<npixels*nbands; i+=nbands, i_out++) {
    for(int band=0; band<nbands; band++) {
      OutputData_[band][i_out] = (float) img[i+band];
    }
  }

  mxArray *order = mxCreateDoubleMatrix(1, 3, mxREAL);
  mxGetPr(order)[0] = 2.0;
  mxGetPr(order)[1] = 1.0;
  mxGetPr(order)[2] = 3.0;

  mxArray *prhs[2] = {outputArray, order};
  mxArray *plhs[1];
  mexCallMATLAB(1, plhs, 2, prhs, "permute");

  mexCallMATLAB(1, prhs, 1, plhs, "double");

  return prhs[0];
}

#if USE_OPENCV
template <class T, int BANDS>
mxArray * cvMatVec2mxArray(const cv::Mat_<cv::Vec<T, BANDS> > &Img)
{
  const mwSize outputDims[3] = {Img.rows, Img.cols, BANDS};
  mxArray *outputArray = mxCreateNumericArray(3, outputDims,
    mxDOUBLE_CLASS, mxREAL);

  const int npixels = Img.rows*Img.cols;
  for(int i=0; i<Img.rows; i++)
  {
    for(int band=0; band<BANDS; band++)
    {
      // Get pointer for this row, in current band:
      double *OutputData_i = mxGetPr(outputArray) + i + band*npixels;
      cv::Vec<T, BANDS> *Img_i = (cv::Vec<T, BANDS> *) Img.ptr(i);

      for(int j=0; j<Img.cols; j++) {
        *OutputData_i = (double) Img_i[j][band];

        // Shift over a column:
        OutputData_i += Img.rows;
      }
    }
  }

  return outputArray;
}

template <class T, int BANDS>
void mxArray2cvMatVec(const mxArray *array, cv::Mat_<cv::Vec<T, BANDS> > &Img)
{
  if(mxGetClassID(array) != mxDOUBLE_CLASS)  {
    mexErrMsgTxt("Input array must be DOUBLE.");
  }

  const mwSize *dims = mxGetDimensions(array);
  Img = cv::Mat_<cv::Vec<T,BANDS> >(dims[0], dims[1]);

  if(mxGetNumberOfDimensions(array) != 3 || dims[2] != BANDS) {
    mexErrMsgTxt("Input array has the wrong number of bands.");
  }

  mxArray *order = mxCreateDoubleMatrix(1, 3, mxREAL);
  mxGetPr(order)[0] = 2.0;
  mxGetPr(order)[1] = 1.0;
  mxGetPr(order)[2] = 3.0;

  mxArray *prhs[2] = {const_cast<mxArray *>(array), order};
  mxArray *arrayTranspose[1];
  mexCallMATLAB(1, arrayTranspose, 2, prhs, "permute");

  const int npixels = Img.rows * Img.cols;

  // Set up pointers to each band of data in the array:
  const double *data[BANDS];
  data[0] = mxGetPr(arrayTranspose[0]);
  for(int i_band = 1; i_band<BANDS; i_band++) {
    data[i_band] = data[i_band-1] + npixels;
  }

  // Copy over the data
  for(int i=0; i < Img.rows; i++)
  {
    cv::Vec<T, BANDS> *Img_i = Img[i];

    for(int j=0; j < Img.cols; j++)
    {
      for(int band=0; band < BANDS; band++)
      {
        Img_i[j][band] = T( *(data[band]) );
        (data[band])++;
      }
    }
  }

  mxDestroyArray(order);
  mxDestroyArray(arrayTranspose[0]);

  return;
}

template <class T>
mxArray * cvMatScalar2mxArray(const cv::Mat_<T> &Img)
{
  if(Img.channels() > 1) {
    mexErrMsgTxt("Input cv::Mat should be scalar-valued.");
  }

  mxArray *outputArray = mxCreateDoubleMatrix(Img.rows, Img.cols, mxREAL);

  for(int i=0; i<Img.rows; i++)
  {
    // Get a pointer to beginning of row in intput/output
    T *Img_i = (T *) Img.ptr(i);
    double *output_i = mxGetPr(outputArray) + i;

    for(int j=0; j<Img.cols; j++)
    {
      *output_i = (double) Img_i[j];

      // Move over a column
      output_i += Img.rows;
    }
  }

  return outputArray;
}

#endif

template <class T>
mxArray * simpleArray2mxArray(const T *inputArray, const unsigned long N)
{
  if(inputArray == NULL)
  {
    mexErrMsgTxt("Null pointer passed to simpleArray2mxArray for conversion.");
  }

  mxArray *outputArray = mxCreateDoubleMatrix(N, 1, mxREAL);
  double *output = mxGetPr(outputArray);

  for(unsigned int i=0; i < N; i++)
  {
    output[i] = (double) inputArray[i];
  }

  return outputArray;
} // simpleArray2mxArray()

template <class T>
unsigned int mxArray2simpleArray(const mxArray *array, T* &outputArray)
{
  if(mxGetClassID(array) != mxDOUBLE_CLASS)
  {
    mexErrMsgTxt("Input mxArray must be of class DOUBLE.");
  }

  const unsigned int N = static_cast<unsigned int>(mxGetNumberOfElements(array));

  const double *arrayData = mxGetPr(array);

  if(outputArray==NULL) {
    outputArray = new T[N];
  }

  for(unsigned int i=0; i < N; i++)
  {
    outputArray[i] = (T) arrayData[i];
  }

  return N;
} // mxArray2simpleArray()

template <class T>
mxArray * stdVector2mxArray(const std::vector<T> &vec)
{
  mxArray *outputArray = mxCreateDoubleMatrix(vec.size(), 1, mxREAL);
  double *output = mxGetPr(outputArray);

  for(unsigned int i=0; i < vec.size(); i++)
  {
    output[i] = double(vec[i]);
  }

  return outputArray;
} // stdVector2mxArray()

template <class T>
void mxArray2stdVector(const mxArray *array, std::vector<T> &vec)
{
  if(mxGetClassID(array) != mxDOUBLE_CLASS)
  {
    mexErrMsgTxt("Input mxArray must be of class DOUBLE.");
  }

  const unsigned int N = static_cast<unsigned int>(mxGetNumberOfElements(array));

  vec.resize(N);

  const double *arrayData = mxGetPr(array);

  for(unsigned int i=0; i < N; i++)
  {
    vec[i] = (T) arrayData[i];
  }

  return;
} // mxArray2stdVector()

#if USE_OPENCV
// template helper for converting cv::Mat to mx matrix
template <class T>
void mapCvMat2mxArray(const cv::Mat &mat, const int nbands, mxArray *array)
{
  const int npixels = mat.rows*mat.cols;
  const int nrows = mat.rows;
  const int ncols = mat.cols;

  const mwSize numMatlabElements = mxGetNumberOfElements(array);
  const mwSize numOpenCvElements = npixels * mat.channels();

  if(numMatlabElements != numOpenCvElements) {
    Anki::CoreTechPrint("Matlab array has a different number of elements than the OpenCV Mat(%d != %d)\n", numMatlabElements, numOpenCvElements);
    return;
  }

  T *array_i = (T *) mxGetData(array);

  for(int i=0; i<nrows; ++i, ++array_i)
  {
    const T *mat_i = mat.ptr<T>(i);

    T *array_j = array_i;

    for(int j=0; j<ncols; ++j, array_j += nrows)
    {
      T *array_band = array_j;

      for(int band=0; band<nbands; ++band, ++mat_i, array_band += npixels)
      {
        *array_band = *mat_i;
      }
    }
  }
}

template <class T>
void mapMxArray2cvMat(const mxArray *array, const int nbands, cv::Mat &mat)
{
  const int npixels = mat.rows*mat.cols;
  const int nrows = mat.rows;
  const int ncols = mat.cols;

  const T *array_i = (const T *) mxGetData(array);

  const mwSize numMatlabElements = mxGetNumberOfElements(array);
  const mwSize numOpenCvElements = npixels * mat.channels();

  if(numMatlabElements != numOpenCvElements) {
    Anki::CoreTechPrint("Matlab array has a different number of elements than the OpenCV Mat(%d != %d)\n", numMatlabElements, numOpenCvElements);
    return;
  }

  for(int i=0; i<nrows; ++i, ++array_i) {
    T *mat_i = mat.ptr<T>(i);

    const T *array_j = array_i;

    for(int j=0; j<ncols; ++j, array_j += nrows) {
      const T *array_band = array_j;

      for(int band=0; band<nbands; ++band, ++mat_i, array_band += npixels) {
        *mat_i = *array_band;
      }
    }
  }
}

template <class T> std::vector<cv::Point_<T> > mxArray2CvPointVector(const mxArray *array)
{
  const T *array_i = (const T *) mxGetData(array);

  const mwSize numMatlabElements = mxGetNumberOfElements(array);
  const mwSize numDimensions = mxGetNumberOfDimensions(array);
  const mwSize *dimensions = mxGetDimensions(array);

  std::vector<cv::Point_<T> > points;

  if(numMatlabElements%2 != 0) {
    Anki::CoreTechPrint("Matlab array number of elements must be divisible by two\n");
    return points;
  }

  if(numDimensions != 2) {
    Anki::CoreTechPrint("Matlab array must be 2D\n");
    return points;
  }

  if(dimensions[0]!=2 && dimensions[1]!=2) {
    Anki::CoreTechPrint("Matlab array must be 2xN or Nx2\n");
    return points;
  }

  if(dimensions[0]==2 && dimensions[1]==2) {
    Anki::CoreTechPrint("Warning: Matlab array is 2x2, which is ambiguous\n");
  }

  const mwSize numPoints = (dimensions[0]==2 ? dimensions[1] : dimensions[0]);
  points.resize(numPoints);

  if(dimensions[0]==2) {
    mwSize iMatlab = 0;
    for(mwSize iOpenCv=0; iOpenCv<dimensions[1]; ++iOpenCv) {
      const T x = array_i[iMatlab++];
      const T y = array_i[iMatlab++];
      points[iOpenCv] = cv::Point_<T>(x, y);
    }
  } else {
    mwSize iMatlab = 0;
    for(mwSize iOpenCv=0; iOpenCv<dimensions[0]; ++iOpenCv) {
      const T x = array_i[iMatlab++];
      points[iOpenCv] = cv::Point_<T>(x, -1);
    }
    for(mwSize iOpenCv=0; iOpenCv<dimensions[0]; ++iOpenCv) {
      const T y = array_i[iMatlab++];
      points[iOpenCv].y = y;
    }
  }

  return points;
}

#endif // #if USE_OPENCV

#endif // #ifndef MEXWRAPPERS_H
