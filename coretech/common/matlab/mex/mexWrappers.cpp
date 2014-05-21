#include "anki/common/matlab/mexWrappers.h"

#if defined(_MSC_VER)
#ifndef snprintf
#define snprintf sprintf_s
#endif
#endif

// Specialization for double input:
template <>
mxArray * image2mxArray<double>(const double *img,
                                const int nrows, const int ncols, const int nbands)
{
  const mwSize outputDims[3] = {static_cast<mwSize>(ncols),
    static_cast<mwSize>(nrows), static_cast<mwSize>(nbands)};
  mxArray *outputArray = mxCreateNumericArray(3, outputDims,
    mxDOUBLE_CLASS, mxREAL);

  const int npixels = nrows*ncols;

  // Get a pointer for each band:
#ifdef _MSC_VER
  double **OutputData_ = (double **) _alloca(nbands);
#else
  double *OutputData_[nbands];
#endif

  OutputData_[0] = mxGetPr(outputArray);
  for(mwSize band=1; band < static_cast<mwSize>(nbands); band++)
    OutputData_[band] = OutputData_[band-1] + npixels;

  for(mwSize i=0, i_out=0; i<static_cast<mwSize>(npixels*nbands); i+=nbands, i_out++) {
    for(mwSize band=0; band<static_cast<mwSize>(nbands); band++) {
      OutputData_[band][i_out] = img[i+band];
    }
  }

  mxArray *order = mxCreateDoubleMatrix(1, 3, mxREAL);
  mxGetPr(order)[0] = 2.0;
  mxGetPr(order)[1] = 1.0;
  mxGetPr(order)[2] = 3.0;

  mxArray *prhs[2] = {outputArray, order};
  mxArray *plhs[1];
  mexCallMATLAB(1, plhs, 2, prhs, "permute");

  return plhs[0];
} // image2mxArray<double>(array)

// Specialization of generic template when output is double, since
// we can just copy the data directly.
template <>
unsigned int mxArray2simpleArray<double>(const mxArray *array, double* &outputArray)
{
  if(mxGetClassID(array) != mxDOUBLE_CLASS)
  {
    mexErrMsgTxt("Input mxArray must be of class DOUBLE.");
  }

  const unsigned int N = static_cast<unsigned int>(mxGetNumberOfElements(array));

  const double *arrayData = mxGetPr(array);

  if(outputArray==NULL) {
    outputArray = new double[N];
  }

  memcpy(outputArray, arrayData, N*sizeof(double));

  return N;
} // mxArray2simpleArray<double>()

#if USE_OPENCV
mxArray* cvMat2mxArray(const cv::Mat &mat)
{
  //int  type   = mat.type();
  mwSize nbands = mat.channels(); //CV_MAT_CN(type);
  int depth  = mat.depth(); //CV_MAT_DEPTH(type);

  mwSize nrows = mat.rows, ncols = mat.cols;

  const mwSize outputDims[3] = {nrows,ncols,nbands};
  mxArray *outputArray;
  switch(depth)
  {
  case CV_8U:
    {
      outputArray = mxCreateNumericArray(3,outputDims,mxUINT8_CLASS,mxREAL);
      mapCvMat2mxArray<uchar>(mat,static_cast<int>(nbands),outputArray);
      break;
    }
  case CV_8S:
    {
      outputArray = mxCreateNumericArray(3,outputDims,mxINT8_CLASS,mxREAL);
      mapCvMat2mxArray<char>(mat,static_cast<int>(nbands),outputArray);
      break;
    }
  case CV_16U:
    {
      outputArray = mxCreateNumericArray(3,outputDims,mxUINT16_CLASS,mxREAL);
      mapCvMat2mxArray<unsigned short>(mat,static_cast<int>(nbands),outputArray);
      break;
    }
  case CV_16S:
    {
      outputArray = mxCreateNumericArray(3,outputDims,mxINT16_CLASS,mxREAL);
      mapCvMat2mxArray<short>(mat,static_cast<int>(nbands),outputArray);
      break;
    }
  case CV_32S:
    {
      outputArray = mxCreateNumericArray(3,outputDims,mxINT32_CLASS,mxREAL);
      mapCvMat2mxArray<int>(mat,static_cast<int>(nbands),outputArray);
      break;
    }
  case CV_32F:
    {
      outputArray = mxCreateNumericArray(3,outputDims,mxSINGLE_CLASS,mxREAL);
      mapCvMat2mxArray<float>(mat,static_cast<int>(nbands),outputArray);
      break;
    }
  case CV_64F:
    {
      outputArray = mxCreateNumericArray(3,outputDims,mxDOUBLE_CLASS,mxREAL);
      mapCvMat2mxArray<double>(mat,static_cast<int>(nbands),outputArray);
      break;
    }
  default:
    {
      //std::cout << "NOT supported depth!!" << depth << ", type: " << type << std::endl;
      //return NULL
      outputArray = NULL;
      mexErrMsgTxt("Unsupported cv::Mat depth for conversion to mxArray!");
    }
  }

  return outputArray;
} // cvMat2mxArray()

void mxArray2cvMat(const mxArray *array, cv::Mat &mat)
{
  // Find number of channels:
  int nrows, ncols, numChannels;

  switch(mxGetNumberOfDimensions(array))
  {
  case 2:
    numChannels = 1;
    nrows = static_cast<int>(mxGetM(array));
    ncols = static_cast<int>(mxGetN(array));
    break;
  case 3:
    nrows       = static_cast<int>(mxGetDimensions(array)[0]);
    ncols       = static_cast<int>(mxGetDimensions(array)[1]);
    numChannels = static_cast<int>(mxGetDimensions(array)[2]);
    break;
  default:
    mexErrMsgTxt("Image must be scalar or vector valued.");
  }

  // Find bit depth:
  switch(mxGetClassID(array))
  {
  case mxDOUBLE_CLASS:
    {
      mat = cv::Mat(nrows, ncols, CV_MAKE_TYPE(CV_64F, numChannels));
      mapMxArray2cvMat<double>(array, numChannels, mat);
      break;
    }

  case mxSINGLE_CLASS:
    {
      mat = cv::Mat(nrows, ncols, CV_MAKE_TYPE(CV_32F, numChannels));
      mapMxArray2cvMat<float>(array, numChannels, mat);
      break;
    }
    /* No such thing as CV_32U in OpenCV?
    case mxUINT32_CLASS:
    {
    mat = cv::Mat(nrows, ncols, CV_MAKE_TYPE(CV_32U, numChannels));
    mapMxArray2cvMat<unsigned int>(array, numChannels, mat);
    break;
    }
    */
  case mxUINT8_CLASS:
    {
      mat = cv::Mat(nrows, ncols, CV_MAKE_TYPE(CV_8U, numChannels));
      mapMxArray2cvMat<unsigned char>(array, numChannels, mat);
      break;
    }

  case mxUINT16_CLASS:
    {
      mat = cv::Mat(nrows, ncols, CV_MAKE_TYPE(CV_16U, numChannels));
      mapMxArray2cvMat<unsigned short>(array, numChannels, mat);
      break;
    }

  default:
    mexErrMsgTxt("Unsupported class for converstion to cv::Mat.");
  } // SWITCH(mxClassID)

  return;
} // mxArray2cvMat()

#endif // USE_OPENCV

mxArray *imageTranspose(mxArray *input)
{
  mxArray *order = mxCreateDoubleMatrix(1, 3, mxREAL);
  mxGetPr(order)[0] = 2.0;
  mxGetPr(order)[1] = 1.0;
  mxGetPr(order)[2] = 3.0;

  mxArray *prhs[2] = {input, order};
  mxArray *plhs[1];
  mexCallMATLAB(1, plhs, 2, prhs, "permute");

  return plhs[0];
}

void safeCallMATLAB(int nlhs, mxArray *plhs[], int nrhs,
                    mxArray *prhs[], const char *functionName)
{
  // Try the call:
  mxArray *mxErr = mexCallMATLABWithTrap(nlhs, plhs, nrhs, prhs, functionName);

  // Handle any errors:
  if(mxErr != NULL) {
    // Base message:
    std::string msg("Matlab returned error: ");

    //mexPrintf("Caught error of class %s.\n", mxGetClassName(mxErr));

    // Use Matlab helper function to get message from the MException object:
    mxArray *plhs[1];
    char buf[1024];
    if(mexCallMATLABWithTrap(1, plhs, 1, &mxErr, "getMExceptionMessage") != NULL)
    {
      snprintf(buf, 1024, "<<Could not get message from MException object!>>");
    } else if(mxGetString(plhs[0], buf, 1024)==1)
    {
      snprintf(buf, 1024, "<<Could not create string from exception message!>>");
    }

    msg += buf;
    mexErrMsgTxt(msg.c_str());
  }
} // safeCallMATLAB()

void assignInMatlab(mxArray *var, const char *name, const char *workspace)
{
  if(var==NULL) {
    std::stringstream msg;
    msg << "Null mxArray pointer passed to assignInMatlab. " <<
      "Creating empty array named '" << name << "'.";
    mexWarnMsgTxt(msg.str().c_str());
    var = mxCreateDoubleMatrix(0, 0, mxREAL);
  }

  mxArray *ws=NULL;
  if(workspace==NULL) {
    ws = mxCreateString("base");
  }
  else if(!strcmp(workspace, "caller") || !strcmp(workspace, "base")) {
    ws = mxCreateString(workspace);
  }
  else {
    mexErrMsgTxt("Workspace for assignInMatlab must be 'base' or 'caller'.");
  }

  mxArray *prhs[3] = {ws, mxCreateString(name), var};
  mexCallMATLAB(0, NULL, 3, prhs, "assignin");
  //mxFree(prhs[0]);
  //mxFree(prhs[1]);

  // We don't free the variable which was just assigned in Matlab because
  // that's Matlab's memory now.
} // assignInMatlab()

mxArray *getFromMatlab(const char *name, const char *workspace)
{
  mxArray *ws=NULL;
  if(workspace==NULL) {
    ws = mxCreateString("base");
  }
  else if(!strcmp(workspace, "caller") || !strcmp(workspace, "base")) {
    ws = mxCreateString(workspace);
  }
  else {
    mexErrMsgTxt("Workspace for assignInMatlab must be 'base' or 'caller'.");
  }

  mxArray *plhs[1];
  mxArray *prhs[2] = {ws, mxCreateString(name)};

  mxArray *mxErr = mexCallMATLABWithTrap(1, plhs, 2, prhs, "evalin");
  if(mxErr != NULL) {
    plhs[0] = NULL;
  }

  return plhs[0];
} // getFromMatlab()

double getScalarDoubleFromMatlab(const char *name, const char *workspace,
                                 const unsigned int handleUndefined)
{
  mxArray *array = getFromMatlab(name, workspace);

  if(array == NULL) {
    std::stringstream msg;
    msg << "Matlab variable '" << name << "' undefined";

    switch(handleUndefined)
    {
    case(0):
      return 0.0;

    case(1):
      {
        msg << ", returning 0.0!";
        mexWarnMsgTxt(msg.str().c_str());
        return 0.0;
      }
    case(2):
      {
        msg << "!";
        mexErrMsgTxt(msg.str().c_str());
      }
    default:
      {
        mexErrMsgTxt("handleUndefined must be 0, 1, or 2.");
      }
    } // SWITCH
  } // IF mexCallMATLABWithTrap failed

  if(mxGetNumberOfElements(array) != 1) {
    std::stringstream msg2;
    msg2 << "Matlab variable '" << name << "' must be a scalar.";
    mexErrMsgTxt(msg2.str().c_str());
  }

  // Note this call automatically converts to DOUBLE regardless of
  // array's actual type:
  return mxGetScalar(array);
} // getScalarDoubleFromMatlab()

void pauseMatlab(double time)
{
  if(time > 0.0) {
    mxArray *prhs[1] = {mxCreateDoubleScalar(time)};
    mexCallMATLAB(0, NULL, 1, prhs, "pause");
    //mxFree(prhs[0]);
  } else {
    mexEvalString("pause");
  }
}

int mexPrintf(const char *format, va_list vaList)
{
  const int BUFFER_LENGTH = 512;

  char text[BUFFER_LENGTH];

  vsnprintf(text, BUFFER_LENGTH, format, vaList);

  return mexPrintf(text);
}
