#ifndef __POWER_H
#define __POWER_H

namespace Power {
  void init(void);
  void enableClocking(void);
  void softReset(bool);
  void stop(void);
}

#endif
