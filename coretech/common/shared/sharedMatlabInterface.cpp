/**
File: sharedMatlabInterface.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/shared/sharedMatlabInterface.h"
//#include "coretech/common/robot/errorHandling.h"

#if (defined(ANKICORETECH_EMBEDDED_USE_MATLAB) && ANKICORETECH_EMBEDDED_USE_MATLAB) || \
  (defined(ANKICORETECH_USE_MATLAB) && ANKICORETECH_USE_MATLAB)

#ifdef _MSC_VER
#include <stdarg.h>

#ifndef snprintf
#define snprintf sprintf_s
#endif
#endif

namespace Anki {
#define TEXT_BUFFER_SIZE 1024

  std::string ConvertToMatlabTypeString(const char *typeName, size_t byteDepth)
  {
#if defined(__APPLE_CC__) // Apple Xcode
    if(typeName[0] == 'h') {
      return std::string("uint8");
    } else if(typeName[0] == 'a') {
      return std::string("int8");
    } else if(typeName[0] == 't') {
      return std::string("uint16");
    } else if(typeName[0] == 's') {
      return std::string("int16");
    } else if(typeName[0] == 'j') {
      return std::string("uint32");
    } else if(typeName[0] == 'i') {
      return std::string("int32");
    } else if(typeName[0] == 'y') {
      return std::string("uint64");
    } else if(typeName[0] == 'x') {
      return std::string("int64");
    } else if(typeName[0] == 'f') {
      return std::string("single");
    } else if(typeName[0] == 'd') {
      return std::string("double");
    }
#else // #if defined(__APPLE_CC__) // Apple Xcode
    if(typeName[0] == 'u') { //unsigned
      if(byteDepth == 1) {
        return std::string("uint8");
      } else if(byteDepth == 2) {
        return std::string("uint16");
      }else if(byteDepth == 4) {
        return std::string("uint32");
      }else if(byteDepth == 8) {
        return std::string("uint64");
      }
    } else if(typeName[0] == 'f' && byteDepth == 4) { //float
      return std::string("single");
    } else if(typeName[0] == 'd' && byteDepth == 8) { //double
      return std::string("double");
    } else { // signed
      if(byteDepth == 1) {
        return std::string("int8");
      } else if(byteDepth == 2) {
        return std::string("int16");
      }else if(byteDepth == 4) {
        return std::string("int32");
      }else if(byteDepth == 8) {
        return std::string("int64");
      }
    }
#endif // #if defined(__APPLE_CC__) // Apple Xcode ... #else

    return std::string("unknown");
  } // std::string convertToMatlabTypeString(const char *typeName, size_t byteDepth)

  /* Redundant with ones in matlabConverters.h
  template<> mxClassID SharedMatlabInterface::GetMatlabClassID<u8>() {
  return mxUINT8_CLASS;
  }

  template<> mxClassID SharedMatlabInterface::GetMatlabClassID<s8>() {
  return mxINT8_CLASS;
  }

  template<> mxClassID SharedMatlabInterface::GetMatlabClassID<u16>() {
  return mxUINT16_CLASS;
  }

  template<> mxClassID SharedMatlabInterface::GetMatlabClassID<s16>() {
  return mxINT16_CLASS;
  }

  template<> mxClassID SharedMatlabInterface::GetMatlabClassID<u32>() {
  return mxUINT32_CLASS;
  }

  template<> mxClassID SharedMatlabInterface::GetMatlabClassID<s32>() {
  return mxINT32_CLASS;
  }

  template<> mxClassID SharedMatlabInterface::GetMatlabClassID<u64>() {
  return mxUINT64_CLASS;
  }

  template<> mxClassID SharedMatlabInterface::GetMatlabClassID<s64>() {
  return mxINT64_CLASS;
  }

  template<> mxClassID SharedMatlabInterface::GetMatlabClassID<f32>() {
  return mxSINGLE_CLASS;
  }

  template<> mxClassID SharedMatlabInterface::GetMatlabClassID<f64>() {
  return mxDOUBLE_CLASS;
  }
  */

  SharedMatlabInterface::SharedMatlabInterface(bool clearWorkspace)
  {
    // Multithreading under Windows requires this command
    // CoInitializeEx(NULL, COINIT_MULTITHREADED);

    ep = engOpen(NULL);

    if(!ep) {
      LOG_ERROR("SharedMatlabInterface Constructor", "Failed to open Matlab engine!");
    }

    if(clearWorkspace){
      EvalString("clear all");
    }

    EvalString("lastAnkiCommandBuffer=cell(0, 1);");
  }

  std::string SharedMatlabInterface::EvalString(const char * const format, ...)
  {
    if(!ep) {
      //AnkiConditionalErrorAndReturnValue(this->ep, "", "Anki.", "Matlab engine is not started/connected");
      LOG_ERROR("Anki.", "Matlab engine is not started/connected");
      return "";
    }
    va_list args;
    char *buffer;

    va_start( args, format );

#ifdef _MSC_VER
    const u32 len = _vscprintf( format, args ) + 1;
#else
    const u32 len = 1024;
#endif

    buffer = (char*) malloc( len * sizeof(char) );
    vsnprintf( buffer, len, format, args );

    engEvalString(this->ep, buffer);

    std::string toReturn = std::string(buffer);
    free( buffer );

    return toReturn;
  }

  std::string SharedMatlabInterface::EvalStringEcho(const char * const format, ...)
  {
    if(!ep) {
      // AnkiConditionalErrorAndReturnValue(this->ep, "", "Anki.", "Matlab engine is not started/connected");
      LOG_ERROR("Anki.", "Matlab engine is not started/connected");
      return "";
    }

    va_list args;
    char *buffer;

    va_start( args, format );

#ifdef _MSC_VER
    const u32 len = _vscprintf( format, args ) + 1;
#else
    const u32 len = 1024;
#endif

    buffer = (char*) malloc( len * sizeof(char) );
    vsnprintf( buffer, len, format, args );

    engEvalString(this->ep, buffer);

    std::string toReturn = std::string(buffer);
    PutString(buffer, (int)toReturn.size(), "lastAnkiCommand");

    EvalString("if length(lastAnkiCommandBuffer)==%d lastAnkiCommandBuffer=lastAnkiCommandBuffer(2:end); end; lastAnkiCommandBuffer{end+1}=lastAnkiCommand;", SharedMatlabInterface::COMMAND_BUFFER_SIZE);

    free( buffer );

    return toReturn;
  }

  std::string SharedMatlabInterface::EvalStringExplicit(const char * buffer)
  {
    if(!ep) {
      // AnkiConditionalErrorAndReturnValue(this->ep, "", "Anki.", "Matlab engine is not started/connected");
      LOG_ERROR("Anki.", "Matlab engine is not started/connected");
      return "";
    }
    engEvalString(this->ep, buffer);

    std::string toReturn = std::string(buffer);

    return toReturn;
  }

  std::string SharedMatlabInterface::EvalStringExplicitEcho(const char * buffer)
  {
    if(!ep) {
      // AnkiConditionalErrorAndReturnValue(this->ep, "", "Anki.", "Matlab engine is not started/connected");
      LOG_ERROR("Anki.", "Matlab engine is not started/connected");
      return "";
    }
    engEvalString(this->ep, buffer);

    std::string toReturn = std::string(buffer);
    PutString(buffer, (int)toReturn.size(), "lastAnkiCommand");

    return toReturn;
  }

  mxArray* SharedMatlabInterface::GetArray(const std::string name)
  {
    if(!ep) {
      // AnkiConditionalErrorAndReturnValue(this->ep, NULL, "Anki.", "Matlab engine is not started/connected");
      LOG_ERROR("Anki.", "Matlab engine is not started/connected");
      return NULL;
    }
    mxArray *arr = engGetVariable(this->ep, name.data());
    return arr;
  }

  MatlabVariableType SharedMatlabInterface::GetType(const std::string name)
  {
    if(!ep) {
      // AnkiConditionalErrorAndReturnValue(this->ep, TYPE_UNKNOWN, "Anki.", "Matlab engine is not started/connected");
      LOG_ERROR("Anki.", "Matlab engine is not started/connected");
      return TYPE_UNKNOWN;
    }

    char typeName[TEXT_BUFFER_SIZE];
    snprintf(typeName, TEXT_BUFFER_SIZE, "%s_types", name.data());
    EvalStringEcho("%s=int32([isa(%s, 'int8'), isa(%s, 'u8'), isa(%s, 'int16'), isa(%s, 'u16'), isa(%s, 'int32'), isa(%s, 'u32'), isa(%s, 'int64'), isa(%s, 'u64'), isa(%s, 'single'), isa(%s, 'double')]);", typeName, name.data(), name.data(), name.data(), name.data(), name.data(), name.data(), name.data(), name.data(), name.data(), name.data());
    int *types = Get<s32>(typeName);
    EvalStringEcho("clear %s;", typeName);

    MatlabVariableType type;

    if(types[0] == 1){type=TYPE_INT8;}
    else if(types[1] == 1){type=TYPE_UINT8;}
    else if(types[2] == 1){type=TYPE_INT16;}
    else if(types[3] == 1){type=TYPE_UINT16;}
    else if(types[4] == 1){type=TYPE_INT32;}
    else if(types[5] == 1){type=TYPE_UINT32;}
    else if(types[6] == 1){type=TYPE_INT64;}
    else if(types[7] == 1){type=TYPE_UINT64;}
    else if(types[8] == 1){type=TYPE_SINGLE;}
    else if(types[9] == 1){type=TYPE_DOUBLE;}
    else{type = TYPE_UNKNOWN;}

    free(types);
    types = 0;

    return type;
  }

#if defined(ANKI_USE_OPENCV)
  Result SharedMatlabInterface::GetCvMat(const CvMat *matrix, std::string name)
  {
    if(!ep) {
      // AnkiConditionalErrorAndReturnValue(this->ep, RESULT_FAIL, "Anki.", "Matlab engine is not started/connected");
      LOG_ERROR("Anki.", "Matlab engine is not started/connected");
      return RESULT_FAIL;
    }

    if(matrix == NULL) {
      // AnkiConditionalErrorAndReturnValue(matrix != NULL, RESULT_FAIL, "Error: CvMat is not initialized for %s\n", name.data());
      LOG_ERROR("Anki.", "Error: CvMat is not initialized for %s\n", name.data());
      return RESULT_FAIL;
    }

    char tmpName[TEXT_BUFFER_SIZE];
    snprintf(tmpName, TEXT_BUFFER_SIZE, "%s_AnkiTMP", name.data());
    EvalString("%s=%s';", tmpName, name.data());
    mxArray *arrayTmp = GetArray(tmpName);

    mxClassID classId = mxGetClassID(arrayTmp);

    mwSize size = mxGetNumberOfElements(arrayTmp);

    bool mismatch = true;
    if(classId == mxDOUBLE_CLASS) {
      //if(mat->type == CV_64F)
      {
        double *valTmp = (double*)mxGetPr(arrayTmp);
        for(s32 i = 0; i<size; i++) {
          matrix->data.db[i] = valTmp[i];
        }
        mismatch = false;
      }
    }else if(classId == mxSINGLE_CLASS) {
      //if(mat->type == CV_32F)
      {
        float *valTmp = (float*)mxGetPr(arrayTmp);
        for(int i = 0; i<(s32)size; i++) {
          matrix->data.fl[i] = valTmp[i];
        }
        mismatch = false;
      }
    }else if(classId == mxINT16_CLASS) {
      //if(mat->type == CV_16S)
      {
        s16 *valTmp = (s16*)mxGetPr(arrayTmp);
        for(int i = 0; i<(s32)size; i++) {
          matrix->data.s[i] = valTmp[i];
        }
        mismatch = false;
      }
    }else if(classId == mxUINT16_CLASS) {
      //if(mat->type == CV_16U)
      {
        u16 *valTmp = (u16*)mxGetPr(arrayTmp);
        for(int i = 0; i<(s32)size; i++) {
          matrix->data.s[i] = valTmp[i];
        }
        mismatch = false;
      }
    }else if(classId == mxINT32_CLASS) {
      //if(mat->type == CV_32S)
      {
        s32 *valTmp = (s32*)mxGetPr(arrayTmp);
        for(int i = 0; i<(s32)size; i++) {
          matrix->data.i[i] = valTmp[i];
        }
        mismatch = false;
      }
    } else {
      CoreTechPrint("Error: Class ID not supported for %s\n", name.data());
      EvalString("clear %s;", tmpName);
      return -1;
    }

    mxDestroyArray(arrayTmp);

    if(mismatch) {
      CoreTechPrint("Error: Class mismatch for %s\n", name.data());
      EvalString("clear %s;", tmpName);
      return -1;
    }

    EvalString("clear %s;", tmpName);
    return 0;
  }

  //like GetCvMat, but also allocates the CvMat* from the heap
  CvMat* SharedMatlabInterface::GetCvMat(const std::string name)
  {
    CvMat* matrix = 0;

    char tmpName[TEXT_BUFFER_SIZE];
    snprintf(tmpName, TEXT_BUFFER_SIZE, "%s_AnkiTMP", name.data());
    EvalString("%s=%s';", tmpName, name.data());
    mxArray *arrayTmp = GetArray(tmpName);

    mxClassID classId = mxGetClassID(arrayTmp);
    mwSize nDims = mxGetNumberOfDimensions(arrayTmp);

    if(nDims != 2) {
      CoreTechPrint("Error: Matrix %s must be 2D\n", name.data());
      return matrix;
    }

    const mwSize *dims = mxGetDimensions(arrayTmp);
    mwSize size = mxGetNumberOfElements(arrayTmp);

    if(classId == mxDOUBLE_CLASS) {
      double *valTmp = (double*)mxGetPr(arrayTmp);
      matrix = cvCreateMat((int)dims[1], (int)dims[0], CV_64F);
      for(int i = 0; i<(int)size; i++) { matrix->data.db[i] = valTmp[i]; }
    }else if(classId == mxSINGLE_CLASS) {
      float *valTmp = (float*)mxGetPr(arrayTmp);
      matrix = cvCreateMat((int)dims[1], (int)dims[0], CV_32F);
      for(int i = 0; i<(int)size; i++) { matrix->data.fl[i] = valTmp[i]; }
    }else if(classId == mxINT16_CLASS) {
      s16 *valTmp = (s16*)mxGetPr(arrayTmp);
      matrix = cvCreateMat((int)dims[1], (int)dims[0], CV_16S);
      for(int i = 0; i<(int)size; i++) { matrix->data.s[i] = valTmp[i]; }
    }else if(classId == mxUINT16_CLASS) {
      u16 *valTmp = (u16*)mxGetPr(arrayTmp);
      matrix = cvCreateMat((int)dims[1], (int)dims[0], CV_16U);
      for(int i = 0; i<(int)size; i++) {matrix->data.s[i] = valTmp[i]; }
    }else if(classId == mxINT32_CLASS) {
      s32 *valTmp = (s32*)mxGetPr(arrayTmp);
      matrix = cvCreateMat((int)dims[1], (int)dims[0], CV_32S);
      for(int i = 0; i<(int)size; i++) { matrix->data.i[i] = valTmp[i]; }
    } else {
      CoreTechPrint("Error: Class ID not supported for %s\n", name.data());
      EvalString("clear %s;", tmpName);
      return matrix;
    }

    mxDestroyArray(arrayTmp);

    EvalString("clear %s;", tmpName);
    return matrix;
  }

  Result SharedMatlabInterface::PutCvMat(const CvMat *matrix, std::string name)
  {
    AnkiConditionalErrorAndReturnValue(this->ep, RESULT_FAIL, "Anki.SharedMatlabInterface.PutCvMat",
                                       "Matlab engine is not started/connected");

    AnkiConditionalErrorAndReturnValue(matrix != NULL, RESULT_FAIL, "Anki.SharedMatlabInterface.PutCvMat",
                                       "CvMat is not initialized for %s", name.c_str());

    char tmpName[TEXT_BUFFER_SIZE];
    snprintf(tmpName, TEXT_BUFFER_SIZE, "%s_AnkiTMP", name.data());

    if((matrix->type&CV_MAT_DEPTH_MASK) == CV_64F) {
      Put<f64>(matrix->data.db, matrix->rows*matrix->step/sizeof(f64), tmpName);
    }else if((matrix->type&CV_MAT_DEPTH_MASK) == CV_32F) {
      Put<f32>(matrix->data.fl, matrix->rows*matrix->step/sizeof(f32), tmpName);
    }else if((matrix->type&CV_MAT_DEPTH_MASK) == CV_16S) {
      Put<s16>(matrix->data.s, matrix->rows*matrix->step/sizeof(s16), tmpName);
    }else if((matrix->type&CV_MAT_DEPTH_MASK) == CV_16U) {
      Put<u16>((const unsigned short*)matrix->data.s, matrix->rows*matrix->step/sizeof(u16), tmpName);
    }else if((matrix->type&CV_MAT_DEPTH_MASK) == CV_32S) {
      Put<s32>(matrix->data.i, matrix->rows*matrix->step/sizeof(s32), tmpName);
    }else if((matrix->type&CV_MAT_DEPTH_MASK) == CV_8U) {
      Put<u8>(matrix->data.ptr, matrix->rows*matrix->step, tmpName);
    }else {
      printf("Error: Class ID 0x%x 0x%x not supported for %s\n", matrix->type&CV_MAT_DEPTH_MASK, matrix->type, name.data());
      EvalString("clear %s;", tmpName);
      return RESULT_FAIL;
    }

    const s32 numChannels = CV_MAT_CN(matrix->type);
    if(numChannels == 1) {
      EvalString("%s=reshape(%s, [%d, %d])';", name.data(), tmpName, matrix->cols, matrix->rows);
    } else {
      EvalString("%s=permute(reshape(%s, [%d, %d, %d]), [3,2,1]);", name.data(), tmpName, numChannels, matrix->cols, matrix->rows);

      if(numChannels == 3) {
        EvalString("%s=%s(:,:,end:-1:1);", name.data(), name.data());
      }
    }

    EvalString("clear %s;", tmpName);

    return RESULT_OK;
  }
#endif // #if defined(ANKI_USE_OPENCV)

  Result SharedMatlabInterface::PutString(const char * characters, s32 nValues, const std::string name)
  {
    if(!ep) {
      //AnkiConditionalErrorAndReturnValue(this->ep, RESULT_FAIL, "Anki.", "Matlab engine is not started/connected");
      LOG_ERROR("Anki.", "Matlab engine is not started/connected");
      return RESULT_FAIL;
    }

    Result returnVal = Put<s8>((s8*)characters, nValues, name);
    EvalString("%s=char(%s');", name.data(), name.data());

    return returnVal;
  }

  s32 SharedMatlabInterface::SetVisible(bool isVisible)
  {
    if(!ep) {
      //AnkiConditionalErrorAndReturnValue(this->ep, -1, "Anki.", "Matlab engine is not started/connected");
      LOG_ERROR("Anki.", "Matlab engine is not started/connected");
      return -1;
    }

    s32 returnVal = engSetVisible(this->ep, isVisible);

    return returnVal;
  }

  bool SharedMatlabInterface::DoesVariableExist(const std::string name)
  {
    if(!ep) {
      //AnkiConditionalErrorAndReturnValue(this->ep, "", "Anki.", "Matlab engine is not started/connected");
      LOG_ERROR("Anki.", "Matlab engine is not started/connected");
      return false;
    }

    EvalString("ans=exist('%s', 'var');", name.data());

    double *ans = Get<double>("ans");
    double ansVal = *ans;
    free(ans); ans = 0;
    if(ansVal<.5)
      return false;
    else
      return true;
  }
} // namespace Anki

#else

//
// Define a dummy symbol to silence linker warnings such as
//   "blah.cpp.o has no symbols"
//
// This symbol is never actually used and may be discarded during final linkage.
//
int coretech_common_shared_sharedMatlabInterface = 0;

#endif // #if (defined(ANKICORETECH_EMBEDDED_USE_MATLAB) && ANKICORETECH_EMBEDDED_USE_MATLAB) || (defined(ANKICORETECH_USE_MATLAB) && ANKICORETECH_USE_MATLAB)
