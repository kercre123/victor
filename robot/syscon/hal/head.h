#ifndef SPI_H
#define SPI_H

#include "portable.h"
#include "anki/cozmo/robot/spineData.h"

namespace Head {
  // Whether we have received any data from the head yet
  extern bool spokenTo;

  // Initialize the SPI peripheral on the designated pins in the source file.
  void init();
  void manage(void* userdata);
}

#endif
