/**
 * File: matlabConverters_basestation.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 11/24/2014
 *
 *
 * Description:
 *
 *   This file defines several conversion routines for translating
 *   basic basestation-specific types back and forth from Matlab's mxArray type.
 *   It should not call any Matlab routines (e.g. via an engine or mex), nor should
 *   it rely on STL (i.e. std::<foo>), so that it can be used extremely
 *   portably.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKI_CORETECH_ENGINE_MATLAB_CONVERTERS_H_
#define _ANKI_CORETECH_ENGINE_MATLAB_CONVERTERS_H_

#if ANKICORETECH_USE_MATLAB

#include "coretech/common/shared/matlabConverters.h"


namespace Anki {

  // TODO: Add conveters here
  
} // namespace Anki


#endif // ANKICORETECH_USE_MATLAB

#endif // _ANKI_CORETECH_ENGINE_MATLAB_CONVERTERS_H_
