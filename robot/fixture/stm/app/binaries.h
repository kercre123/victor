#ifndef __BINARIES_H
#define __BINARIES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
  // Cube binaries
  extern const uint8_t g_CubeTest[], g_CubeTestEnd[];
  extern const uint8_t g_CubeStub[], g_CubeStubEnd[];
  extern const uint8_t g_CubeBoot[], g_CubeBootEnd[]; //DEBUG -- this bin is nested in CubeStub for OTP programming
  extern const uint8_t g_CubeStubFcc[], g_CubeStubFccEnd[];
  
  // Body binaries
  extern const uint8_t g_BodyTest[], g_BodyTestEnd[];
  extern const uint8_t g_BodyBoot[], g_BodyBootEnd[];
  extern const uint8_t g_Body[], g_BodyEnd[];
  
  extern int g_canary;
  
  //print known info on all included binaries
  //@param csv - true prints in CSV-friendly format. False format for console view.
  void binPrintInfo(bool csv);
  
#ifdef __cplusplus
}
#endif

#endif
