//
//  CozmoEngineSignals.cpp
//
//  Description: Signals that are emitted from within CozmoGame that other parts of CozmoGame may listen for.
//               Primarily used to trigger signals based on incoming U2G messages.
//
//  Created by Kevin Yoon 01/13/2015
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/signals/cozmoEngineSignals.h"


namespace Anki {
  namespace Cozmo {
    
#define SIGNAL_DEFINITION_MODE SIGNAL_CLASS_DEFINITION_MODE
#include "anki/cozmo/basestation/signals/cozmoEngineSignals.def"
    
  } // namespace Cozmo
} // namespace Anki
