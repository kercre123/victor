#ifndef __POWER_H
#define __POWER_H

enum PowerMode {
  POWER_UNINIT = 0,
  POWER_ACTIVE,       // Encoders
  POWER_CALM,
  POWER_STOP,
  POWER_ERASE
};

namespace Power {
  void init(void);
  void tick(void);
  void wakeUp(void);
  void setMode(PowerMode);
  void disableHead();
  void enableHead();
}

#endif
