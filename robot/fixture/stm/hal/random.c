#include <string.h>
#include "hal/board.h"
#include "hal/portable.h"
#include "hal/random.h"

void InitRandom()
{
  static bool inited = 0;
  if( !inited ) {
    // Turn it on
    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
    RNG->CR |= RNG_CR_RNGEN;
    inited = 1;
  }
}

uint32_t GetRandom()
{
  // Get a 32-bit value once ready
  while (!(RNG->SR & RNG_SR_DRDY))
    ;
  return RNG->DR;
}
