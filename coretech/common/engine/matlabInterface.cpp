#include "coretech/common/engine/matlabInterface.h"

#if defined(ANKICORETECH_USE_MATLAB) && ANKICORETECH_USE_MATLAB

namespace Anki {

  Matlab::Matlab(bool clearWorkspace)
  : SharedMatlabInterface(clearWorkspace)
  {
    
  }

  
} // namespace Anki

#else
//
// Define a dummy symbol to silence linker warnings such as
//   "blah.cpp.o has no symbols"
//
// This symbol is never actually used and may be discarded during final linkage.
//
int coretech_common_engine_matlabInterface = 0;

#endif // defined(ANKICORETECH_USE_MATLAB) && ANKICORETECH_USE_MATLAB
