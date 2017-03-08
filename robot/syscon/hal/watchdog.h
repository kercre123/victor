#ifndef __WATCHDOG_H
#define __WATCHDOG_H

enum watchdog_channels {
  WDOG_ROOT_LOOP,
  WDOG_MAIN_LOOP,
  WDOG_UART,
  WDOG_TOTAL_CHANNELS
};

static const uint8_t wdog_channel_mask = (1 << WDOG_TOTAL_CHANNELS) - 1;

namespace Watchdog {
  void init(void);
  void kick(uint8_t channel);
};

#endif
