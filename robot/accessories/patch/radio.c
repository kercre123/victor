// Listen for (receive) and transmit packets
#include "hal.h"
#include "nrf.h"

// 384uS is the maximum listen time (since we use LSB of T2 with MCLK/24)
#define LISTEN_TIME_US  384

// Temporary holding area for radio payloads
u8 data _radioIn[HAND_LEN] _at_ 0x50;    // Inbound payload
u8 data _radioOut[HAND_LEN];             // Outbound payload

code u8 SETUP_OFF[] = {
  WREG | CONFIG,        0,                      // Power down
  0
};

// Handshake configuration
code u8 SETUP_RX_HAND[] = {
  WREG | STATUS,        RX_DR | TX_DS | MAX_RT, // Clear IRQ status
  WREG | CONFIG,  PWR_UP|PRIM_RX | EN_CRC|CRCO, // Power up RX with 16-bit CRC
  WREG | FLUSH_RX,      0,                      // Flush anything in the FIFO
  WREG | FLUSH_TX,      0,                      // Flush anything in the FIFO
  WREG | RX_PW_P0,      HAND_LEN,               // Expect a 17 byte reply
  0
};
code u8 SETUP_TX_HAND[] = {
  WREG | STATUS,        RX_DR | TX_DS | MAX_RT, // Clear IRQ status
  WREG | CONFIG,        PWR_UP | EN_CRC | CRCO, // Power up TX with 16-bit CRC
  0
};

// Send a packet to radio FIFO - XXX: Move to bootloader
void RadioSend(u8 idata *buf, u8 len)
{
  // Send the address
  RFCSN = 0;
  SPIRDAT = W_TX_PAYLOAD_NOACK; // FIFO is 2 bytes deep, so first byte never waits

  // Send the payload
  do
  {
    while(!(SPIRSTAT & TXREADY))    // Wait for FIFO empty
      ;
    SPIRDAT = *buf;
    buf++;
  } while (--len);
  
  // Wait for TX complete, then clean up
  while(!(SPIRSTAT & TXEMPTY))
    ;   
  RFCSN = 1;
}

extern u8 _hopBlackout, _hopIndex, _hop, _beatAdjust;
// Complete a handshake with the robot - hop, broadcast, listen, timeout
bit RadioHandshake()
{  
  u8 chan, jitter;

  // Hop to next channel, skipping past WiFi blackout
  chan = _hopBlackout;    // XXX: Legacy no-hop support
  if (_hopIndex)
  {
    _hop += _hopIndex;
    if (_hop > 52)
      _hop -= 53;
    chan = 4 + _hop;
    if (chan >= _hopBlackout)
      chan += 22;
  }
  
  // Listen for packet or timeout
  RadioSetup(SETUP_RX_HAND);
  RadioHopTo(chan);
  jitter = TimerMSB();
  TimerStart(RF_START_US+(1+5+HAND_LEN+2)*8+LISTEN_TIME_US);
  RFCE = 1;
  RFF = 0;
  TF2 = 0;
  
  // While radio is turning around, queue the outbound packet
  RadioSend(_radioOut, HAND_LEN);

  //DebugPrint('f', &jitter, 1);  // Show frame-to-frame jitter in units of 384uS (only works when Sleep() has no PWRDWN)
  DebugPrint('h', &chan, 1);    // Show where we hopped to

  // Wait here
  while (!RFF)
    if (TF2)    // Timeout
    {
      RFCE = 0;
      RadioSetup(SETUP_OFF);
      RFCKEN = 0;
      return 0;
    }
  jitter = TimerLSB();  // No timeout, keep going
  RFCE = 0;

  // Read the new packet now to add 20uS delay - robot needs delay
  RadioRead(_radioIn, HAND_LEN);

  // Start TX, wait for TX complete
  for (chan = 0; chan < 5; chan++)    // XXX: Add more delay for robot
    RadioSetup(SETUP_TX_HAND);
  RFCE = 1;
  RFF = 0;

  // Adjust beat counter based on measured jitter
  DebugPrint('j', &jitter, 1);  // Show microsecond-jitter (units of 1.5uS)
  jitter >>= 5;                         // T2/20=ticks - We call it T2/32 to dampening it a bit
  jitter += -(8-LISTEN_TIME_US/2/48);   // Mid point becomes 0 adjustment
  _beatAdjust = jitter;                 // If we arrive late, lengthen beat to arrive early
  
  // Wait here
  while (!RFF)
    ;
  RFCE = 0;

  // Shut it all down
  RadioSetup(SETUP_OFF);
  RFCKEN = 0;
  
  return 1;
}

// Old non-hopping robots park on a single channel and sync on first packet
extern u8 _shakeWait;
void RadioLegacyStart()
{
  // Listen for a packet, or let the watchdog time us out
  RadioSetup(SETUP_RX_HAND);
  RadioHopTo(_hopBlackout);
  RFCE = 1;
  RFF = 0;
  while (!RFF)
    ;
  RFCE = 0;
  RadioSetup(SETUP_OFF);
  RFCKEN = 0;
  
  // From this moment (packet received), we need to rewind about 750uS or 25 ticks
  // Listen "center point" is 6 ticks, LedInit burns 3, radio receive is 11 (130+200), power up is ?? (150uS?)
  _beatAdjust = -26;
  _shakeWait = 8;     // Because legacy cubes space their handshakes 7 beats apart and we just ran one
}

// All the junk following this line is to workaround a one-liner bug in LedSetValues in the EP3 bootloader - oops!

// IR LEDs are not included in this count, since they're lit by duck-hunt timing code
#define NUM_LEDS          12
#define CHARGER_LED       16  // First charger LED (EP3 chargers are wired differently)
#define GAMMA_CORRECT(x)  ((x)*(x) >> 8)

// This maps from Charlieplex pairing (native) order into payload order, to simplify sharing
// The payload indexes LEDs relative to the screw - R4G4B4 R1G1B1 R2G2B2 R3G3B3 (ask Vance/ME for more details)
u8 code LED_FROM_PAYLOAD[] = {    // Note: This is incorrectly typed to help the code generator
  &_radioIn[1+0*3], &_radioIn[1+2*3], &_radioIn[1+3*3], &_radioIn[1+1*3],   // Reds
  &_radioIn[2+0*3], &_radioIn[2+2*3], &_radioIn[2+3*3], &_radioIn[2+1*3],   // Greens
  &_radioIn[3+0*3], &_radioIn[3+3*3], &_radioIn[3+2*3], &_radioIn[3+1*3],   // Blues
  0, 0, 0, 0,
  // This duplicate copy supports chargers
  &_radioIn[1+0*3], &_radioIn[1+2*3], &_radioIn[1+3*3], &_radioIn[1+1*3],   // Reds
  &_radioIn[2+0*3], &_radioIn[2+2*3], &_radioIn[3+3*3], &_radioIn[2+1*3],   // Greens - XXX: B3/G3 are swapped
  &_radioIn[3+0*3], &_radioIn[2+3*3], &_radioIn[3+2*3], &_radioIn[3+1*3]    // Blues
};

// Pairable LEDs are stored in odd/even pairs, but DIR lights up just one at a time
u8 code LED_DIR[] = {
  DEFDIR-LED3-LED5, DEFDIR-LED1-LED4, DEFDIR-LED2-LED5, DEFDIR-LED0-LED4, // R4 R2 R3 R1
  DEFDIR-LED3-LED2, DEFDIR-LED1-LED0, DEFDIR-LED2-LED3, DEFDIR-LED0-LED1, // G4 G2 G3 G1
  DEFDIR-LED3-LED1, DEFDIR-LED2-LED0, DEFDIR-LED1-LED3, DEFDIR-LED0-LED2, // B4 B3 B2 B1
  DEFDIR-LED5-LED3, DEFDIR-LED4-LED1, DEFDIR-LED5-LED2, DEFDIR-LED4-LED0, // I4 I2 I3 I1 (Dx-4)
  // Chargers are wired differently than cubes - here are the charger LEDs
  DEFDIR,           DEFDIR-LEC1-LEC4, DEFDIR-LEC2-LEC5, DEFDIR-LEC0-LEC4, // -- R2 R3 R1
  DEFDIR,           DEFDIR-LEC1-LEC0, DEFDIR-LEC2-LEC3, DEFDIR-LEC0-LEC1, // -- G2 B3 G1  - XXX: B3/G3 are swapped
  DEFDIR,           DEFDIR-LEC2-LEC0, DEFDIR-LEC1-LEC3, DEFDIR-LEC0-LEC2, // -- G3 B2 B1
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
  0,          LEC1,       LEC2+LEC0,  LEC2+LEC0,  // -- G2 B3 G1  - XXX: B3/G3 are swapped
  0,          LEC2,       LEC1+LEC0,  LEC1+LEC0,  // -- G3 B2 B1
  0,          0,          LEC4+LEC5,  LEC4+LEC5,  // -- -- I3 I1 (Dx-4)
};

// Precomputed list of paired LEDs and (linear) brightness to speed up ISR
typedef struct {
  u8 dir, port, value;
} LedValue;
static LedValue idata _leds[NUM_LEDS+1] _at_ 0x80;  // +1 for the terminator

// Set RGB LED values from _radioIn[1..12]
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
