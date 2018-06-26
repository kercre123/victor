/**
 * File: powerModeManager.h
 *
 * Author: Kevin Yoon
 * Created: 5/31/2018
 *
 * Description: Manages transitions between syscon power modes
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef COZMO_POWER_MODE_MANAGER_H_
#define COZMO_POWER_MODE_MANAGER_H_

#include "coretech/common/shared/types.h"

namespace Anki {
  namespace Cozmo {
    namespace PowerModeManager {
      
      void EnableActiveMode(bool enable, bool calibOnEnable);
      void Update();
      
    } // namespace PowerModeManager
  } // namespace Cozmo
} // namespace Anki

#endif // COZMO_POWER_MODE_MANAGER_H_
