#ifndef SPI_H
#define SPI_H

#include "portable.h"

namespace Head {
  // Whether we have received any data from the head yet
  extern bool spokenTo;

  // Initialize the SPI peripheral on the designated pins in the source file.
  void init();
}

#endif
