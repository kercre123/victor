#ifndef _MEX_EMBEDDED_WRAPPERS_H_
#define _MEX_EMBEDDED_WRAPPERS_H_

#include "mex.h"
#include "anki/embeddedCommon.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> void mxArrayToArray(const mxArray * const array, Array<Type> &mat)
    {
      const int npixels = mat.get_size(0)*mat.get_size(1);
      const int nrows = mat.get_size(0);
      const int ncols = mat.get_size(1);

      const Type * const matlabMatrixStartPointer = reinterpret_cast<const Type *>( mxGetData(array) );

      const mwSize numMatlabElements = mxGetNumberOfElements(array);

      if(numMatlabElements != npixels) {
        mexPrintf("mxArrayToArray<Type>(array,mat) - Matlab array has a different number of elements than the Anki::Embedded::Array (%d != %d)\n", numMatlabElements, npixels);
        return;
      }

      const mxClassID matlabClassId = mxGetClassID(array);
      const mxClassID templateClassId = ConvertToMatlabType(typeid(Type).name(), sizeof(Type));
      if(matlabClassId != templateClassId) {
        mexPrintf("mxArrayToArray<Type>(array,mat) - Matlab classId does not match with template %d!=%d\n", matlabClassId, templateClassId);
        return;
      }

      for(s32 y=0; y<nrows; ++y) {
        Type * const array_rowPointer = mat.Pointer(y, 0);
        const Type * const matlabMatrix_rowPointer = matlabMatrixStartPointer + y*ncols;

        for(s32 x=0; x<ncols; ++x) {
          array_rowPointer[x] = matlabMatrix_rowPointer[x];
        }
      }
    }

    template<typename Type> Array<Type> mxArrayToArray(const mxArray * const matlabArray)
    {
      const Type * const matlabMatrixStartPointer = reinterpret_cast<const Type *>( mxGetData(matlabArray) );

      const mwSize numMatlabElements = mxGetNumberOfElements(matlabArray);
      const mwSize numDimensions = mxGetNumberOfDimensions(matlabArray);
      const mwSize *dimensions = mxGetDimensions(matlabArray);

      if(numDimensions != 2) {
        mexPrintf("mxArrayToArray<Type> - Matlab array must be 2D\n");
        return Array<Type>();
      }

      const mxClassID matlabClassId = mxGetClassID(matlabArray);
      const mxClassID templateClassId = ConvertToMatlabType(typeid(Type).name(), sizeof(Type));
      if(matlabClassId != templateClassId) {
        mexPrintf("mxArrayToArray<Type> - Matlab classId does not match with template %d!=%d\n", matlabClassId, templateClassId);
        return Array<Type>();
      }

      Array<Type> array = AllocateArrayFromHeap<Type>(dimensions[0], dimensions[1]);

      for(s32 y=0; y<dimensions[0]; ++y) {
        Type * const array_rowPointer = array.Pointer(y, 0);

        for(s32 x=0; x<dimensions[1]; ++x) {
          array_rowPointer[x] = *(matlabMatrixStartPointer + x*dimensions[0] + y);
        }
      }

      return array;
    }

    template<typename Type> mxArray* arrayToMxArray(const Array<Type> &array)
    {
      const mxClassID classId = ConvertToMatlabType(typeid(Type).name(), sizeof(Type));

      const mwSize outputDims[2] = {array.get_size(0), array.get_size(1)};
      mxArray *outputArray = mxCreateNumericArray(2, outputDims, classId, mxREAL);
      Type * const matlabMatrixStartPointer = (Type *) mxGetData(outputArray);

      for(s32 y=0; y<outputDims[0]; ++y) {
        const Type * const array_rowPointer = array.Pointer(y, 0);

        for(s32 x=0; x<outputDims[1]; ++x) {
          *(matlabMatrixStartPointer + x*outputDims[0] + y) = array_rowPointer[x];
        }
      }

      return outputArray;
    }
  } // namespace Embedded
} // namespace Anki

#endif // #ifndef _MEX_EMBEDDED_WRAPPERS_H_
