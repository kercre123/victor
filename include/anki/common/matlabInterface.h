
#ifndef _ANKICORETECH_COMMON_MATLAB_INTERFACE_H_
#define _ANKICORETECH_COMMON_MATLAB_INTERFACE_H_

#include "anki/common/config.h"

#if defined(ANKICORETECH_USE_MATLAB)

#include <string>
#include <list>
#include <vector>

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#include "engine.h"
#include "matrix.h"

namespace Anki
{
enum MatlabVariableType {TYPE_UNKNOWN, TYPE_INT8, TYPE_UINT8, TYPE_INT16, TYPE_UINT16, TYPE_INT32, TYPE_UINT32, TYPE_INT64, TYPE_UINT64, TYPE_SINGLE, TYPE_DOUBLE};
 
// Converts from typeid names to Matlab types
mxClassID ConvertToMatlabType(const char *typeName, size_t byteDepth);
std::string ConvertToMatlabTypeString(const char *typeName, size_t byteDepth);

class Matlab {
public:
  static const u32 COMMAND_BUFFER_SIZE = 100;

  Engine *ep;
  
  Matlab(bool clearWorkspace=false);

  std::string EvalString(const char * format, ...);

  //Same as EvalString, but in addition, Matlab will keep a std::list of the last few
  //EvalStringEcho calls, stored in "lastC2Mat2CommandBuffer"
  std::string EvalStringEcho(const char * format, ...);

  //Evaluate the std::string exactly as is, to prevent losing %d, etc.
  std::string EvalStringExplicit(const char * buffer);
  std::string EvalStringExplicitEcho(const char * buffer);

  Anki::MatlabVariableType GetType(const std::string name);

  //Check if the variable exists on the workspace
  bool DoesVariableExist(const std::string name);

  s32 SetVisible(bool isVisible);

  //Raw array access
  mxArray* GetArray(const std::string name);

  #if defined(ANKI_USE_OPENCV)
  s32 GetIplImage(IplImage *im, const std::string name);
  s32 PutIplImage(const IplImage *im, const std::string name, bool flipRedBlue);

  s32 GetCvMat(const CvMat *matrix, const std::string name); //*mat must be initialized before passing to this function
  CvMat* GetCvMat(const std::string name);                   //like GetCvMat, but also creates the CvMat*
  s32 PutCvMat(const CvMat *matrix, const std::string name); //*mat must be initialized before passing to this function
  #endif //#if defined(ANKI_USE_OPENCV)

  //Character strings need to be converted slightly to appear correctly in Matlab
  s32 PutString(const char * characters, u32 nValues, const std::string name);

  template<typename T> s32 PutMatrix(const Anki::Matrix<T> &matrix, const std::string name)
  {
    if(!ep) {
      DASError("Anki.PutMatrix", "Matlab engine is not started/connected");
      return -1;
    }

    const std::string tmpName = name + std::string("_AnkiTMP");
    const std::string matlabTypeName = Anki::ConvertToMatlabTypeString(typeid(T).name(), sizeof(T));

    EvalStringEcho("%s=zeros([%d,%d],'%s');", name.data(), matrix.get_size(0), matrix.get_size(1), matlabTypeName.data());

    for(u32 y=0; y<matrix.get_size(0); y++) {
      Put<T>(matrix.Pointer(y,0), matrix.get_size(1), tmpName);
      EvalStringEcho("%s(%d,:)=%s;", name.data(), y+1, tmpName.data());
    }
  
    EvalString("clear %s;", tmpName.data());
  
    return 0;
  }

  template<typename T> Anki::Matrix<T> GetMatrix(const std::string name)
  {
    if(!ep) {
      DASError("Anki.GetMatrix", "Matlab engine is not started/connected");
      return Anki::Matrix<T>(0, 0, NULL, 0);
    }

    const std::string tmpName = name + std::string("_AnkiTMP");
        
    EvalString("%s=%s';", tmpName.data(), name.data());
    mxArray *arrayTmp = GetArray(tmpName.data());

    if(!arrayTmp) {
      DASError("Anki.GetMatrix", "%s could not be got from Matlab", tmpName.data());
      return Anki::Matrix<T>(0, 0, NULL, 0);
    }

    const mxClassID ankiVisionClassId = Anki::ConvertToMatlabType(typeid(T).name(), sizeof(T));

    const mxClassID matlabClassId = mxGetClassID(arrayTmp);
   
    if(matlabClassId != ankiVisionClassId) {
      DASError("Anki.GetMatrix", "matlabClassId != ankiVisionClassId");
      return Anki::Matrix<T>(0, 0, NULL, 0);
    }

    const size_t numCols = mxGetM(arrayTmp);
    const size_t numRows = mxGetN(arrayTmp);
    const u32 stride = Anki::RoundUp<u32>(static_cast<u32>(sizeof(T)*numCols), Anki::MEMORY_ALIGNMENT);

    Anki::Matrix<T> ankiMatrix(static_cast<u32>(numRows), static_cast<u32>(numCols), reinterpret_cast<T*>(calloc(stride*numCols,1)), stride);

    T *matlabArrayTmp = reinterpret_cast<T*>(mxGetPr(arrayTmp));
    u32 matlabIndex = 0;
    for(u32 y=0; y<ankiMatrix.get_size(0); y++) {
      T * rowPointer = ankiMatrix.Pointer(y, 0);
      for(u32 x=0; x<ankiMatrix.get_size(1); x++) {
        rowPointer[x] = matlabArrayTmp[matlabIndex++];
      }
    }

    mxDestroyArray(arrayTmp);

    EvalString("clear %s;", tmpName.data());

    return ankiMatrix;
  }

  template<typename T> s32 Put(const T * values, u32 nValues, const std::string name)
  {
    if(!ep) {
      DASError("Anki.Put", "Matlab engine is not started/connected");
      return -1;
    }

    const mwSize dims[1] = {nValues};
    const mxClassID matlabType = Anki::ConvertToMatlabType(typeid(T).name(), sizeof(T));
    mxArray *arrayTmp = mxCreateNumericArray(1, &dims[0], matlabType, mxREAL);
    T *matlabBufferTmp = (T*) mxGetPr(arrayTmp);
    for(u32 i = 0; i<nValues; i++) {
      matlabBufferTmp[i] = values[i];
    }
    engPutVariable(ep, name.data(), arrayTmp);
    mxDestroyArray(arrayTmp);

    return 0;
  }
  
  template<typename T> T* Get(const std::string name)
  {
    if(!ep) {
      DASError("Anki.Get", "Matlab engine is not started/connected");
      return NULL;
    }

    T *valTmp = 0, *val = 0;

    mxArray *arrayTmp = GetArray(name);

    if(arrayTmp) {
      const mwSize size = mxGetNumberOfElements(arrayTmp);
      valTmp = reinterpret_cast<T*>(mxGetPr(arrayTmp));
      val = reinterpret_cast<T*>(calloc(size, sizeof(T)));
      for(u32 i = 0; i<(s32)size; i++) {
        val[i] = valTmp[i];
      }
      mxDestroyArray(arrayTmp);
    } else {
      DASError("Anki.Get", "No variable named %s exists on the workspace.\n", name.data());
      return NULL;
    }

    return val;
  }
}; // class Matlab

} // namespace Anki

#endif // #if defined(ANKI_USE_MATLAB)

#endif // _ANKICORETECH_COMMON_MATLAB_INTERFACE_H_
