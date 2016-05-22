#include "hal.h"

// Pairable LEDs are stored in odd/even pairs, but DIR lights up just one at a time
u8 code LED_DIR[] = {
  DEFDIR-LED3-LED5, DEFDIR-LED1-LED4, DEFDIR-LED2-LED5, DEFDIR-LED0-LED4, // R4 R2 R3 R1
  DEFDIR-LED3-LED2, DEFDIR-LED1-LED0, DEFDIR-LED2-LED3, DEFDIR-LED0-LED1, // G4 G2 G3 G1
  DEFDIR-LED3-LED1, DEFDIR-LED2-LED0, DEFDIR-LED1-LED3, DEFDIR-LED0-LED2, // B4 B3 B2 B1
  DEFDIR-LED5-LED3, DEFDIR-LED4-LED1, DEFDIR-LED5-LED2, DEFDIR-LED4-LED0, // I4 I2 I3 I1 (Dx-4)
};

// PORT (pin settings) light both anodes in a pair - that way, you can "&" adjacent LED_DIRs to light LEDs in pairs
u8 code LED_PORT[] = {
  LED3+LED1,  LED3+LED1,  LED2+LED0,  LED2+LED0,  // R4 R2 R3 R1
  LED3+LED1,  LED3+LED1,  LED2+LED0,  LED2+LED0,  // G4 G2 G3 G1
  LED3+LED2,  LED3+LED2,  LED1+LED0,  LED1+LED0,  // B4 B3 B2 B1
  LED4+LED5,  LED4+LED5,  LED4+LED5,  LED4+LED5,  // I4 I2 I3 I1 (Dx-4)
};

// Turn on a single LED (0-15)
void LightOn(u8 i)
{
  // XXX: If I'm an EP3 charger, use EP3 charger wiring
  //if (IsCharger())    
  //  i += CHARGER_LED;
  LEDPORT = LED_PORT[i];
  LEDDIR = LED_DIR[i];
}

static s8 _led;   // Downward counting pointer to the current LED

code u8 lookup[] = {1, 2, 3, 5, 6, 7, 9, 10, 11};
void LedTest(u8 num)
{
  _led++;
  if (_led >= num)
    _led -= 3;
  if (_led < 0)
    _led = 0;
  
  LightOn(lookup[_led]);
}
