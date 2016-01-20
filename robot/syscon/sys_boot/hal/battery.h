#ifndef BATTERY_H
#define BATTERY_H

#include <stddef.h>

namespace Battery {
  // Initialize the charge pins and sensing
  void init();

  // Update the state of the battery
  void manage();
}

#endif
