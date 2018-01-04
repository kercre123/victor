/**
 File: matlabInterface.h
 Author: Andrew Stein
 Created: 11/21/2014
 
 Extends shared Matlab interface to add basestation-specific converters.
 
 Copyright Anki, Inc. 2014
 For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
 **/
#ifndef ANKI_CORETECH_COMMON_BASESTATION_MATLAB_INTERFACE_H_
#define ANKI_CORETECH_COMMON_BASESTATION_MATLAB_INTERFACE_H_

#if ANKICORETECH_USE_MATLAB

#include "coretech/common/shared/sharedMatlabInterface.h"
#include "coretech/common/engine/matlabConverters_basestation.h"

namespace Anki {
  
  class Matlab : public SharedMatlabInterface
  {
  public:
    Matlab(bool clearWorkspace=false);
    
  }; // class Matlab
  
} // namespace Anki


#endif // ANKICORETECH_USE_MATLAB

#endif // ANKI_CORETECH_COMMON_BASESTATION_MATLAB_INTERFACE_H_

