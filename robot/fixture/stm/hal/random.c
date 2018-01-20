#include <string.h>
#include "hal/board.h"
#include "hal/portable.h"
#include "hal/random.h"

void InitRandom()
{
  // Turn it on
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
  RNG->CR |= RNG_CR_RNGEN;
}

u32 GetRandom()
{
  // Get a 32-bit value once ready
  while (!(RNG->SR & RNG_SR_DRDY))
    ;
  return RNG->DR;
}
