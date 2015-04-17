//
//  CozmoGameSignals.cpp
//
//  Created by Kevin Yoon 01/13/2015
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/game/signals/CozmoGameSignals.h"

namespace Anki {
  namespace Cozmo {

    #define SIGNAL_DEFINITION_MODE SIGNAL_CLASS_DEFINITION_MODE
    #include "anki/cozmo/game/signals/cozmoGameSignals.def"
    
  } // namespace Cozmo
} // namespace Anki
