/**
* File: matlabConverters.h
*
* Author: Andrew Stein (andrew)
* Created: 10/9/2013
*
* Information on last revision to this file:
*    $LastChangedDate$
*    $LastChangedBy$
*    $LastChangedRevision$
*
* Description:
*
*   This files defines several conversion routines for translating
*   basic types back and forth from Matlab's mxArray type. It should
*   not call any Matlab routines (e.g. via an engine or mex), nor should
*   it rely on STL (i.e. std::<foo>), so that it can be used extremely
*   portably.
*
* Copyright: Anki, Inc. 2013
*
**/

#ifndef _ANKICORETECHEMBEDDED_MATLAB_CONVERTERS_H_
#define _ANKICORETECHEMBEDDED_MATLAB_CONVERTERS_H_

#include "anki/common/robot/config.h"

#if ANKICORETECH_EMBEDDED_USE_MATLAB

#include <typeinfo>

#include "anki/common/robot/array2d.h"

#include "engine.h"

namespace Anki {
  namespace Embedded {
    // #pragma mark --- Matlab Converter Definitions ---

    // Convert a Matlab mxArray to an Anki::Array, without allocating memory
    template<typename Type> void mxArrayToArray(const mxArray * const array, Array<Type> &mat);

    // Convert a Matlab mxArray to an Anki::Array. Allocate and return the Anki::Array
    template<typename Type> Array<Type> mxArrayToArray(const mxArray * const matlabArray, MemoryStack &memory);

    // This works with cell arrays if every element is the same type
    template<typename Type> Array<Array<Type> > mxCellArrayToArray(const mxArray * const matlabArray, MemoryStack &memory);

    // Copy a string to a null-terminated string, allocated from the MemoryStack
    // Overloaded version of Matlab's mxArrayToString() that uses a MemoryStack instead of the heap
    char* mxArrayToString(const mxArray * const matlabArray, MemoryStack &memory);

    // Convert a cell array of strings to a cell array of const pointers
    // The strings are allocated from "MemoryStack &memory", so do not need to be freed
    Array<char *> mxCellArrayToStringArray(const mxArray * const matlabArray, MemoryStack &memory);

    // Convert an Anki::Array to a Matlab mxArray. Allocate and return the mxArray
    template<typename Type> mxArray* arrayToMxArray(const Array<Type> &array);

    // Convert an array of null-terminated strings to a Matlab cell array
    mxArray* stringArrayToMxCellArray(const Array<const char *> &array);

    // Convert a raw image data pointer with size to a mxAarray.  Allocate and return the mxArray
    template<typename Type> mxArray* imageArrayToMxArray(const Type *data, const s32 nrows, const s32 ncols, const mwSize nbands);

    // Convert a primitive type to a C++ to a Matlab class ID
    // (If we have not defined a specialization for a particular type below,
    //  then mxUNKNOWN_CLASS gets returned by this parent template.)
    template<typename Type> mxClassID getMatlabClassID(void) { return mxUNKNOWN_CLASS; }

    // Forward declare the defined specializations for known types:
    template<> mxClassID getMatlabClassID<f32>(void);
    template<> mxClassID getMatlabClassID<f64>(void);

    template<> mxClassID getMatlabClassID<s16>(void);
    template<> mxClassID getMatlabClassID<u16>(void);

    template<> mxClassID getMatlabClassID<s8>(void);
    template<> mxClassID getMatlabClassID<u8>(void);

    template<> mxClassID getMatlabClassID<s32>(void);
    template<> mxClassID getMatlabClassID<u32>(void);

    template<> mxClassID getMatlabClassID<s64>(void);
    template<> mxClassID getMatlabClassID<u64>(void);

    // #pragma mark --- Matlab Converter Implementations ---

    template<typename Type> void mxArrayToArray(const mxArray * const array, Array<Type> &mat)
    {
      AnkiConditionalError(mxGetNumberOfDimensions(array) == 2,
        "mxArrayToArray", "Matlab array should be 2D.\n");

      const int nrows = mat.get_size(0);
      const int ncols = mat.get_size(1);

      const Type * const matlabMatrixStartPointer = reinterpret_cast<const Type *>( mxGetData(array) );

      if(mxGetM(array) != nrows || mxGetN(array) != ncols) {
        AnkiError("mxArrayToArray", "Matlab array size does not match Anki::Embedded::Array size: (%d x %d) vs (%d x %d).\n",
          mxGetM(array), mxGetN(array), nrows, ncols);
        return;
      }

      const mxClassID matlabClassId = mxGetClassID(array);
      const mxClassID templateClassId = getMatlabClassID<Type>();

      if(matlabClassId != templateClassId) {
        AnkiError("mxArrayToArray", "mxArrayToArray<Type>(array,mat) - Matlab classId does not match with template %d!=%d\n", matlabClassId, templateClassId);
        return;
      }

      for(s32 y=0; y<nrows; ++y) {
        Type * const pArray = mat.Pointer(y, 0);
        //const Type * const pMatlabMatrix = matlabMatrixStartPointer + y*ncols;
        const Type * const pMatlabMatrixRowY = matlabMatrixStartPointer + y;

        for(s32 x=0; x<ncols; ++x) {
          pArray[x] = *(pMatlabMatrixRowY + x*nrows);
        }
      }
    } // template<typename Type> void mxArrayToArray(const mxArray * const array, Array<Type> &mat)

    template<typename Type> Array<Type> mxArrayToArray(const mxArray * const matlabArray, MemoryStack &memory)
    {
      const Type * const matlabMatrixStartPointer = reinterpret_cast<const Type *>( mxGetData(matlabArray) );

      //const mwSize numMatlabElements = mxGetNumberOfElements(matlabArray);
      const mwSize numDimensions = mxGetNumberOfDimensions(matlabArray);
      const mwSize *dimensions = mxGetDimensions(matlabArray);

      if(numDimensions != 2) {
        AnkiError("mxArrayToArray", "mxArrayToArray<Type> - Matlab array must be 2D\n");
        return Array<Type>();
      }

      if(mxIsCell(matlabArray)) {
        AnkiError("mxArrayToArray", "Input can't be a cell array. Use mxCellArrayToArray() instead.");
        return Array<Type>(static_cast<s32>(dimensions[0]), static_cast<s32>(dimensions[1]), memory);
      }

      mxClassID matlabClassId;
      mxClassID templateClassId;

      matlabClassId = mxGetClassID(matlabArray);
      templateClassId = getMatlabClassID<Type>();

      if(matlabClassId != templateClassId) {
        AnkiError("mxArrayToArray", "mxArrayToArray<Type> - Matlab classId does not match with template %d!=%d\n", matlabClassId, templateClassId);
        return Array<Type>();
      }

      Array<Type> array(static_cast<s32>(dimensions[0]), static_cast<s32>(dimensions[1]), memory);

      if(!array.IsValid()) {
        AnkiError("mxArrayToArray", "mxArrayToArray<Type> - Could not allocate array\n");
        return Array<Type>();
      }

      for(mwSize y=0; y<dimensions[0]; ++y) {
        Type * const pArray = array.Pointer(static_cast<s32>(y), 0);

        for(mwSize x=0; x<dimensions[1]; ++x) {
          pArray[x] = *(matlabMatrixStartPointer + x*dimensions[0] + y);
        }
      }

      return array;
    } // template<typename Type> Array<Type> mxArrayToArray(const mxArray * const matlabArray)

    template<typename Type> Array<Array<Type> > mxCellArrayToArray(const mxArray * const matlabArray, MemoryStack &memory)
    {
      const mwSize numDimensions = mxGetNumberOfDimensions(matlabArray);
      const mwSize *dimensions = mxGetDimensions(matlabArray);

      if(numDimensions != 2) {
        AnkiError("mxCellArrayToArray", "mxCellArrayToArray<Type> - Matlab array must be 2D\n");
        return Array<Array<Type> >();
      }

      if(!mxIsCell(matlabArray)) {
        AnkiError("mxCellArrayToArray", "Input must be a cell array");
        return Array<Array<Type> >();
      }

      if(dimensions[0] == 0 || dimensions[1] == 0) {
        return Array<Array<Type> >(static_cast<s32>(dimensions[0]), static_cast<s32>(dimensions[1]), memory);
      }

      mxClassID matlabClassId;
      mxClassID templateClassId;

      const mxArray * firstElement = mxGetCell(matlabArray, 0);
      matlabClassId = mxGetClassID(firstElement);
      templateClassId = getMatlabClassID<Type>();

      if(matlabClassId != templateClassId) {
        AnkiError("mxCellArrayToArray", "mxCellArrayToArray<Type> - Matlab classId does not match with template %d!=%d\n", matlabClassId, templateClassId);
        return Array<Array<Type> >();
      }

      Array<Array<Type> > array(static_cast<s32>(dimensions[0]), static_cast<s32>(dimensions[1]), memory);

      if(!array.IsValid()) {
        AnkiError("mxCellArrayToArray", "mxCellArrayToArray<Type> - Could not allocate array\n");
        return Array<Array<Type> >();
      }

      // Allocate the memory for inner objects
      for(mwSize y=0; y<dimensions[0]; ++y) {
        Array<Type> * const pArray = array.Pointer(static_cast<s32>(y), 0);

        for(mwSize x=0; x<dimensions[1]; ++x) {
          mwIndex subs[2] = {y, x};

          const mwIndex cellIndex = mxCalcSingleSubscript(matlabArray, 2, &subs[0]);

          const mxArray * curMatlabArray = mxGetCell(matlabArray, cellIndex);

          pArray[x] = mxArrayToArray<Type>(curMatlabArray, memory);
        }
      }

      return array;
    }

    inline char* mxArrayToString(const mxArray * const matlabArray, MemoryStack &memory)
    {
      const mxClassID matlabClassId = mxGetClassID(matlabArray);

      AnkiConditionalErrorAndReturnValue(matlabClassId == mxCHAR_CLASS,
        NULL, "mxArrayToString", "Matlab classId is not char %d!=%d\n", matlabClassId, mxCHAR_CLASS);

      char * curMatlabString = mxArrayToString(matlabArray);

      AnkiConditionalErrorAndReturnValue(curMatlabString,
        NULL, "mxArrayToString", "Could not convert string");

      const s32 stringLength = static_cast<s32>(strlen(curMatlabString)) + 1;

      char * curCString = reinterpret_cast<char*>(memory.Allocate(stringLength));

      AnkiConditionalErrorAndReturnValue(curCString,
        NULL, "mxArrayToString", "Could not allocate string");

      strncpy(curCString, curMatlabString, stringLength);

      mxFree(curMatlabString);

      return curCString;
    }

    inline Array<char *> mxCellArrayToStringArray(const mxArray * const matlabArray, MemoryStack &memory)
    {
      const mwSize numDimensions = mxGetNumberOfDimensions(matlabArray);
      const mwSize *dimensions = mxGetDimensions(matlabArray);

      if(numDimensions != 2) {
        AnkiError("mxCellArrayToStringArray", "mxCellArrayToStringArray - Matlab array must be 2D\n");
        return Array<char *>();
      }

      if(!mxIsCell(matlabArray)) {
        AnkiError("mxCellArrayToStringArray", "Input must be a cell array");
        return Array<char *>();
      }

      if(dimensions[0] == 0 || dimensions[1] == 0) {
        return Array<char *>();
      }

      mxClassID matlabClassId;

      const mxArray * firstElement = mxGetCell(matlabArray, 0);
      matlabClassId = mxGetClassID(firstElement);

      if(matlabClassId != mxCHAR_CLASS) {
        AnkiError("mxCellArrayToStringArray", "mxCellArrayToStringArray - Matlab classId does not match with template %d!=%d\n", matlabClassId, mxCHAR_CLASS);
        return Array<char *>();
      }

      Array<char *> array(static_cast<s32>(dimensions[0]), static_cast<s32>(dimensions[1]), memory);

      if(!array.IsValid()) {
        AnkiError("mxCellArrayToStringArray", "mxCellArrayToStringArray - Could not allocate array\n");
        return Array<char *>();
      }

      // Allocate the memory for inner objects
      for(mwSize y=0; y<dimensions[0]; ++y) {
        char ** pArray = array.Pointer(static_cast<s32>(y), 0);

        for(mwSize x=0; x<dimensions[1]; ++x) {
          mwIndex subs[2] = {y, x};

          const mwIndex cellIndex = mxCalcSingleSubscript(matlabArray, 2, &subs[0]);

          const mxArray * curMatlabArray = mxGetCell(matlabArray, cellIndex);

          char * curCString = mxArrayToString(curMatlabArray, memory);

          pArray[x] = curCString;
        }
      }

      return array;
    }

    inline mxArray* stringArrayToMxCellArray(const Array<const char *> &array)
    {
      const mwSize outputDims[2] = {
        static_cast<mwSize>(array.get_size(0)),
        static_cast<mwSize>(array.get_size(1))};

      mxArray *outputArray = mxCreateCellArray(2, outputDims);

      for(mwSize y=0; y<outputDims[0]; ++y) {
        const char * const * const pArray = array.Pointer(static_cast<s32>(y), 0);

        for(mwSize x=0; x<outputDims[1]; ++x) {
          mwIndex subs[2] = {y, x};

          const mwIndex cellIndex = mxCalcSingleSubscript(outputArray, 2, &subs[0]);

          mxSetCell(outputArray, cellIndex, mxCreateString(pArray[x]));
        }
      }

      return outputArray;
    } // inline mxArray* stringArrayToMxCellArray(const Array<const char *> &array)

    template<typename Type> mxArray* arrayToMxArray(const Array<Type> &array)
    {
      const mxClassID classId = getMatlabClassID<Type>();

      const mwSize outputDims[2] = {
        static_cast<mwSize>(array.get_size(0)),
        static_cast<mwSize>(array.get_size(1))};

      mxArray *outputArray = mxCreateNumericArray(2, outputDims, classId, mxREAL);
      Type * const matlabMatrixStartPointer = (Type *) mxGetData(outputArray);

      for(mwSize y=0; y<outputDims[0]; ++y) {
        const Type * const pArray = array.Pointer(static_cast<s32>(y), 0);

        for(mwSize x=0; x<outputDims[1]; ++x) {
          *(matlabMatrixStartPointer + x*outputDims[0] + y) = pArray[x];
        }
      }

      return outputArray;
    } // template<typename Type> mxArray* arrayToMxArray(const Array<Type> &array)

    template<typename Type>
    mxArray * imageArrayToMxArray(const Type *img, const int nrows, const int ncols, const mwSize nbands)
    {
      // Seems to be faster to copy the data in single precision and
      // transposed and to do the permute and double cast at the end.

      const mwSize outputDims[3] = {static_cast<mwSize>(nrows),
        static_cast<mwSize>(ncols), static_cast<mwSize>(nbands)};

      mxArray *outputArray = mxCreateNumericArray(3, outputDims,
        getMatlabClassID<Type>(), mxREAL);

      if(outputArray == NULL) {
        // TODO: do something else about this or rely on caller to catch?
        return outputArray;
      }

      const int npixels = nrows*ncols;

      // Get a pointer for start of current row in each band:
      Type **outputRow = new Type*[nbands];
      outputRow[0] = (Type *) mxGetData(outputArray);
      for(mwSize band=1; band < nbands; band++) {
        outputRow[band] = outputRow[band-1] + npixels;
      }

      // Switch from row-major to col-major
      for(mwSize i=0; i<outputDims[0]; ++i) {
        for(mwSize j=0; j<outputDims[1]; ++j) {
          for(mwSize band=0; band<outputDims[2]; ++band) {
            *(outputRow[band] + j*nrows) = *img;
            img++; // Move ahead to next band at this pixel
          } // FOR each band
        } // FOR each column

        // Move all band pointers down one row
        for(mwSize band=0; band<outputDims[2]; ++band) {
          outputRow[band]++;
        }
      } // FOR each row

      delete[] outputRow;

      return outputArray;
    }
  } // namespace Embedded
} // namespace Anki

#endif /* #if ANKICORETECH_EMBEDDED_USE_MATLAB */

#endif /* _ANKICORETECHEMBEDDED_MATLAB_CONVERTERS_H_ */
