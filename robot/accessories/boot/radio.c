// Listen for (receive) and transmit packets
#include "hal.h"
#include "nrf.h"

// Temporary holding area for radio payloads (inbound and outbound)
u8 data _radioPayload[32] _at_ 0x50;    // Largest packet supported by chip is 32 bytes

// Read bytes from the radio FIFO
void RadioRead(u8 idata *buf, u8 len)
{
  u8 dummy;
  
  // Start the read, and fully drain the FIFO
  RFCSN = 0;
  SPIRDAT = R_RX_PAYLOAD;
  while(!(SPIRSTAT & TXEMPTY))
    ;
  dummy = SPIRDAT;
  dummy = SPIRDAT;
  
  SPIRDAT = 0;    // Keep pumping TX to keep RX coming
  do {
    SPIRDAT = 0;
    while(!(SPIRSTAT & RXREADY))
      ;
    *buf = SPIRDAT;
    buf++;
  } while (--len);
  
  RFCSN = 1;
}

// Send a packet to radio FIFO
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

// Hop to the selected channel
void RadioHopTo(u8 channel)
{
  RFCSN = 0;

  SPIRDAT = WREG | RF_CH;
  SPIRDAT = channel;
  while(!(SPIRSTAT & TXEMPTY))
    ;
  
  RFCSN = 1;
}

// Configure the radio with a zero-terminated list of radio registers and values
void RadioSetup(u8 code *conf)
{
  u8 len;
  RFCKEN = 1;   // Enable radio clock
  
  do {
    // Check for optional payload length prefix
    len = 1;
    if (lessthanpow2(*conf, WREG))
    {
      len = *conf;
      conf++;
    }
    
    // Send the register number
    RFCSN = 0;
    SPIRDAT = *conf;        // FIFO is 2 bytes deep, so first byte never waits
    conf++;
    
    // Send the payload
    do
    {
      while(!(SPIRSTAT & TXREADY))    // Wait for FIFO empty
        ;
      SPIRDAT = *conf;
      conf++;
    } while (--len);

    // Wait for TX complete, then clean up
    while(!(SPIRSTAT & TXEMPTY))
      ;   
    RFCSN = 1;
  } while (*conf);    // Terminate at trailing 0
}

// Beacon (advertise/connect) configuration
extern code u8 ADVERTISEMENT[]; // Has to be in a separate file because LX51 sucks

code u8 SETUP_OFF[] = {
  WREG | CONFIG,        0,                      // Power down
  0
};

code u8 SETUP_TX_BEACON[] = {
  WREG | STATUS,        RX_DR | TX_DS | MAX_RT, // Clear IRQ status
  WREG | FLUSH_TX,      0,                      // Flush anything in the FIFO
  WREG | FLUSH_RX,      0,                      // Flush anything in the FIFO
  WREG | RF_SETUP,      RF_PWR1,                // 1mbps, -6dB (due to crap antenna)
  WREG | RF_CH,         ADV_CHANNEL,            // Hop to advertising frequency
  WREG | EN_AA,         0,                      // Disable Enhanced Shockburst
  WREG | SETUP_RETR,    0,                      // Disable Enhanced Shockburst
  WREG | SETUP_AW,      2,                      // Address width = 4
  WREG | RX_PW_P0,      10,                     // Expect a 10 byte reply

  4, WREG|TX_ADDR,      0xca,0x5c,0xad,0xed,    // Broadcast TX address - last byte "ed" follows nRF rules
  4, WREG|RX_ADDR_P0,   0xca,0x11,0xab,0x1e,    // Private RX address - last byte first on air
  0
};
code u8 SETUP_RX[] = {
  WREG | CONFIG,  PWR_UP|PRIM_RX | EN_CRC|CRCO, // Power up RX with 16-bit CRC
  WREG | STATUS,        RX_DR | TX_DS | MAX_RT, // Clear IRQ status
  0
};
// Send a beacon to the robot and wait for a reply or timeout
bit RadioBeacon()
{
  u8 gotPacket;
  
  // Start TX
  RadioSetup(ADVERTISEMENT);
  
  // Wait for TX complete
  IRCON = 0;
  RFCE = 1;
  PWRDWN = STANDBY;   // Note: Supposed to set PWRDWN=0 afterward (but so far not needed?)
  RFCE = 0;

  // Start RX
  RadioSetup(SETUP_RX);
  
  // Listen for packet or timeout (either will wake us)
  IRCON = 1;          // Note: Weird hack to clear interrupt flags without changing code size
  RFCE = 1;
  PWRDWN = STANDBY;
  RFCE = 0;
  
  // If we got a packet, read it  
  gotPacket = RFF;
  if (gotPacket)
    RadioRead((u8 idata *)SyncPkt, SyncLen);
  
  // Power down radio again - this makes RFF 'forget' its state
  RadioSetup(SETUP_OFF);
  RFCKEN = 0;
  
  return gotPacket;
}

// Handshake configuration
code u8 SETUP_RX_HAND[] = {
  WREG | STATUS,        RX_DR | TX_DS | MAX_RT, // Clear IRQ status
  WREG | CONFIG,  PWR_UP|PRIM_RX | EN_CRC|CRCO, // Power up RX with 16-bit CRC
  WREG | FLUSH_RX,      0,                      // Flush anything in the FIFO
  WREG | RX_PW_P0,      HAND_LEN,               // Expect a 17 byte reply
  0
};
code u8 SETUP_TX_HAND[] = {
  WREG | STATUS,        RX_DR | TX_DS | MAX_RT, // Clear IRQ status
  WREG | CONFIG,        PWR_UP | EN_CRC | CRCO, // Power up TX with 16-bit CRC
  WREG | FLUSH_TX,      0,                      // Flush anything in the FIFO
  HAND_LEN,W_TX_PAYLOAD_NOACK,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  4, WREG|TX_ADDR,      0xca,0x11,0xab,0x1e,    // Private TX address - last byte first on air
  0
};

extern u8 _hopBlackout;
// Complete a handshake with the robot - hop, broadcast, listen, timeout
// This function is only for OTA - hopping is not supported since we ran out of testing time
bit RadioHandshake()
{
  // Listen for packet or timeout
  RadioSetup(SETUP_RX_HAND);
  RadioHopTo(_hopBlackout);
  
  RFCE = 1;
  RFF = 0;
  while (!RFF)    // Wait until contact, or watchdog kills us due to broken connection
    ;
  RFCE = 0;
  
  // Start TX, wait for TX complete
  RadioSetup(SETUP_TX_HAND);  
  RFCE = 1;
  RFF = 0;
  while (!RFF)
    ;
  RFCE = 0;

  RadioRead(_radioPayload, 17);
  
  // Shut it all down
  RadioSetup(SETUP_OFF);
  RFCKEN = 0;
  
  return 1;
}
