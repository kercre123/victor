/**
File: matlabConverters.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/shared/matlabConverters.h"

#if ANKICORETECH_EMBEDDED_USE_MATLAB || ANKICORETECH_USE_MATLAB

namespace Anki {

  // Template specializations for returning the right Matlab class
    // for given types

    template<> mxClassID GetMatlabClassID<f32>(void) { return mxSINGLE_CLASS; }
    template<> mxClassID GetMatlabClassID<f64>(void) { return mxDOUBLE_CLASS; }

    template<> mxClassID GetMatlabClassID<s16>(void) { return mxINT16_CLASS; }
    template<> mxClassID GetMatlabClassID<u16>(void) { return mxUINT16_CLASS; }

    template<> mxClassID GetMatlabClassID<s8>(void) { return mxINT8_CLASS; }
    template<> mxClassID GetMatlabClassID<u8>(void) { return mxUINT8_CLASS; }

    template<> mxClassID GetMatlabClassID<s32>(void) { return mxINT32_CLASS; }
    template<> mxClassID GetMatlabClassID<u32>(void) { return mxUINT32_CLASS; }

    template<> mxClassID GetMatlabClassID<s64>(void) { return mxINT64_CLASS; }
    template<> mxClassID GetMatlabClassID<u64>(void) { return mxUINT64_CLASS; }

} // namespace Anki

#else

//
// Define a dummy symbol to silence linker warnings such as
//   "blah.cpp.o has no symbols"
//
// This symbol is never actually used and may be discarded during final linkage.
//
int coretech_common_shared_matlabConverters = 0;

#endif // #if ANKICORETECH_EMBEDDED_USE_MATLAB || ANKICORETECH_USE_MATLAB
