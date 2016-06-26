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
  
  // From this moment (packet received), we need to rewind about 42 ticks (measured)
  // Listen "center point" is 6 ticks, LedInit burns 3, radio receive is 11 (130+200), power up is ?? (150uS?)
  _beatAdjust = -42;
  _shakeWait = 7;     // Because legacy cubes space their handshakes 7 beats apart and we just ran one
}
