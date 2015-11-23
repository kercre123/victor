#ifndef __BINARIES_H
#define __BINARIES_H

#include <stdint.h>

extern "C" {
  // Cube firmware
  extern const uint8_t g_Cube[], g_CubeEnd[];

  // nRF51 firmware (boot and app)
  extern const uint8_t g_BodyBoot[], g_BodyBootEnd[];
  extern const uint8_t g_Body[], g_BodyEnd[];

  // K02 firmware (boot and app)
  extern const uint8_t g_K02Boot[], g_K02BootEnd[];
  extern const uint8_t g_K02[], g_K02End[];

  // Espressif firmware (boot and app - and some weird extra junk - ask Daniel)
  extern const uint8_t g_EspBlank[], g_EspBlankEnd[];
  extern const uint8_t g_EspUser[], g_EspUserEnd[];
  extern const uint8_t g_EspBoot[], g_EspBootEnd[];
  extern const uint8_t g_EspInit[], g_EspInitEnd[];
  
  // SWD shims - these are used to flash MCUs via SWD
  extern const uint8_t g_shimK02[], g_shimK02End[];
  extern const uint8_t g_shimBody[], g_shimBodyEnd[];  
}

#endif
