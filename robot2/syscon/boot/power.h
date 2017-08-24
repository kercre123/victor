#ifndef __POWER_H
#define __POWER_H

namespace Power {
  void init(void);
  void enableClocking(void);
  static void softReset(void) {}
}

#endif
