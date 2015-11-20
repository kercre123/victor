#ifndef __BINARIES_H
#define __BINARIES_H

#include <stdint.h>

extern "C" {
  extern const uint8_t g_Cube[];
  extern const uint8_t g_CubeEnd[];

  extern const uint8_t g_Body[];
  extern const uint8_t g_BodyEnd[];
  extern const uint8_t g_BodyBoot[];
  extern const uint8_t g_BodyBootEnd[];

  extern const uint8_t g_K02[];
  extern const uint8_t g_K02End[];
  extern const uint8_t g_K02Boot[];
  extern const uint8_t g_K02BootEnd[];
  
  extern const uint8_t g_EspBlank[];
  extern const uint8_t g_EspBlankEnd[];
  extern const uint8_t g_EspUser[];
  extern const uint8_t g_EspUserEnd[];
  extern const uint8_t g_EspBoot[];
  extern const uint8_t g_EspBootEnd[];
  extern const uint8_t g_EspInit[];
  extern const uint8_t g_EspInitEnd[];
}

#endif
