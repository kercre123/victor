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

void delayms(u8 delay);

// Test mode configuration: CONFIG (RX/TX), RF_SETUP (power/data rate), CHAN, unused
u8 code TESTS[] = {
  PRIM_RX|PWR_UP, RF_PWR0, 2,  0,
  
  PWR_UP,         RF_PWR0, 2,  0,
  PWR_UP,         RF_PWR0, 42, 0,
  PWR_UP,         RF_PWR0, 81, 0,
  
  PRIM_RX|PWR_UP, RF_PWR0, 81, 0,
  
  PWR_UP,         RF_PWR0|PLL_LOCK|CONT_WAVE, 2,  0,
  PWR_UP,         RF_PWR0|PLL_LOCK|CONT_WAVE, 42, 0,
  PWR_UP,         RF_PWR0|PLL_LOCK|CONT_WAVE, 81, 0,
};

extern u8 xdata _whichTest;
void TransmitData()
{
  u8 i;
  
  for (i = 31; i; --i)
    _radioPayload[i] = 0x55;

  // Enable the radio clock
  RFCKEN = 1;
  
  i = (_whichTest & 7) << 2;        // Offset into TESTS
  
  writeReg(RF_SETUP, TESTS[i+1]);   // Must come first, since it enables power
  writeReg(RF_CH, TESTS[i+2]);
  
  // Turn off Enhanced Shockburst (on by default) - Cube protocol handles its own reliability
  writeReg(EN_AA, 0);
  writeReg(SETUP_RETR, 0);

  writeReg(SETUP_AW, 2);   // 4 bytes
  
  // Power up in receive or transmit mode
  writeReg(CONFIG, TESTS[i+0]);
  delayms(5);

  // Tests 1-3, send packets forever
  if (_whichTest && !(_whichTest & 4))
  {
    // Continuous: writeMultiReg(W_TX_PAYLOAD_NOACK, _radioPayload, 17);   // Buffer one extra
    while (1)
    {
      delayms(5);
      i++;
      LedTest(_whichTest);
      if (i == 7)
      {
        // Our craziest case is 3 packets per 35ms on same channel - or 9 packets hopping!
        i = 0;
        writeMultiReg(W_TX_PAYLOAD_NOACK, _radioPayload, 32);
        writeMultiReg(W_TX_PAYLOAD_NOACK, _radioPayload, 32);
        writeMultiReg(W_TX_PAYLOAD_NOACK, _radioPayload, 32);

        // Clear radio IRQ flags and RRF
        writeReg(STATUS, RX_DR | TX_DS | MAX_RT);
        
        RFF = 0;  // The "8051 way" to acknowledge an untaken interrupt
        RFCE = 1; // Kick off radio and come back in 5ms when it's done
      } else {
        RFCE = 0;
      }
    }
  }
  
  // Enable radio
  RFCE = 1;
  
  // Else just wait here
  while (1)
  {
    LedTest(_whichTest);
    delayms(1);
  }
}
