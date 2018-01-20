/**
* File: matlabConverters.h
*
* Author: Andrew Stein (andrew)
* Created: 10/9/2013
*
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

#ifndef _ANKI_CORETECH_SHARED_MATLAB_CONVERTERS_H_
#define _ANKI_CORETECH_SHARED_MATLAB_CONVERTERS_H_

#if ANKICORETECH_EMBEDDED_USE_MATLAB || ANKICORETECH_USE_MATLAB

#include "coretech/common/shared/types.h"

#include <typeinfo>

#include "mex.h"

namespace Anki {

  // Convert a raw image data pointer with size to a mxAarray.  Allocate and return the mxArray
  template<typename Type> mxArray* imageArrayToMxArray(const Type *data, const s32 nrows, const s32 ncols, const mwSize nbands);

  // Convert a primitive type to a C++ to a Matlab class ID
  // (If we have not defined a specialization for a particular type below,
  //  then mxUNKNOWN_CLASS gets returned by this parent template.)
  template<typename Type> mxClassID GetMatlabClassID(void) { return mxUNKNOWN_CLASS; }

  // Forward declare the defined specializations for known types:
  template<> mxClassID GetMatlabClassID<f32>(void);
  template<> mxClassID GetMatlabClassID<f64>(void);

  template<> mxClassID GetMatlabClassID<s16>(void);
  template<> mxClassID GetMatlabClassID<u16>(void);

  template<> mxClassID GetMatlabClassID<s8>(void);
  template<> mxClassID GetMatlabClassID<u8>(void);

  template<> mxClassID GetMatlabClassID<s32>(void);
  template<> mxClassID GetMatlabClassID<u32>(void);

  template<> mxClassID GetMatlabClassID<s64>(void);
  template<> mxClassID GetMatlabClassID<u64>(void);

  
  //
  // Templated Implementations
  //
  
  // TODO: Move to _impl file?

  template<typename Type>
  mxArray * imageArrayToMxArray(const Type *img, const int nrows, const int ncols, const mwSize nbands)
  {
    // Seems to be faster to copy the data in single precision and
    // transposed and to do the permute and double cast at the end.

    const mwSize outputDims[3] = {static_cast<mwSize>(nrows),
      static_cast<mwSize>(ncols), static_cast<mwSize>(nbands)};

    mxArray *outputArray = mxCreateNumericArray(3, outputDims,
      GetMatlabClassID<Type>(), mxREAL);

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
    
  } // imageArrayToMxArray()

} // namespace Anki

#endif // #if ANKICORETECH_EMBEDDED_USE_MATLAB || ANKICORETECH_USE_MATLAB

#endif /* _ANKI_CORETECH_SHARED_MATLAB_CONVERTERS_H_ */
