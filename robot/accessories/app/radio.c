// Listen for (receive) and transmit packets
#include "hal.h"
#include "nrf.h"

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

extern u8 _hopBlackout, _hopIndex, _hop, _jitterTicks;
// Complete a handshake with the robot - hop, broadcast, listen, timeout
void RadioHandshake()
{
  u8 chan;

  // Hop to next channel, skipping past WiFi blackout
  chan = _hopBlackout;    // For no-hop support
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
  RFCE = 1;

  // While radio is turning around, queue the outbound packet
  RadioSend(_radioOut, HAND_LEN);
  DebugPrint('h', &chan, 1);    // Show where we hopped to
  Sleep();    // Wait for radio or timeout

  // In case of no packet, sleep for one TX time - to keep timing balanced
  RTC2CMP0 = RXTX_TICKS-1;
  RTC2CMP1 = 0;                   // In case we got here from first-sync

  if (RFF)    // If packet received
  {
    // Sleep for _jitterTicks - this re-centers timing so next RX finishes at RXTX_TICKS+listenTicks
    // It also gives the poor robot a chance to turnaround its own radio
    RFCE = 0;
    RTC2CON = 0;
    RTC2CMP0 = _jitterTicks-2;      // Borrow one tick to pay back for TX later
    RTC2CON = RTC_COMPARE_RESET | RTC_ENABLE;
    RadioRead(_radioIn, HAND_LEN);  // Read the new packet before turnaround
    Sleep();      // Wait to timeout
    
    // Start TX, wait for TX complete
    RTC2CMP0 = RXTX_TICKS;          // We need an extra tick here because TX runs slow
    RadioSetup(SETUP_TX_HAND);
    RFCE = 1;
    PetWatchdog();// Only because we heard from the robot
    Sleep();      // Wait for radio done (timeout is scheduled later)
  }

  RFCE = 0;
  RadioSetup(SETUP_OFF);
  RFCKEN = 0;
  return;
}
