#ifndef __WATCHDOG_H
#define __WATCHDOG_H

#include <stdint.h>

enum WatchdogChannels {
  WDOG_HAL_EXEC,
  WDOG_SPINE_COMMS,
  WDOG_WIFI_COMMS,
  WDOG_MAIN_EXEC,
  WDOG_TOTAL_CHANNELS
};

static const uint32_t MAXIMUM_RESET_COUNT = 5;

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      namespace Watchdog {
        void init(void);
        void kickAll(void);
        void kick(const uint8_t channel);
        void pet(void);
        void suspend(void);
      }
    }
  }
}

#endif
