#ifndef _ANKICORETECHEMBEDDED_COMMON_MATLAB_INTERFACE_H_
#define _ANKICORETECHEMBEDDED_COMMON_MATLAB_INTERFACE_H_

#include "anki/embeddedCommon/config.h"
#include "anki/embeddedCommon/array2d.h"

#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)

#include <string>
#include <list>
#include <vector>

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#include "engine.h"
#include "matrix.h"

namespace Anki
{
  namespace Embedded
  {
    enum MatlabVariableType {TYPE_UNKNOWN, TYPE_INT8, TYPE_UINT8, TYPE_INT16, TYPE_UINT16, TYPE_INT32, TYPE_UINT32, TYPE_INT64, TYPE_UINT64, TYPE_SINGLE, TYPE_DOUBLE};

    // Converts from typeid names to Matlab types
    mxClassID ConvertToMatlabType(const char *typeName, size_t byteDepth);
    const char * ConvertToMatlabTypeString(const char *typeName, size_t byteDepth);

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

#if defined(ANKI_USE_OPENCV)
      s32 GetIplImage(IplImage *im, const std::string name);
      s32 PutIplImage(const IplImage *im, const std::string name, bool flipRedBlue);

      s32 GetCvMat(const CvMat *matrix, const std::string name); //*mat must be initialized before passing to this function
      CvMat* GetCvMat(const std::string name);                   //like GetCvMat, but also creates the CvMat*
      s32 PutCvMat(const CvMat *matrix, const std::string name); //*mat must be initialized before passing to this function
#endif //#if defined(ANKI_USE_OPENCV)

      //Character strings need to be converted slightly to appear correctly in Matlab
      s32 PutString(const char * characters, s32 nValues, const std::string name);

      template<typename T> s32 PutArray2d(const Array2d<T> &matrix, const std::string name)
      {
        if(!ep) {
          DASError("Anki.PutArray2d", "Matlab engine is not started/connected");
          return -1;
        }

        const std::string tmpName = name + std::string("_AnkiTMP");
        const std::string matlabTypeName = Anki::Embedded::ConvertToMatlabTypeString(typeid(T).name(), sizeof(T));

        EvalStringEcho("%s=zeros([%d,%d],'%s');", name.data(), matrix.get_size(0), matrix.get_size(1), matlabTypeName.data());

        for(s32 y=0; y<matrix.get_size(0); y++) {
          Put<T>(matrix.Pointer(y,0), matrix.get_size(1), tmpName);
          EvalStringEcho("%s(%d,:)=%s;", name.data(), y+1, tmpName.data());
        }

        EvalString("clear %s;", tmpName.data());

        return 0;
      }

      template<typename T> Array2d<T> GetArray2d(const std::string name)
      {
        if(!ep) {
          DASError("Anki.GetArray2d", "Matlab engine is not started/connected");
          return Array2d<T>(0, 0, NULL, 0, false);
        }

        const std::string tmpName = name + std::string("_AnkiTMP");

        EvalString("%s=%s';", tmpName.data(), name.data());
        mxArray *arrayTmp = GetArray(tmpName.data());

        if(!arrayTmp) {
          DASError("Anki.GetArray2d", "%s could not be got from Matlab", tmpName.data());
          return Array2d<T>(0, 0, NULL, 0, false);
        }

        const mxClassID ankiVisionClassId = Anki::Embedded::ConvertToMatlabType(typeid(T).name(), sizeof(T));

        const mxClassID matlabClassId = mxGetClassID(arrayTmp);

        if(matlabClassId != ankiVisionClassId) {
          DASError("Anki.GetArray2d", "matlabClassId != ankiVisionClassId");
          return Array2d<T>(0, 0, NULL, 0, false);
        }

        const size_t numCols = mxGetM(arrayTmp);
        const size_t numRows = mxGetN(arrayTmp);
        const s32 stride = Array2d<T>::ComputeRequiredStride(static_cast<s32>(numCols),false);

        Array2d<T> ankiArray2d(static_cast<s32>(numRows), static_cast<s32>(numCols), reinterpret_cast<T*>(calloc(stride*numCols,1)), stride*static_cast<s32>(numCols), false);

        T *matlabArrayTmp = reinterpret_cast<T*>(mxGetPr(arrayTmp));
        s32 matlabIndex = 0;
        for(s32 y=0; y<ankiArray2d.get_size(0); y++) {
          T * rowPointer = ankiArray2d.Pointer(y, 0);
          for(s32 x=0; x<ankiArray2d.get_size(1); x++) {
            rowPointer[x] = matlabArrayTmp[matlabIndex++];
          }
        }

        mxDestroyArray(arrayTmp);

        EvalString("clear %s;", tmpName.data());

        return ankiArray2d;
      }

      template<typename T> s32 Put(const T * values, s32 nValues, const std::string name)
      {
        if(!ep) {
          DASError("Anki.Put", "Matlab engine is not started/connected");
          return -1;
        }

        const mwSize dims[1] = {static_cast<mwSize>(nValues)};
        const mxClassID matlabType = Anki::Embedded::ConvertToMatlabType(typeid(T).name(), sizeof(T));
        mxArray *arrayTmp = mxCreateNumericArray(1, &dims[0], matlabType, mxREAL);
        T *matlabBufferTmp = (T*) mxGetPr(arrayTmp);
        for(s32 i = 0; i<nValues; i++) {
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
          for(s32 i = 0; i<(s32)size; i++) {
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
  } // namespace Embedded
} // namespace Anki

#endif // #if defined(ANKI_USE_MATLAB)

#endif // _ANKICORETECHEMBEDDED_COMMON_MATLAB_INTERFACE_H_
