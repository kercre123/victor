#ifndef UART_H
#define UART_H

#include <stdint.h>

namespace UART {
  extern bool initialized;
  
  int get();
  void print( const char* fmt, ...);
}
  
#endif
