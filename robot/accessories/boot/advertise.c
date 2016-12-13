/**
 * Advertising  - this is where the accessory spends 99.9% of its life
 */
#include "hal.h"
#include "nrf.h"

// TX time is 130+136uS, RX time is 130+136uS, turnaround time is usually <200ms
// On the Signal Hound, robot typically ends 500-600uS from TX start
#define AWAKE_TIME_US      750      // Empirically need 750uS for reliable OTA
#define BEACON_INTERVAL_US 1000000  // Send beacons about once a second

// This is the advertising packet - it gets patched by the bootloader
code u8 ADVERTISEMENT[] = {
  10,W_TX_PAYLOAD_NOACK,0x1e,0xab,0x11,0xca,    // Broadcast payload - private address BROADCAST ORDER
  /* 0x3ff4 */          0xff,0xff,              // Little-endian model (0 = charger, 1 = cube 1, etc)
  /* 0x3ff6 */          0xff,0xff,              // Little-endian firmware patch bitmask (0=installed)
                        0x06,                   // Hardware version - 3=EP2, 4=EP3/EP3F :(, 6=Pilot/Prod
                        0xff,                   // Reserved byte  
  WREG | CONFIG,        PWR_UP | EN_CRC | CRCO, // Power up TX with 16-bit CRC
  0
};

extern code u8 SETUP_TX_BEACON[];
void RadioSetup(u8 code *conf);

// Go into power saving mode and advertise until we get a sync packet
void Advertise()
{
  u8 i;
  
  // Schedule one second wakeups
  CLKLFCTRL = CLKLF_RC;     // Use sloppy RC timer to save power
  WUCON = WAKE_ON_TICK | WAKE_ON_RADIO | WAKE_ON_XOSC;
  
  // Turn off all inputs to save power
  for (i = INPUT_OFF; i < 8+INPUT_OFF; i++)
  {
    P0CON = i;
    P1CON = i;
  }
  
  // Set up radio for advertise/receive
  RadioSetup(SETUP_TX_BEACON);

  while (1)
  {
    // Start the "awake" timer - when we time out, we go back to sleep
    RTC2CON = 0;
    RTC2CMP0 = US_TO_TICK(AWAKE_TIME_US);
    RTC2CMP1 = 0;
    RTC2CON = RTC_COMPARE_RESET | RTC_ENABLE;

    // Send a beacon - if we get a reply, exit low-power mode and return
    if (RadioBeacon())
    { 
      RTC2CON = 0;              
      for (i = NO_PULL; i < 8+NO_PULL; i++)
      {
        P0CON = i;
        P1CON = i;
      }
      return;
    }
    
    // No reply, go to sleep for one becon interval, then try again
    RTC2CMP1 = US_TO_TICK(BEACON_INTERVAL_US) >> 8;   // MSB, since interval is long
    PetSlowWatchdog();    
    
    CLKCTRL = X16IRQ;       // Turn on X16IRQ to wake up faster
    P0DIR = 255-PWR_BIT-SPI_PINS;     // Float GPIO, force power off
    P1DIR = ~0;
    
    P0 = SPI_PINS;        // Forcing SPI_PINS high saves power due to BMA222 restless legs (kicking SCL/SDA every 20ms)
    IRCON = 0;            // Clear any wakeup sources
    PWRDWN = RETENTION;   // Sleep HARD
    P0 = PWR_BIT+SPI_PINS;// Start to restore power
    PWRDWN = STANDBY;     // Doze a bit more until the IRQ goes off
    CLKCTRL = 0;          // Turn off X16IRQ
  }
}

// Return the model/type from the advertising packet
u8 GetModel()
{
  // If MSB is not set (0xff), our model is the lsb
  // This allows us to change model once at the factory
  u8 lsb = *(u8 code*)(0x3ff4);
  u8 msb = *(u8 code*)(0x3ff5);
  if (0xff == msb)
    return lsb;
  return msb;
}
