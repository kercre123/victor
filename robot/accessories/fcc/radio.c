// Listen for (receive) and transmit packets
#include "hal.h"
#include "nrf.h"

// Temporary holding area for radio payloads (inbound and outbound)
u8 idata _radioPayload[32];   // Largest packet supported by chip is 32 bytes

// We don't return STATUS because we never need it - it's cheaper AND faster to watch RFF
void writeReg(u8 reg, u8 value)
{
  RFCSN = 0;

  SPIRDAT = W_REGISTER + reg;       // FIFO is 2 bytes deep, so first byte never waits
  SPIRDAT = value;
  while(!(SPIRSTAT & TXEMPTY))  ;   // Wait for TX complete
  
  RFCSN = 1;
}

void writeMultiReg(u8 reg, u8 idata *buf, u8 length)
{
  RFCSN = 0;
  
  SPIRDAT = reg;        // FIFO is 2 bytes deep, so first byte never waits
  
  do
  {
    while(!(SPIRSTAT & TXREADY))  ;
    SPIRDAT = *buf;
    buf++;
  } while (--length);

  while(!(SPIRSTAT & TXEMPTY))  ;   // Wait for TX complete
  
  RFCSN = 1;
}

// Test 1: Listen on 2481
// Test 2: Packets on 2481
// Test 3: Packets on 2403
// Test 4: Carrier on 2481
// Test 5: Carrier on 2403
void delayms(u8 delay);

extern u8 xdata _whichTest;
void TransmitData()
{
  u8 i;
  
  for (i = 31; i; --i)
    _radioPayload[i] = 0x55;

  // Enable the radio clock
  RFCKEN = 1;

  // Set datarate (1mbps - default) and power (0dbm)
  writeReg(RF_SETUP, RF_PWR1 | RF_PWR0 | PLL_LOCK);    // XXX: PLL Lock is for test purposes only
  if (_whichTest >= 4)
    writeReg(RF_SETUP, RF_PWR1 | /*RF_PWR0 |*/ PLL_LOCK);    // -6dB

  // Set channel
  if (_whichTest & 1)
    writeReg(RF_CH, 81);   // 2481
  else
    writeReg(RF_CH, 3);    // 2403
  
  // Turn off Enhanced Shockburst (on by default) - Cube protocol handles its own reliability
  writeReg(EN_AA, 0);
  writeReg(SETUP_RETR, 0);

  writeReg(SETUP_AW, 2);   // 4 bytes
  
  // Receive length is 32 bytes (on both pipes)
  writeReg(RX_PW_P0, 32);
  writeReg(RX_PW_P1, 32);

  // Clear radio IRQ flags and RRF
  writeReg(STATUS, RX_DR | TX_DS | MAX_RT);
  RFF = 0;  // The "8051 way" to acknowledge an untaken interrupt

  // XXX: Set address is not needed yet - default will do
  //hal_nrf_set_address(HAL_NRF_TX, radioStruct.ADDRESS_TX_PTR); // cube to body

  // Flush the last packet we received
  writeReg(FLUSH_RX, 0);
  
  // Write payload to radio TX FIFO
  writeMultiReg(W_TX_PAYLOAD_NOACK, _radioPayload, 10);

  if (_whichTest == 1)
  {
    // Power up in receive mode - takes 150uS (on top of the 130uS PLL time), so do this ASAP
    writeReg(CONFIG, PRIM_RX | PWR_UP | EN_CRC | CRCO);

    // Enable receiver
    RFCE = 1;

    // Now sit and listen for a packet
    //while (!RFF) ;
    while(1)
      LedTest(_whichTest);      
  }

  // Got it - switch to transmit mode - takes 150uS (on top of the 130uS PLL time), so do this ASAP
  // Doing this way apparently prevents a tpece2csn spec violation?  Not sure
  writeReg(CONFIG, PWR_UP | EN_CRC | CRCO);    

  // Toggle radio CE signal to switch from receive to continuous transmit
  RFCE = 0;
  RFCE = 1;

  // Broadcast continually
  /*
  if (_whichTest >= 4)
  {
    writeReg(RF_SETUP, RF_PWR1 | RF_PWR0 | PLL_LOCK | CONT_WAVE);    // XXX: PLL Lock is for test purposes onlyj
    while(1)
      LedTest(_whichTest);  
  }
  */
    
  writeMultiReg(W_TX_PAYLOAD_NOACK, _radioPayload, 17);

  // Send packets continually
  while (1)
  {
    LedTest(_whichTest);
    writeMultiReg(W_TX_PAYLOAD_NOACK, _radioPayload, 17);

    // Clear radio IRQ flags and RRF
    writeReg(STATUS, RX_DR | TX_DS | MAX_RT);
    RFF = 0;  // The "8051 way" to acknowledge an untaken interrupt

    // Wait for radio to signal transmit complete
    while (!RFF) ;
  }
    
  // Power everything down
  RFCE = 0; 
  writeReg(CONFIG, 0);
}

