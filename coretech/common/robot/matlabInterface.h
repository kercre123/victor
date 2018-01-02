/**
 File: matlabInterface.h
 Author: Andrew Stein
 Created: 11/21/2014
 
 Extends shared Matlab interface to support Embedded-specific data structures.
 
 Copyright Anki, Inc. 2014
 For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
 **/
#ifndef ANKI_CORETECH_COMMON_EMBEDDED_MATLAB_INTERFACE_H_
#define ANKI_CORETECH_COMMON_EMBEDDED_MATLAB_INTERFACE_H_

#if ANKICORETECH_EMBEDDED_USE_MATLAB

#include "coretech/common/shared/sharedMatlabInterface.h"

#include "coretech/common/robot/matlabConverters_embedded.h"

#include "coretech/common/robot/array2d.h"
#include "coretech/common/robot/errorHandling.h"



namespace Anki {
namespace Embedded {
  
  //These mex-only functions and #defines output to the Matlab command line, in a similar way to the AnkiError #defines

#ifdef ANKI_MEX_BUILD

  extern char tmpMexBuffer1[1024];
  extern char tmpMexBuffer2[1024];
  
#define _AnkiMatlab_Logf(logLevel, eventName, eventValue, file, funct, line, ...)\
{\
snprintf(tmpMexBuffer1, 1024, "LOG[%d] - %s - %s\n", logLevel, eventName, eventValue);\
snprintf(tmpMexBuffer2, 1024, tmpMexBuffer1, ##__VA_ARGS__);\
mexPrintf(tmpMexBuffer2);\
}


#undef AnkiWarn
#define AnkiWarn(eventName, eventValue_format, ...) \
{ _AnkiMatlab_Logf(ANKI_LOG_LEVEL_WARN, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }
  
#undef AnkiConditionalWarn
#define AnkiConditionalWarn(expression, eventName, eventValue_format, ...) \
if(!(expression)) { _AnkiMatlab_Logf(ANKI_LOG_LEVEL_WARN, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }
  
#undef AnkiConditionalWarnAndReturn
#define AnkiConditionalWarnAndReturn(expression, eventName, eventValue_format, ...) \
if(!(expression)) {\
_AnkiMatlab_Logf(ANKI_LOG_LEVEL_WARN, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
return;\
}
  
#undef AnkiError
#define AnkiError(eventName, eventValue_format, ...) _AnkiMatlab_Logf(ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);
  
#undef AnkiConditionalError
#define AnkiConditionalError(expression, eventName, eventValue_format, ...) \
if(!(expression)) { \
_AnkiMatlab_Logf(ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
}
  
#undef AnkiConditionalErrorAndReturn
#define AnkiConditionalErrorAndReturn(expression, eventName, eventValue_format, ...) \
if(!(expression)) {\
_AnkiMatlab_Logf(ANKI_LOG_LEVEL_ERROR, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); \
return;\
}
#endif // #ifdef ANKI_MEX_BUILD
  
  
  class Matlab : public SharedMatlabInterface
  {
  public:
    
    Matlab(bool clearWorkspace=false);
    
    template<typename Type> Result PutArray(const Array<Type> &matrix, const std::string name);
    
    template<typename Type> Array<Type> GetArray(const std::string name, MemoryStack &memory);

    template<typename Type> Result PutQuad(const Quadrilateral<Type> &quad, const std::string name);
    
    template<typename Type> Quadrilateral<Type> GetQuad(const std::string name);

    // Overload the templated Put() coming from base class for Point<s16>
    Result PutPoints(const Point<s16> * values, s32 nValues, const std::string name);

    
  }; // class Matlab

  
  //
  // Templated implementations:
  //
  
  // TODO: Move to _impl file?
  
  template<typename Type>
  Result Matlab::PutArray(const Array<Type> &matrix, const std::string name)
  {
    const mwSize matrixHeight = static_cast<mwSize>(matrix.get_size(0));
    const mwSize matrixWidth  = static_cast<mwSize>(matrix.get_size(1));
    
    AnkiConditionalErrorAndReturnValue(ep, RESULT_FAIL, "Anki.PutArray<Type>", "Matlab engine is not started/connected");
    
    const mxClassID whichClass = GetMatlabClassID<Type>();
    AnkiConditionalErrorAndReturnValue(whichClass != mxUNKNOWN_CLASS, RESULT_FAIL, "Anki.PutArray<Type>", "Unknown type to convert to a mxClassID");
    
    // Create the transpose:
    const mwSize dims[2] = {matrixWidth, matrixHeight};
    mxArray* mxMatrix = mxCreateNumericArray(2, dims, whichClass, mxREAL);
    Type *data = static_cast<Type*>(mxGetData(mxMatrix));
    
    AnkiConditionalErrorAndReturnValue(matrixHeight <= std::numeric_limits<s32>::max(),
                                       RESULT_FAIL_INVALID_SIZE,
                                       "Anki.PutArray<Type>",
                                       "Matrix too large to use call to matrix.Pointer()");
    
    for(mwSize y=0; y<matrixHeight; y++) {
      memcpy(data, matrix.Pointer(static_cast<s32>(y),0), matrixWidth*sizeof(Type));
      data += matrixWidth;
    }
    
    engPutVariable(ep, name.data(), mxMatrix);
    
    // Transpose to what we actually want over in matlab
    EvalString("%s = %s';", name.data(), name.data());
    
    /*
     const std::string tmpName = name + std::string("_AnkiTMP");
     const std::string matlabTypeName = Anki::Embedded::ConvertToMatlabTypeString(typeid(Type).name(), sizeof(Type));
     
     EvalStringEcho("%s=zeros([%d,%d],'%s');", name.data(), matrixHeight, matrixWidth, matlabTypeName.data());
     
     for(s32 y=0; y<matrixHeight; y++) {
     Put<Type>(matrix.Pointer(y,0), matrixWidth, tmpName);
     EvalStringEcho("%s(%d,:)=%s;", name.data(), y+1, tmpName.data());
     }
     
     EvalString("clear %s;", tmpName.data());
     */
    return RESULT_OK;
  } // PutArray()
  
  
  template<typename Type>
  Result Matlab::PutQuad(const Quadrilateral<Type> &quad, const std::string name)
  {
    AnkiConditionalErrorAndReturnValue(ep, RESULT_FAIL, "Anki.PutQuad<Type>", "Matlab engine is not started/connected");
    
    // Initially just put a 8-element array into Matlab
    Type tempArray[8] = {
      quad[0].x, quad[1].x, quad[2].x, quad[3].x,
      quad[0].y, quad[1].y, quad[2].y, quad[3].y
    };
    
    SharedMatlabInterface::Put<Type>(tempArray, 8, name);
    
    // Reshape the array to be 4x2 -- [x(:) y(:)]
    EvalStringEcho("%s = reshape(%s, [4 2]);", name.data(), name.data());
    
    return RESULT_OK;
  } // PutQuad()
  
  
  template<typename Type>
  Array<Type> Matlab::GetArray(const std::string name, MemoryStack &memory)
  {
    AnkiConditionalErrorAndReturnValue(ep, Array<Type>(0, 0, NULL, 0), "Anki.GetArray<Type>", "Matlab engine is not started/connected");
    
    const std::string tmpName = name + std::string("_AnkiTMP");
    
    EvalString("%s=%s';", tmpName.data(), name.data());
    mxArray *arrayTmp = SharedMatlabInterface::GetArray(tmpName.data());
    
    AnkiConditionalErrorAndReturnValue(arrayTmp, Array<Type>(0, 0, NULL, 0), "Anki.GetArray<Type>", "%s could not be got from Matlab", tmpName.data());
    
    AnkiConditionalErrorAndReturnValue(mxGetClassID(arrayTmp) == GetMatlabClassID<Type>(), Array<Type>(0, 0, NULL, 0), "Anki.GetArray<Type>", "matlabClassId != ankiVisionClassId");
    
    const size_t numCols = mxGetM(arrayTmp);
    const size_t numRows = mxGetN(arrayTmp);
    // Unused? const s32 stride = Array<Type>::ComputeRequiredStride(static_cast<s32>(numCols), Flags::Buffer(true,false));
    
    Array<Type> ankiArray(static_cast<s32>(numRows), static_cast<s32>(numCols), memory);
    
    Type *matlabArrayTmp = reinterpret_cast<Type*>(mxGetPr(arrayTmp));
    s32 matlabIndex = 0;
    
    s32 ankiArrayHeight = ankiArray.get_size(0);
    s32 ankiArrayWidth = ankiArray.get_size(1);
    
    for(s32 y=0; y<ankiArrayHeight; y++) {
      Type * pThisData = ankiArray.Pointer(y, 0);
      for(s32 x=0; x<ankiArrayWidth; x++) {
        pThisData[x] = matlabArrayTmp[matlabIndex++];
      }
    }
    
    mxDestroyArray(arrayTmp);
    
    EvalString("clear %s;", tmpName.data());
    
    return ankiArray;
  } // GetArray()
  
  
  template<typename Type>
  Quadrilateral<Type> Matlab::GetQuad(const std::string name)
  {
    AnkiConditionalErrorAndReturnValue(this->ep, Quadrilateral<Type>(),
                                       "Anki.Get", "Matlab engine is not started/connected");
    
    const mxArray* mxQuad = SharedMatlabInterface::GetArray(name);
    
    AnkiConditionalErrorAndReturnValue(mxQuad != NULL, Quadrilateral<Type>(),
                                       "Anki.GetQuad", "No variable named '%s' found.", name.c_str());
    AnkiConditionalErrorAndReturnValue(mxGetM(mxQuad)==4 && mxGetN(mxQuad)==2, Quadrilateral<Type>(),
                                       "Anki.GetQuad", "Variable '%s' is not 4x2 in size.", name.c_str());
    AnkiConditionalErrorAndReturnValue(mxGetClassID(mxQuad)==mxDOUBLE_CLASS, Quadrilateral<Type>(),
                                       "Anki.GetQuad", "Variable '%s' must be DOUBLE to get as quad.", name.c_str());
    
    const double* x = mxGetPr(mxQuad);
    const double* y = x + 4;
    
    return Quadrilateral<Type>(Point<Type>(static_cast<Type>(x[0]), static_cast<Type>(y[0])),
                               Point<Type>(static_cast<Type>(x[1]), static_cast<Type>(y[1])),
                               Point<Type>(static_cast<Type>(x[2]), static_cast<Type>(y[2])),
                               Point<Type>(static_cast<Type>(x[3]), static_cast<Type>(y[3])));
  } // GetQuad()
  
  
} // namespace Embedded
} // namespace Anki

#endif // #if ANKICORETECH_EMBEDDED_USE_MATLAB

#endif // ANKI_CORETECH_COMMON_EMBEDDED_MATLAB_INTERFACE_H_

