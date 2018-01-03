/**
File: sharedMatlabInterface.h
Author: Peter Barnum
Created: 2013

A simple wrapper around the Matlab interface between C and matlab. These routines can be used to send and receive data from the Matlab process, and to execute commands in the Matlab window.
 
 Update 11-21-2014, Andrew Stein:
   Promoting to coretech/common/shared, instead of being embedded-specific.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef ANKI_CORETECH_COMMON_SHARED_MATLAB_INTERFACE_H_
#define ANKI_CORETECH_COMMON_SHARED_MATLAB_INTERFACE_H_

#if (defined(ANKICORETECH_EMBEDDED_USE_MATLAB) && ANKICORETECH_EMBEDDED_USE_MATLAB) || \
(defined(ANKICORETECH_USE_MATLAB) && ANKICORETECH_USE_MATLAB)

#include "coretech/common/shared/types.h"
#include "coretech/common/shared/matlabConverters.h"

#include <string>
#include <list>
#include <vector>

#if ANKICORETECH_EMBEDDED_USE_OPENCV || ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

/*#ifdef ANKI_MEX_BUILD
#include "mex.h"
#endif
 */

#include "engine.h"
#include "matrix.h"

// This ignores the "Format string is not a string literal" warning caused by
// AnkiMatlab_Logf macro below.
#pragma GCC diagnostic ignored "-Wformat-security"

// TODO: Better way to get the right logging macros for each platform.
// (It's kinda gross for common-shared to reach into basestation- or robot-specific code.)
#ifdef CORETECH_ROBOT
#include "coretech/common/robot/errorHandling.h"
#define LOG_ERROR(name, format, ...) AnkiError(name, format, ##__VA_ARGS__)
#define LOG_WARNING(name, format, ...) AnkiWarn(name, format, ##__VA_ARGS__)
#elif defined(CORETECH_ENGINE)
#include "util/logging/logging.h"
#define LOG_ERROR(name, format, ...) PRINT_NAMED_ERROR(name, format, ##__VA_ARGS__)
#define LOG_WARNING(name, format, ...) PRINT_NAMED_WARNING(name, format, ##__VA_ARGS__)
//#define LOG_INFO(name, format, ...) PRINT_NAMED_INFO(name, format, ...)
#else
#define LOG_ERROR(name, format, ...) do{printf("ERROR: %s - ", name); printf(format, ##__VA_ARGS__);}while(0)
#define LOG_WARNING(name, format, ...) do{printf("WARNING: %s - ", name); printf(format, ##__VA_ARGS__);}while(0)
#endif

namespace Anki
{
  enum MatlabVariableType {TYPE_UNKNOWN, TYPE_INT8, TYPE_UINT8, TYPE_INT16, TYPE_UINT16, TYPE_INT32, TYPE_UINT32, TYPE_INT64, TYPE_UINT64, TYPE_SINGLE, TYPE_DOUBLE};

  // Converts from typeid names to Matlab types
  // mxClassID ConvertToMatlabType(const char *typeName, size_t byteDepth); // TODO: Remove
  std::string ConvertToMatlabTypeString(const char *typeName, size_t byteDepth);

  class SharedMatlabInterface {
  public:
    static const s32 COMMAND_BUFFER_SIZE = 100;
    
    Engine *ep;

    std::string EvalString(const char * format, ...);

    //Same as EvalString, but in addition, Matlab will keep a std::list of the last few
    //EvalStringEcho calls, stored in "lastC2Mat2CommandBuffer"
    std::string EvalStringEcho(const char * format, ...);

    //Evaluate the std::string exactly as is, to prevent losing %d, etc.
    std::string EvalStringExplicit(const char * buffer);
    std::string EvalStringExplicitEcho(const char * buffer);

    MatlabVariableType GetType(const std::string name);

    // Redundant with function in MatlabConverters.h
    //template<typename Type> static mxClassID GetMatlabClassID();

    //Check if the variable exists on the workspace
    bool DoesVariableExist(const std::string name);

    s32 SetVisible(bool isVisible);

    //Raw array access
    mxArray* GetArray(const std::string name);

    //
    // Generic converters (same for Embedded vs. Basestation platform):
    //
    
    //Character strings need to be converted slightly to appear correctly in Matlab
    Result PutString(const char * characters, s32 nValues, const std::string name);
   
    template<typename Type> Result PutOpencvMat(const cv::Mat_<Type> &matrix, const std::string name);

    template<typename Type> Result Put(const Type * values, s32 nValues, const std::string name);

    template<typename Type> Type* Get(const std::string name);

  protected:
    
    // You can't make an instance of the base class: you must use a derived
    // class, for either embedded or basestation
    SharedMatlabInterface(bool clearWorkspace);

    
  }; // class Matlab

  
  // TODO: Move to _impl file?
  
  //
  // Templated implementations:
  //
  
  /* Redundant with one in matlabConverters.h
  template<typename Type>
  mxClassID BaseMatlabInterface::GetMatlabClassID()
  {
    return mxUNKNOWN_CLASS;
  }
   */
 
  template<typename Type> Result SharedMatlabInterface::PutOpencvMat(const cv::Mat_<Type> &matrix, const std::string name)
  {
    const mwSize matrixHeight = static_cast<mwSize>(matrix.rows);
    const mwSize matrixWidth  = static_cast<mwSize>(matrix.cols);

    if(!ep) {
      // TODO: How to print a message the right way for basestation vs. embedded??
      //AnkiConditionalErrorAndReturnValue(ep, RESULT_FAIL, "Anki.PutOpencvMat<Type>", "Matlab engine is not started/connected");
      LOG_ERROR("Anki.PutOpencvMat<Type>", "Matlab engine is not started/connected");
      return RESULT_FAIL;
    }

    const mxClassID whichClass = GetMatlabClassID<Type>();
    if(whichClass == mxUNKNOWN_CLASS) {
      // TODO: How to print a message the right way for basestation vs. embedded??
      //AnkiConditionalErrorAndReturnValue(whichClass != mxUNKNOWN_CLASS, RESULT_FAIL, "Anki.PutOpencvMat<Type>", "Unknown type to convert to a mxClassID");
      return RESULT_FAIL;
    }

    // Create the transpose:
    const mwSize dims[2] = {matrixWidth, matrixHeight};
    mxArray* mxMatrix = mxCreateNumericArray(2, dims, whichClass, mxREAL);
    Type *data = static_cast<Type*>(mxGetData(mxMatrix));

    for(mwSize y=0; y<matrixHeight; y++) {
      memcpy(data, matrix.template ptr<u8>(y,0), matrixWidth*sizeof(Type));
      data += matrixWidth;
    }

    engPutVariable(ep, name.data(), mxMatrix);

    // Transpose to what we actually want over in matlab
    EvalString("%s = %s';", name.data(), name.data());

    return RESULT_OK;
  }

 

  template<typename Type>
  Result SharedMatlabInterface::Put(const Type * values, s32 nValues, const std::string name)
  {
    if(!ep) {
      // TODO: How to print a message the right way for basestation vs. embedded??
      //AnkiConditionalErrorAndReturnValue(this->ep, RESULT_FAIL, "Anki.Put", "Matlab engine is not started/connected");
      return RESULT_FAIL;
    }

    const mwSize dims[1] = {static_cast<mwSize>(nValues)};
    const mxClassID matlabType = GetMatlabClassID<Type>();
    mxArray *arrayTmp = mxCreateNumericArray(1, &dims[0], matlabType, mxREAL);
    Type *matlabBufferTmp = (Type*) mxGetPr(arrayTmp);
    for(s32 i = 0; i<nValues; i++) {
      matlabBufferTmp[i] = values[i];
    }
    engPutVariable(ep, name.data(), arrayTmp);
    mxDestroyArray(arrayTmp);

    return RESULT_OK;
  } // template<typename Type> Result Matlab::Put(const Type * values, s32 nValues, const std::string name)

  template<typename Type>
  Type* SharedMatlabInterface::Get(const std::string name)
  {
    if(!ep) {
      // TODO: How to print a message the right way for basestation vs. embedded??
      //AnkiConditionalErrorAndReturnValue(this->ep, NULL, "Anki.Get", "Matlab engine is not started/connected");
      return NULL;
    }

    Type *valTmp = 0, *val = 0;

    mxArray *arrayTmp = GetArray(name);

    if(arrayTmp) {
      const mwSize size = mxGetNumberOfElements(arrayTmp);
      valTmp = reinterpret_cast<Type *>(mxGetPr(arrayTmp));
      val = reinterpret_cast<Type *>(calloc(size, sizeof(Type)));
      for(s32 i = 0; i<(s32)size; i++) {
        val[i] = valTmp[i];
      }
      mxDestroyArray(arrayTmp);
    } else {
      // TODO: How to print a message the right way for basestation vs. embedded??
      //AnkiError("Anki.Get", "No variable named %s exists on the workspace", name.data());
      return NULL;
    }

    return val;
  } // template<typename Type> T* Matlab::Get(const std::string name)

  
  /* Redundant with ones in matlabConverters.h
  // #pragma mark --- Specializations ---
  template<> mxClassID BaseMatlabInterface::GetMatlabClassID<u8>();
  template<> mxClassID BaseMatlabInterface::GetMatlabClassID<s8>();
  template<> mxClassID BaseMatlabInterface::GetMatlabClassID<u16>();
  template<> mxClassID BaseMatlabInterface::GetMatlabClassID<s16>();
  template<> mxClassID BaseMatlabInterface::GetMatlabClassID<u32>();
  template<> mxClassID BaseMatlabInterface::GetMatlabClassID<s32>();
  template<> mxClassID BaseMatlabInterface::GetMatlabClassID<u64>();
  template<> mxClassID BaseMatlabInterface::GetMatlabClassID<s64>();
  template<> mxClassID BaseMatlabInterface::GetMatlabClassID<f32>();
  template<> mxClassID BaseMatlabInterface::GetMatlabClassID<f64>();
  */
} // namespace Anki

#endif // #if (defined(ANKICORETECH_EMBEDDED_USE_MATLAB) && ANKICORETECH_EMBEDDED_USE_MATLAB) || (defined(ANKICORETECH_USE_MATLAB) && ANKICORETECH_USE_MATLAB)

#endif // ANKI_CORETECH_COMMON_SHARED_MATLAB_INTERFACE_H_


