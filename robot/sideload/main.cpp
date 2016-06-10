#include "MK02F12810.h"

#include "portable.h"
#include "hardware.h"

#include "sideload.h"

static inline uint16_t WaitForWord(void) {
  while(~SPI0_SR & SPI_SR_RFDF_MASK) ;  // Wait for a byte
  uint16_t ret = SPI0_POPR;
 
  SPI0_SR = SPI0_SR;
  return ret;
}

extern "C" bool verify_cert(void) {
  return false;
}
