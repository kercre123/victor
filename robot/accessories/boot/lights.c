// The LED controller 
#include "hal.h"

// IR LEDs are not included in this count, since they're lit by duck-hunt timing code
#define NUM_LEDS          12
#define CHARGER_LED       16  // First charger LED (EP3 chargers are wired differently)
#define GAMMA_CORRECT(x)  ((x)*(x) >> 8)

// This maps from Charlieplex pairing (native) order into payload order, to simplify sharing
// The payload indexes LEDs relative to the screw - R4G4B4 R1G1B1 R2G2B2 R3G3B3 (ask Vance/ME for more details)
u8 code LED_FROM_PAYLOAD[] = {    // Note: This is incorrectly typed to help the code generator
  &_radioPayload[1+0*3], &_radioPayload[1+2*3], &_radioPayload[1+3*3], &_radioPayload[1+1*3],   // Reds
  &_radioPayload[2+0*3], &_radioPayload[2+2*3], &_radioPayload[2+3*3], &_radioPayload[2+1*3],   // Greens
  &_radioPayload[3+0*3], &_radioPayload[3+3*3], &_radioPayload[3+2*3], &_radioPayload[3+1*3],   // Blues
  0, 0, 0, 0,
  // This duplicate copy supports chargers
  &_radioPayload[1+0*3], &_radioPayload[1+2*3], &_radioPayload[1+3*3], &_radioPayload[1+1*3],   // Reds
  &_radioPayload[2+0*3], &_radioPayload[2+2*3], &_radioPayload[2+3*3], &_radioPayload[2+1*3],   // Greens
  &_radioPayload[3+0*3], &_radioPayload[3+3*3], &_radioPayload[3+2*3], &_radioPayload[3+1*3]    // Blues
};

// Pairable LEDs are stored in odd/even pairs, but DIR lights up just one at a time
u8 code LED_DIR[] = {
  DEFDIR-LED3-LED5, DEFDIR-LED1-LED4, DEFDIR-LED2-LED5, DEFDIR-LED0-LED4, // R4 R2 R3 R1
  DEFDIR-LED3-LED2, DEFDIR-LED1-LED0, DEFDIR-LED2-LED3, DEFDIR-LED0-LED1, // G4 G2 G3 G1
  DEFDIR-LED3-LED1, DEFDIR-LED2-LED0, DEFDIR-LED1-LED3, DEFDIR-LED0-LED2, // B4 B3 B2 B1
  DEFDIR-LED5-LED3, DEFDIR-LED4-LED1, DEFDIR-LED5-LED2, DEFDIR-LED4-LED0, // I4 I2 I3 I1 (Dx-4)
  // Chargers are wired differently than cubes - here are the charger LEDs
  DEFDIR,           DEFDIR-LEC1-LEC4, DEFDIR-LEC2-LEC5, DEFDIR-LEC0-LEC4, // -- R2 R3 R1
  DEFDIR,           DEFDIR-LEC1-LEC0, DEFDIR-LEC2-LEC3, DEFDIR-LEC0-LEC1, // -- G2 G3 G1
  DEFDIR,           DEFDIR-LEC2-LEC0, DEFDIR-LEC1-LEC3, DEFDIR-LEC0-LEC2, // -- B3 B2 B1
  DEFDIR,           DEFDIR,           DEFDIR-LED5-LED2, DEFDIR-LED4-LED0, // -- -- I3 I1 (Dx-4)
};

// PORT (pin settings) light both anodes in a pair - that way, you can "&" adjacent LED_DIRs to light LEDs in pairs
u8 code LED_PORT[] = {
  LED3+LED1,  LED3+LED1,  LED2+LED0,  LED2+LED0,  // R4 R2 R3 R1
  LED3+LED1,  LED3+LED1,  LED2+LED0,  LED2+LED0,  // G4 G2 G3 G1
  LED3+LED2,  LED3+LED2,  LED1+LED0,  LED1+LED0,  // B4 B3 B2 B1
  LED4+LED5,  LED4+LED5,  LED4+LED5,  LED4+LED5,  // I4 I2 I3 I1 (Dx-4)
  // Chargers are wired differently than cubes - here are the charger LEDs
  0,          LEC1,       LEC2+LEC0,  LEC2+LEC0,  // -- R2 R3 R1
  0,          LEC1,       LEC2+LEC0,  LEC2+LEC0,  // -- G2 G3 G1
  0,          LEC2,       LEC1+LEC0,  LEC1+LEC0,  // -- B3 B2 B1
  0,          0,          LEC4+LEC5,  LEC4+LEC5,  // -- -- I3 I1 (Dx-4)
};

// Turn on a single LED (0-15)
void LightOn(u8 i)
{
  if (IsCharger())      // Chargers are wired differently
    i += CHARGER_LED;
  LEDPORT = LED_PORT[i];
  LEDDIR = LED_DIR[i];
}

// Precomputed list of paired LEDs and (linear) brightness to speed up ISR
typedef struct {
  u8 dir, port, value;
} LedValue;
static LedValue idata _leds[NUM_LEDS+1] _at_ 0x80;  // +1 for the terminator

// Set RGB LED values from _radioPayload[1..12]
void LedSetValues()
{
  LedValue idata *led = _leds;
  u8 i, v, dir, left;

  i = 0;
  if (IsCharger())      // Chargers are wired differently
    i = CHARGER_LED; 
  
  // Gamma-correct 8-bit payload values and look for pairs (to boost brightness)
  dir = 0;
  for (; !((i & 15) == NUM_LEDS); i++)
  {
    v = *(u8 idata *)(LED_FROM_PAYLOAD[i]);   // Deswizzle
    v = GAMMA_CORRECT(v);   
    if (!v)           // If LED is not lit, skip adding it
      goto skip;
    if (v > 0xfc)     // If LED is >= 0xfc (our max brightness), cap it
      v = 0xfc;
    
    led->value = v;
    led->port = LED_PORT[i];
    v = led->dir = LED_DIR[i];
    
    // If the last guy was lit and I'm his neighbor, let's share a time slot!
    if (dir && (i & 1))
    {
      dir &= v;             // Light both directions at once            
      left = led->value - (led-1)->value;   // How much do I have left after sharing?
      if (left & 0x80)      // If I have less than my neighbor...
      {
        led->dir = dir;         // I'll share the smaller part
        (led-1)->value = -left; // He gets what's left
      } else {
        (led-1)->dir = dir;     // He can share the smaller part
        led->value = left;      // And I'll take what's left
        if (!left)      // If I had nothing left, throw away my entry
          continue;
      }
    }
    led++;
    
  skip:
    dir = v;
  }

  // Set the terminating LED value to "off"
  led->value = 0;
  led->port = 0;
  led->dir = DEFDIR;
}

// The LED ISR also manages beat timing (since it drives the TICK counter)
extern u8 _beatTicks, _beatAdjust, _beatCount;

// The LED ISR needs to preserve these bytes
LedValue idata *_led _at_ 0x70;
u8 _nextBeat         _at_ 0x71;
u8 _dither           _at_ 0x72;
TICK_ISR() using 1
{
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
    // XXX: Could support dimmer LEDs by scheduling T1/T0 ISR to turn it off
    LEDDIR = DEFDIR;        // Turn LED back off
    howlong = 2;
  }
  
  // Stop at end-of-beat if this LED runs past it, OR it was the last (0) LED..
  if (howlong > _nextBeat || !(_led->value))
    howlong = _nextBeat;    // Just run to next beat
  RTC2CMP0 = howlong-1;
  
  // Did we just start a new beat?  Alert everyone
  if (_led == _leds)
    _beatCount++;
  _led++;
  
  // If there's no time for another tick, we finished the beat
  _nextBeat -= howlong;
  if (lessthanpow2(_nextBeat, 2))
  {
    _led = _leds;           // Reset to first LED
    _nextBeat += _beatAdjust + _beatTicks;  // Apply adjustment to next beat time
    _beatAdjust = 0;
    _dither ^= 2;           // Cycle 0,2,3,1 to reduce flicker
    if (_dither & 2)
      _dither ^= 1;
  }
}

// Start LED driving and beat counting ISR - reset the chip to exit
void LedInit(void)
{
  // Set all LEDs to high-drive
  P0CON = HIGH_DRIVE | 0;
  P0CON = HIGH_DRIVE | 1;
  P0CON = HIGH_DRIVE | 2;
  P0CON = HIGH_DRIVE | 3;
  P0CON = HIGH_DRIVE | 5;
  P0CON = HIGH_DRIVE | 6;
  
  // Get ISR in sane state before starting it
  LedSetValues();

  // Get LED IRQ up and running
  CLKCTRL = XOSC_ON;        // Leave XOSC on to to feed RC clk
  CLKLFCTRL = CLKLF_XOSC;   // Generate RC clk from XOSC for accuracy
  RTC2PCO = 1;              // Prestart in 1 cycle to improve LED timing (requires XOSC RC)
  WUCON = WAKE_ON_TICK;
  // Retention mode stops working if too low (about 50 with XOSC off, 2 with RC-only/XOSC on)
  RTC2CMP0 = _nextBeat = 2;
  RTC2CMP1 = 0;
  RTC2CON = RTC_COMPARE_RESET | RTC_ENABLE;
  
  // Enable TICK IRQ
  WUIRQ = 1;
  EA = 1;
}
