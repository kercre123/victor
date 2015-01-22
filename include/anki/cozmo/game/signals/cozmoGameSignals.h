//
//  CozmoGameSignals.h
//
//  Created by Kevin Yoon 01/13/2015
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#ifndef __COZMO_GAME_SIGNALS__
#define __COZMO_GAME_SIGNALS__

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/common/basestation/utils/signals/simpleSignal.hpp"


namespace Anki {
  namespace Cozmo {

    class CozmoGameSignals {
    public:

      // 1. Initial include just defines the definition modes for use below
      #include "anki/cozmo/game/signals/cozmoGameSignals.def"
      
      #define SIGNAL_CLASS_NAME CozmoGameSignals
      
      // 2. Define all the signal types
      #define SIGNAL_DEFINITION_MODE SIGNAL_CLASS_DECLARATION_MODE
      #include "anki/cozmo/game/signals/cozmoGameSignals.def"
      
    };
    
  } // namespace Cozmo
} // namespace Anki

#endif // __COZMO_GAME_SIGNALS__