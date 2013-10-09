#include "anki/embeddedCommon/matlabConverters.h"

#if ANKICORETECH_EMBEDDED_USE_MATLAB

namespace Anki {
  namespace Embedded {
    // Template specializations for returning the right Matlab class
    // for given types

    template<> mxClassID getMatlabClassID<f32>(void) { return mxSINGLE_CLASS; }
    template<> mxClassID getMatlabClassID<f64>(void) { return mxDOUBLE_CLASS; }

    template<> mxClassID getMatlabClassID<s16>(void) { return mxINT16_CLASS; }
    template<> mxClassID getMatlabClassID<u16>(void) { return mxUINT16_CLASS; }

    template<> mxClassID getMatlabClassID<s8>(void) { return mxINT8_CLASS; }
    template<> mxClassID getMatlabClassID<u8>(void) { return mxUINT8_CLASS; }

    template<> mxClassID getMatlabClassID<s32>(void) { return mxINT32_CLASS; }
    template<> mxClassID getMatlabClassID<u32>(void) { return mxUINT32_CLASS; }

    template<> mxClassID getMatlabClassID<s64>(void) { return mxINT64_CLASS; }
    template<> mxClassID getMatlabClassID<u64>(void) { return mxUINT64_CLASS; }
  } // namespace Embedded
} // namespace Anki

#endif // #if ANKICORETECH_EMBEDDED_USE_MATLAB