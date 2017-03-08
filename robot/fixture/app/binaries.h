#ifndef __BINARIES_H
#define __BINARIES_H

#include <stdint.h>

extern "C" {
  // Cube firmware
  extern const uint8_t g_Cube[], g_CubeEnd[];
  extern const uint8_t g_CubeFCC[], g_CubeFCCEnd[];

  // nRF51 firmware (boot and app)
  extern const uint8_t g_BodyBLE[], g_BodyBLEEnd[];
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
  extern const uint8_t g_EspSafe[], g_EspSafeEnd[];
  
  // SWD stubs - these are used to flash MCUs via SWD
  extern const uint8_t g_stubK02[], g_stubK02End[];
  extern const uint8_t g_stubBody[], g_stubBodyEnd[];  
  
  // Firmware for local (nRF51) radio
  extern const uint8_t g_Radio[], g_RadioEnd[];
  
  
  //print known info on all included binaries
  //@param csv - true prints in CSV-friendly format. False format for console view.
  void binPrintInfo(bool csv);
}

#endif
