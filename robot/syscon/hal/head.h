#ifndef HEAD_H
#define HEAD_H

namespace Head {
  // Whether we have received any data from the head yet
  extern bool spokenTo;
  extern bool synced;
                                                                                                                                                                                                                                                                
  // Initialize the SPI peripheral on the designated pins in the source file.
  void init();
  void manage();
  void enterLowPowerMode();
  void leaveLowPowerMode();
  void enableFixtureComms(bool enable); 
}

#endif
