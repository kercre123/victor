#include "coretech/common/engine/matlabInterface.h"

#if defined(ANKICORETECH_USE_MATLAB) && ANKICORETECH_USE_MATLAB

namespace Anki {

  Matlab::Matlab(bool clearWorkspace)
  : SharedMatlabInterface(clearWorkspace)
  {
    
  }

  
} // namespace Anki

#endif // defined(ANKICORETECH_USE_MATLAB) && ANKICORETECH_USE_MATLAB
