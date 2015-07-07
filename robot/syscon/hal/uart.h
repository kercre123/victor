#ifndef UART_H
#define UART_H

#include <stdint.h>

namespace UART {
  extern bool initialized;
  
  // Set/clear transmit mode - you can't receive while in transmit mode
  void setTransmit(bool doTransmit);
  void configure();
  
  // Return true if we are receiving a break from the host
  int isBreak();

  void put(uint8_t c);
  void put(const char* s);

  void hex(uint32_t c);
  void hex(uint8_t c);

  void dec(int value);

  void dump(const uint8_t* data, uint32_t len);

  int get();
}
  
#endif
