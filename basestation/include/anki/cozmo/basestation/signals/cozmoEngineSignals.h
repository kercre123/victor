//
//  CozmoEngineSignals.h
//
//  Description: Signals that are emitted from within CozmoEngine that CozmoGame may listen for.
//
//  Created by Kevin Yoon 01/13/2015
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#ifndef __COZMO_ENGINE_SIGNALS__
#define __COZMO_ENGINE_SIGNALS__

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/common/basestation/utils/signals/simpleSignal.hpp"
#include "clad/types/actionTypes.h"

namespace Anki {
  namespace Cozmo {
      
    class CozmoEngineSignals {
    
    public:
      
      // 1. Initial include just defines the definition modes for use below
      #include "anki/cozmo/basestation/signals/cozmoEngineSignals.def"
    
      #undef SIGNAL_CLASS_NAME
      #define SIGNAL_CLASS_NAME CozmoEngineSignals
      
      // 2. Define all the signal types
      #define SIGNAL_DEFINITION_MODE SIGNAL_CLASS_DECLARATION_MODE
      #include "anki/cozmo/basestation/signals/cozmoEngineSignals.def"
      
    };
    
    
  } // namespace Cozmo
} // namespace Anki

#endif // __COZMO_ENGINE_SIGNALS__
