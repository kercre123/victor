#ifndef __BINARIES_H
#define __BINARIES_H

#include <stdint.h>

extern "C" {
  extern const uint8_t* g_EspBlank;
  extern const uint8_t* g_EspBlankEnd;
  //extern const uint8_t* g_EspUser;
  //extern const uint8_t* g_EspUserEnd;
  extern const uint8_t* g_EspBoot;
  extern const uint8_t* g_EspBootEnd;
  extern const uint8_t* g_EspInit;
  extern const uint8_t* g_EspInitEnd;
}

#endif
