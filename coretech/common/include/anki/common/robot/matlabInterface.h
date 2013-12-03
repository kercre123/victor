/**
File: matlabInterface.h
Author: Peter Barnum
Created: 2013

A simple wrapper around the Matlab interface between C and matlab. These routines can be used to send and receive data from the Matlab process, and to execute commands in the Matlab window.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_MATLAB_INTERFACE_H_
#define _ANKICORETECHEMBEDDED_COMMON_MATLAB_INTERFACE_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/matlabConverters.h"
#include "anki/common/robot/errorHandling.h"

#if ANKICORETECH_EMBEDDED_USE_MATLAB

#include <string>
#include <list>
#include <vector>

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

//#include "mex.h"
#include "engine.h"
#include "matrix.h"

namespace Anki
{
  namespace Embedded
  {
    enum MatlabVariableType {TYPE_UNKNOWN, TYPE_INT8, TYPE_UINT8, TYPE_INT16, TYPE_UINT16, TYPE_INT32, TYPE_UINT32, TYPE_INT64, TYPE_UINT64, TYPE_SINGLE, TYPE_DOUBLE};

    // Converts from typeid names to Matlab types
    // mxClassID ConvertToMatlabType(const char *typeName, size_t byteDepth); // TODO: Remove
    std::string ConvertToMatlabTypeString(const char *typeName, size_t byteDepth);

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

    class Matlab {
    public:
      static const s32 COMMAND_BUFFER_SIZE = 100;

      Engine *ep;

      Matlab(bool clearWorkspace=false);

      std::string EvalString(const char * format, ...);

      //Same as EvalString, but in addition, Matlab will keep a std::list of the last few
      //EvalStringEcho calls, stored in "lastC2Mat2CommandBuffer"
      std::string EvalStringEcho(const char * format, ...);

      //Evaluate the std::string exactly as is, to prevent losing %d, etc.
      std::string EvalStringExplicit(const char * buffer);
      std::string EvalStringExplicitEcho(const char * buffer);

      MatlabVariableType GetType(const std::string name);

      //Check if the variable exists on the workspace
      bool DoesVariableExist(const std::string name);

      s32 SetVisible(bool isVisible);

      //Raw array access
      mxArray* GetArray(const std::string name);

      //Character strings need to be converted slightly to appear correctly in Matlab
      Result PutString(const char * characters, s32 nValues, const std::string name);

      template<typename Type> Result PutArray(const Array<Type> &matrix, const std::string name);

      template<typename Type> Array<Type> GetArray(const std::string name);

      template<typename Type> Result Put(const Type * values, s32 nValues, const std::string name);

      template<typename Type> Type* Get(const std::string name);
    }; // class Matlab

#pragma mark --- Implementations ---

    template<typename Type> Result Matlab::PutArray(const Array<Type> &matrix, const std::string name)
    {
      const s32 matrixHeight = matrix.get_size(0);
      const s32 matrixWidth = matrix.get_size(1);

      AnkiConditionalErrorAndReturnValue(ep, RESULT_FAIL, "Anki.PutArray<Type>", "Matlab engine is not started/connected");

      const std::string tmpName = name + std::string("_AnkiTMP");
      const std::string matlabTypeName = Anki::Embedded::ConvertToMatlabTypeString(typeid(Type).name(), sizeof(Type));

      EvalStringEcho("%s=zeros([%d,%d],'%s');", name.data(), matrixHeight, matrixWidth, matlabTypeName.data());

      for(s32 y=0; y<matrixHeight; y++) {
        Put<Type>(matrix.Pointer(y,0), matrixWidth, tmpName);
        EvalStringEcho("%s(%d,:)=%s;", name.data(), y+1, tmpName.data());
      }

      EvalString("clear %s;", tmpName.data());

      return RESULT_OK;
    } // template<typename Type> Result Matlab::PutArray(const Array<Type> &matrix, const std::string name)

    template<typename Type> Array<Type> Matlab::GetArray(const std::string name)
    {
      AnkiConditionalErrorAndReturnValue(ep, Array<Type>(0, 0, NULL, 0), "Anki.GetArray<Type>", "Matlab engine is not started/connected");

      const std::string tmpName = name + std::string("_AnkiTMP");

      EvalString("%s=%s';", tmpName.data(), name.data());
      mxArray *arrayTmp = GetArray(tmpName.data());

      AnkiConditionalErrorAndReturnValue(arrayTmp, Array<Type>(0, 0, NULL, 0), "Anki.GetArray<Type>", "%s could not be got from Matlab", tmpName.data());

      AnkiConditionalErrorAndReturnValue(mxGetClassID(arrayTmp) == getMatlabClassID<Type>(), Array<Type>(0, 0, NULL, 0), "Anki.GetArray<Type>", "matlabClassId != ankiVisionClassId");

      const size_t numCols = mxGetM(arrayTmp);
      const size_t numRows = mxGetN(arrayTmp);
      const s32 stride = Array<Type>::ComputeRequiredStride(static_cast<s32>(numCols), Flags::Buffer(true,false));

      Array<Type> ankiArray(static_cast<s32>(numRows), static_cast<s32>(numCols), reinterpret_cast<Type*>(calloc(stride*numCols,1)), stride*static_cast<s32>(numCols));

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
    } // template<typename Type> Array<Type> Matlab::GetArray(const std::string name)

    template<typename Type> Result Matlab::Put(const Type * values, s32 nValues, const std::string name)
    {
      AnkiConditionalErrorAndReturnValue(this->ep, RESULT_FAIL, "Anki.Put", "Matlab engine is not started/connected");

      const mwSize dims[1] = {static_cast<mwSize>(nValues)};
      const mxClassID matlabType = getMatlabClassID<Type>();
      mxArray *arrayTmp = mxCreateNumericArray(1, &dims[0], matlabType, mxREAL);
      Type *matlabBufferTmp = (Type*) mxGetPr(arrayTmp);
      for(s32 i = 0; i<nValues; i++) {
        matlabBufferTmp[i] = values[i];
      }
      engPutVariable(ep, name.data(), arrayTmp);
      mxDestroyArray(arrayTmp);

      return RESULT_OK;
    } // template<typename Type> Result Matlab::Put(const Type * values, s32 nValues, const std::string name)

    template<typename Type> Type* Matlab::Get(const std::string name)
    {
      AnkiConditionalErrorAndReturnValue(this->ep, NULL, "Anki.Get", "Matlab engine is not started/connected");

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
        AnkiError("Anki.Get", "No variable named %s exists on the workspace.\n", name.data());
        return NULL;
      }

      return val;
    } // template<typename Type> T* Matlab::Get(const std::string name)

    template<> Result Matlab::Put<Point<s16> >(const Point<s16> * values, s32 nValues, const std::string name);
  } // namespace Embedded
} // namespace Anki

#endif // #if defined(ANKI_USE_MATLAB)

#endif // _ANKICORETECHEMBEDDED_COMMON_MATLAB_INTERFACE_H_
