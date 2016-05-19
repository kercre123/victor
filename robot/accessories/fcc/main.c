/**
 * Super quick and dirty nRF31 FCC test firmware
 * This works by cycling power - each power interruption (<0.5s) switches tests
 *
 * Test modes are sort-of displayed on LEDs:
 * 1 = RX only
 * 2 = TX packets 2403
 * 3 = TX packets 2481
 * 4 = TX carrier 2403
 * 5 = TX carrier 2481
 */
#include "hal.h"

void delayms(u8 delay)
{
  do {
    u8 j = 16;
    do {
      u8 i = 248;
      while (--i);  // 4 cycles*248*16 is about 1ms (with overhead)
    } while (--j);
  } while(--delay);
}

u8 xdata _whichTest;
void main(void)
{
  delayms(100);
  
  _whichTest++;
  _whichTest &= 7;
  
  if (!_whichTest || _whichTest > 5)
    _whichTest = 1;
  
  CLKLFCTRL = 1;  // Turn on RC LF
  TransmitData();
}
