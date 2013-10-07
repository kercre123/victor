#ifndef _ANKICORETECHEMBEDDED_COMMON_MATLAB_INTERFACE_H_
#define _ANKICORETECHEMBEDDED_COMMON_MATLAB_INTERFACE_H_

#include "anki/embeddedCommon/config.h"
#include "anki/embeddedCommon/array2d.h"

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
    mxClassID ConvertToMatlabType(const char *typeName, size_t byteDepth);
    std::string ConvertToMatlabTypeString(const char *typeName, size_t byteDepth);

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
      Result GetIplImage(IplImage *im, const std::string name);
      Result PutIplImage(const IplImage *im, const std::string name, bool flipRedBlue);

      Result GetCvMat(const CvMat *matrix, const std::string name); //*mat must be initialized before passing to this function
      CvMat* GetCvMat(const std::string name);                   //like GetCvMat, but also creates the CvMat*
      Result PutCvMat(const CvMat *matrix, const std::string name); //*mat must be initialized before passing to this function
#endif //#if defined(ANKI_USE_OPENCV)

      //Character strings need to be converted slightly to appear correctly in Matlab
      Result PutString(const char * characters, s32 nValues, const std::string name);

      template<typename Type> Result PutArray(const Array<Type> &matrix, const std::string name);

      template<typename Type> Array<Type> GetArray(const std::string name);

      template<typename Type> Result Put(const Type * values, s32 nValues, const std::string name);

      template<typename Type> Type* Get(const std::string name);
    }; // class Matlab

    // Convert a Matlab mxArray to an Anki::Array, without allocating memory
    template<typename Type> void mxArrayToArray(const mxArray * const array, Array<Type> &mat);

    // Convert a Matlab mxArray to an Anki::Array. Allocate and return the Anki::Array
    template<typename Type> Array<Type> mxArrayToArray(const mxArray * const matlabArray);

    // Convert an Anki::Array to a Matlab mxArray. Allocate and return the mxArray
    template<typename Type> mxArray* arrayToMxArray(const Array<Type> &array);

#pragma mark --- Implementations ---

    template<typename Type> Result Matlab::PutArray(const Array<Type> &matrix, const std::string name)
    {
      AnkiConditionalErrorAndReturnValue(ep, RESULT_FAIL, "Anki.PutArray<Type>", "Matlab engine is not started/connected");

      const std::string tmpName = name + std::string("_AnkiTMP");
      const std::string matlabTypeName = Anki::Embedded::ConvertToMatlabTypeString(typeid(Type).name(), sizeof(Type));

      EvalStringEcho("%s=zeros([%d,%d],'%s');", name.data(), matrix.get_size(0), matrix.get_size(1), matlabTypeName.data());

      for(s32 y=0; y<matrix.get_size(0); y++) {
        Put<Type>(matrix.Pointer(y,0), matrix.get_size(1), tmpName);
        EvalStringEcho("%s(%d,:)=%s;", name.data(), y+1, tmpName.data());
      }

      EvalString("clear %s;", tmpName.data());

      return RESULT_OK;
    } // template<typename Type> Result Matlab::PutArray(const Array<Type> &matrix, const std::string name)

    template<typename Type> Array<Type> Matlab::GetArray(const std::string name)
    {
      AnkiConditionalErrorAndReturnValue(ep, Array<Type>(0, 0, NULL, 0, false), "Anki.GetArray<Type>", "Matlab engine is not started/connected");

      const std::string tmpName = name + std::string("_AnkiTMP");

      EvalString("%s=%s';", tmpName.data(), name.data());
      mxArray *arrayTmp = GetArray(tmpName.data());

      AnkiConditionalErrorAndReturnValue(arrayTmp, Array<Type>(0, 0, NULL, 0, false), "Anki.GetArray<Type>", "%s could not be got from Matlab", tmpName.data());

      AnkiConditionalErrorAndReturnValue(mxGetClassID(arrayTmp) == ConvertToMatlabType(typeid(Type).name(), sizeof(Type)), Array<Type>(0, 0, NULL, 0, false), "Anki.GetArray<Type>", "matlabClassId != ankiVisionClassId");

      const size_t numCols = mxGetM(arrayTmp);
      const size_t numRows = mxGetN(arrayTmp);
      const s32 stride = Array<Type>::ComputeRequiredStride(static_cast<s32>(numCols),false);

      Array<Type> ankiArray(static_cast<s32>(numRows), static_cast<s32>(numCols), reinterpret_cast<Type*>(calloc(stride*numCols,1)), stride*static_cast<s32>(numCols), false);

      Type *matlabArrayTmp = reinterpret_cast<Type*>(mxGetPr(arrayTmp));
      s32 matlabIndex = 0;
      for(s32 y=0; y<ankiArray.get_size(0); y++) {
        Type * rowPointer = ankiArray.Pointer(y, 0);
        for(s32 x=0; x<ankiArray.get_size(1); x++) {
          rowPointer[x] = matlabArrayTmp[matlabIndex++];
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
      const mxClassID matlabType = Anki::Embedded::ConvertToMatlabType(typeid(Type).name(), sizeof(Type));
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

    template<typename Type> void mxArrayToArray(const mxArray * const array, Array<Type> &mat)
    {
      const int npixels = mat.get_size(0)*mat.get_size(1);
      const int nrows = mat.get_size(0);
      const int ncols = mat.get_size(1);

      const Type * const matlabMatrixStartPointer = reinterpret_cast<const Type *>( mxGetData(array) );

      const mwSize numMatlabElements = mxGetNumberOfElements(array);

      if(numMatlabElements != npixels) {
        AnkiError("mxArrayToArray", "mxArrayToArray<Type>(array,mat) - Matlab array has a different number of elements than the Anki::Embedded::Array (%d != %d)\n", numMatlabElements, npixels);
        return;
      }

      const mxClassID matlabClassId = mxGetClassID(array);
      const mxClassID templateClassId = ConvertToMatlabType(typeid(Type).name(), sizeof(Type));
      if(matlabClassId != templateClassId) {
        AnkiError("mxArrayToArray", "mxArrayToArray<Type>(array,mat) - Matlab classId does not match with template %d!=%d\n", matlabClassId, templateClassId);
        return;
      }

      for(s32 y=0; y<nrows; ++y) {
        Type * const array_rowPointer = mat.Pointer(y, 0);
        const Type * const matlabMatrix_rowPointer = matlabMatrixStartPointer + y*ncols;

        for(s32 x=0; x<ncols; ++x) {
          array_rowPointer[x] = matlabMatrix_rowPointer[x];
        }
      }
    } // template<typename Type> void mxArrayToArray(const mxArray * const array, Array<Type> &mat)

    template<typename Type> Array<Type> mxArrayToArray(const mxArray * const matlabArray)
    {
      const Type * const matlabMatrixStartPointer = reinterpret_cast<const Type *>( mxGetData(matlabArray) );

      //const mwSize numMatlabElements = mxGetNumberOfElements(matlabArray);
      const mwSize numDimensions = mxGetNumberOfDimensions(matlabArray);
      const mwSize *dimensions = mxGetDimensions(matlabArray);

      if(numDimensions != 2) {
        AnkiError("mxArrayToArray", "mxArrayToArray<Type> - Matlab array must be 2D\n");
        return Array<Type>();
      }

      const mxClassID matlabClassId = mxGetClassID(matlabArray);
      const mxClassID templateClassId = ConvertToMatlabType(typeid(Type).name(), sizeof(Type));
      if(matlabClassId != templateClassId) {
        AnkiError("mxArrayToArray", "mxArrayToArray<Type> - Matlab classId does not match with template %d!=%d\n", matlabClassId, templateClassId);
        return Array<Type>();
      }

      Array<Type> array = AllocateArrayFromHeap<Type>(dimensions[0], dimensions[1]);

      for(mwSize y=0; y<dimensions[0]; ++y) {
        Type * const array_rowPointer = array.Pointer(static_cast<s32>(y), 0);

        for(mwSize x=0; x<dimensions[1]; ++x) {
          array_rowPointer[x] = *(matlabMatrixStartPointer + x*dimensions[0] + y);
        }
      }

      return array;
    } // template<typename Type> Array<Type> mxArrayToArray(const mxArray * const matlabArray)

    template<typename Type> mxArray* arrayToMxArray(const Array<Type> &array)
    {
      const mxClassID classId = ConvertToMatlabType(typeid(Type).name(), sizeof(Type));

      const mwSize outputDims[2] = {static_cast<mwSize>(array.get_size(0)),
        static_cast<mwSize>(array.get_size(1))};

      mxArray *outputArray = mxCreateNumericArray(2, outputDims, classId, mxREAL);
      Type * const matlabMatrixStartPointer = (Type *) mxGetData(outputArray);

      for(mwSize y=0; y<outputDims[0]; ++y) {
        const Type * const array_rowPointer = array.Pointer(static_cast<s32>(y), 0);

        for(mwSize x=0; x<outputDims[1]; ++x) {
          *(matlabMatrixStartPointer + x*outputDims[0] + y) = array_rowPointer[x];
        }
      }

      return outputArray;
    } // template<typename Type> mxArray* arrayToMxArray(const Array<Type> &array)

    template<> Result Matlab::Put<Point<s16> >(const Point<s16> * values, s32 nValues, const std::string name);
  } // namespace Embedded
} // namespace Anki

#endif // #if defined(ANKI_USE_MATLAB)

#endif // _ANKICORETECHEMBEDDED_COMMON_MATLAB_INTERFACE_H_
