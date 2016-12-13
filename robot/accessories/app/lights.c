#include "hal.h"

// Sleep until the next IRQ
void Sleep()
{
  IRCON = 0;
  PWRDWN = STANDBY;
  PWRDWN = 0;
}
// Deep sleep until RTC time is up
// This works only with RTC and can't work with 
void DeepSleep()
{
  IRCON = 0;
  PWRDWN = RETENTION;
  Sleep();    // Until RTC fires
}

typedef struct {
  u8 dir, port, value;
} LedValue;
LedValue idata _leds[12+1] _at_ LED_START;

LedValue idata *_led;
u8 _dither;
extern u8 _beatTicks, _nextBeat;

// Light up the LEDs for the remainder of the beat
void LightLEDs()
{
  do {
    u8 howlong;
    
    // Set the LEDs first (using a weird trick to change PORT/DIR close together)
    u16 led = *(u16 idata *)_led;
    LEDDIR = DEFDIR;              // Avoid glitchy multiplexing
    LEDPORT = led;
    LEDDIR = led >> 8;
    
    // How long before the next interrupt?  Dither 6.2 value into tick count
    howlong = _led->value;
    if (lessthanpow2(howlong, 8)) // Because min RTC = 2, values under 1.75 must be boosted
      howlong += _dither;
    howlong += _dither;
    howlong >>= 2;

    // Even if the LED was turned off this cycle, use the minimum RTC delay (2)
    if (lessthanpow2(howlong, 2))
    {
      LEDDIR = DEFDIR;        // Turn LED back off
      howlong = 2;
    }
    
    // Stop at end-of-beat if this LED runs past it, OR it was the last (0) LED..
    if (howlong > _nextBeat || !(_led->value))
      howlong = _nextBeat;    // Just run to next beat
    RTC2CMP0 = howlong-1;
    
    // Sleep while LED is lit, or deep sleep if howlong >= 3
    if (RTC2CMP0 & ~1)
      DeepSleep();
    else
      Sleep();
    
    // If there's no time for another tick, we finished the beat
    _led++;
    _nextBeat -= howlong;
  } while (!lessthanpow2(_nextBeat, 2));

  _led = _leds;           // Reset to first LED
  _nextBeat += _beatTicks;// Workout next beat time
  _dither ^= 2;           // Cycle 0,2,3,1 to reduce flicker
  if (_dither & 2)
    _dither ^= 1;
}
