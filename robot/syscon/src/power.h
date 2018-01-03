#ifndef __POWER_H
#define __POWER_H

namespace Power {
  void init(void);
  void stop(void);
  void eject(void);
  void softReset(bool erase = false);
}

#endif
