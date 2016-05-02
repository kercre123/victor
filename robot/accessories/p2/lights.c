#include "hal.h"

// IR LEDs are not included in this count, since they're lit by different code
#define NUM_LEDS    12          // 12 RGB LEDs

// Pairable LEDs are stored in odd/even pairs, but DIR lights up just one at a time
// Note that DIR turns all P0 pins to inputs, which is safe on EP3+ but requires EP1 accel.c to fix SDA/SCL later
u8 code LED_DIR[] = {
  DEFDIR-LED3-LED5, DEFDIR-LED1-LED4, DEFDIR-LED2-LED5, DEFDIR-LED0-LED4, // R4 R2 R3 R1
  DEFDIR-LED3-LED2, DEFDIR-LED1-LED0, DEFDIR-LED2-LED3, DEFDIR-LED0-LED1, // G4 G2 G3 G1
  DEFDIR-LED3-LED1, DEFDIR-LED2-LED0, DEFDIR-LED1-LED3, DEFDIR-LED0-LED2, // B4 B3 B2 B1
  DEFDIR,
};
// PORT (pin settings) light both anodes in a pair - that way, you can "&" adjacent LED_DIRs to light LEDs in pairs
u8 code LED_PORT[] = {
  LED3+LED1,        LED3+LED1,        LED2+LED0,        LED2+LED0,        // R4 R2 R3 R1
  LED3+LED1,        LED3+LED1,        LED2+LED0,        LED2+LED0,        // G4 G2 G3 G1
  LED3+LED2,        LED3+LED2,        LED1+LED0,        LED1+LED0,        // B4 B3 B2 B1
  0,
};

// Turn on a single LED (0-11), or 12 for none
void LightOn(u8 i)
{
  // XXX: If I'm an EP3 charger, use EP3 charger wiring
  //if (*((u8 code*)0x3FF3) & 0x80)   
  //  i += 13;
  LEDDIR = LED_DIR[i];
  LEDPORT = LED_PORT[i];
}
