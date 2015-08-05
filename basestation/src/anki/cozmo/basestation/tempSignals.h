//
//  tempSignals.h
//
//  Created by Andrew Stein 8/4/2015
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#ifndef __COZMO_ENGINE_SIGNALS__
#define __COZMO_ENGINE_SIGNALS__

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/common/basestation/utils/signals/simpleSignal.hpp"

#include "messageEngineToGame.h"

namespace Anki {
  namespace Cozmo {
    
    class CozmoEngineSignals
    {
    public:
      
      // 1. Initial include just defines the definition modes for use below
#include "tempSignals.def"
      
#undef SIGNAL_CLASS_NAME
#define SIGNAL_CLASS_NAME CozmoEngineSignals
      
      // 2. Define all the signal types
#define SIGNAL_DEFINITION_MODE SIGNAL_CLASS_DECLARATION_MODE
#include "tempSignals.def"
      
    };
    
  } // namespace Cozmo
} // namespace Anki

#endif // __COZMO_ENGINE_SIGNALS__