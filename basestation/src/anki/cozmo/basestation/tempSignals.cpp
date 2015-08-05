//
//  tempSignals.cpp
//
//  Created by Andrew Stein 8/4/2015
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/shared/cozmoTypes.h"
#include "tempSignals.h"

namespace Anki {
  namespace Cozmo {
    
#define SIGNAL_DEFINITION_MODE SIGNAL_CLASS_DEFINITION_MODE
#include "tempSignals.def"
    
  } // namespace Cozmo
} // namespace Anki
