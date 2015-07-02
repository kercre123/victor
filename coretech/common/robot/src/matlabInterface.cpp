#include "anki/common/robot/matlabInterface.h"

#if (defined(ANKICORETECH_EMBEDDED_USE_MATLAB) && ANKICORETECH_EMBEDDED_USE_MATLAB)

//#ifdef ANKI_MEX_BUILD
//#include "mex.h"
//#endif

namespace Anki {
namespace Embedded {
  
  // Buffers for mex messages:
  char tmpMexBuffer1[1024];
  char tmpMexBuffer2[1024];
  
  
  Matlab::Matlab(bool clearWorkspace)
  : SharedMatlabInterface(clearWorkspace)
  {
    
  }

  
  Result Matlab::PutPoints(const Point<s16> * values, s32 nValues, const std::string name)
  {
    AnkiConditionalErrorAndReturnValue(this->ep, RESULT_FAIL, "Anki.Put", "Matlab engine is not started/connected");
    
    const mwSize dims[2] = {2, static_cast<mwSize>(nValues)};
    const mxClassID matlabType = GetMatlabClassID<s16>();
    mxArray *arrayTmp = mxCreateNumericArray(2, &dims[0], matlabType, mxREAL);
    s16 *matlabBufferTmp = (s16*) mxGetPr(arrayTmp);
    for(s32 i = 0, ci=0; i<nValues; i++) {
      matlabBufferTmp[ci++] = values[i].x;
      matlabBufferTmp[ci++] = values[i].y;
    }
    engPutVariable(ep, name.data(), arrayTmp);
    mxDestroyArray(arrayTmp);
    
    return RESULT_OK;
  }

} // namespace Embedded
} // namespace Anki

#endif // #if (defined(ANKICORETECH_EMBEDDED_USE_MATLAB) && ANKICORETECH_EMBEDDED_USE_MATLAB) 