#include "hal.h"

// Pairable LEDs are stored in odd/even pairs, but DIR lights up just one at a time
u8 code LED_DIR[] = {
  255 -LED3-LED5,   // 0: R4
  255 -LED1-LED4,   // 1: R2
  255 -LED2-LED5,   // 2: R3
  255 -LED0-LED4,   // 3: R1  
  
  255 -LED3-LED2,   // 4: G4
  255 -LED1-LED0,   // 5: G2
  255 -LED2-LED3,   // 6: G3
  255 -LED0-LED1,   // 7: G1

  255 -LED3-LED1,   // 8: B4
  255 -LED2-LED0,   // 9: B3
  255 -LED1-LED3,   // 10:B2
  255 -LED0-LED2,   // 11:B1
  
  255               // Dark byte, all off
};
// PORT (pin settings) light both LEDs in a pair - that way, you can "&" adjacent LED_DIRs to light LEDs in pairs
u8 code LED_PORT[] = {
  LED3 +LED1,   // R4
  LED3 +LED1,   // R2
  LED2 +LED0,   // R3
  LED2 +LED0,   // R1 
  
  LED3 +LED1,   // G4
  LED3 +LED1,   // G2
  LED2 +LED0,   // G3
  LED2 +LED0,   // G1

  LED3 +LED2,   // B4
  LED3 +LED2,   // B3
  LED1 +LED0,   // B2
  LED1 +LED0,   // B1
  
  0                       // Dark byte, all off 
};

static u8 _led;   // Downward counting pointer to the current LED

void LedTest(u8 num)
{
  if (!num)
    return;
  if (!_led)
    _led = num;
  _led--;
    
  LEDPORT = LED_PORT[_led + 4];   // XXX: Red doesn't work on charger or EP1
  LEDDIR = LED_DIR[_led + 4];
}
