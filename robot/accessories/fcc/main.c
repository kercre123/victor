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

u8 xdata _whichTest;  // In xdata so we preserve our value between reboots
void main(void)
{
  // Set all LEDs to high-drive
  P0CON = HIGH_DRIVE | 0;
  P0CON = HIGH_DRIVE | 1;
  P0CON = HIGH_DRIVE | 2;
  P0CON = HIGH_DRIVE | 3;
  P0CON = HIGH_DRIVE | 5;
  P0CON = HIGH_DRIVE | 6;
  
  delayms(100);   // Debounce time - so we don't double-increment
  
  _whichTest++;
  _whichTest &= 7;
  
  CLKLFCTRL = 1;  // Turn on RC LF
  delayms(5);  
  TransmitData();
}
