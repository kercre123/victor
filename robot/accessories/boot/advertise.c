/**
 * Advertising  - this is where the accessory spends 99.9% of its life
 */
#include "hal.h"
#include "nrf.h"

// TX time is 130+128uS, RX time is 130+128uS, radio powerup time is ???
// On the Signal Hound, robot typically ends 500-600uS from TX time
#define AWAKE_TIME_US      1200     // Empirically need 1200uS for reliable OTA
#define BEACON_INTERVAL_US 1000000  // Send beacons about once a second

// This is the advertising packet - it gets patched by the bootloader
// XXX: Correctly set MSBs on to avoid packet errors - once you figure out WHICH IS THE MSB!
code u8 ADVERTISEMENT[] = {
  10,W_TX_PAYLOAD_NOACK,0x1e,0xab,0x11,0xca,    // Broadcast payload - private address REVERSE ORDER
  /* 0x3ff4 */          0xff,0xff,              // Little-endian model (0 = charger, 1 = cube 1, etc)
  /* 0x3ff6 */          0xff,0xff,              // Little-endian firmware patch bitmask (0=installed)
                        0x04,                   // Hardware version - 3=EP2, 4=EP3
                        0xff,                   // Reserved byte
  0
};

// Go into power saving mode and advertise until we get a sync packet
void Advertise()
{
  u8 i;
  
  // Schedule one second wakeups
  CLKLFCTRL = CLKLF_RC;     // Use sloppy RC timer to save power
  WUCON = WAKE_ON_TICK | WAKE_ON_RADIO;
  
  // Turn off all inputs to save power
  for (i = 0; i < 8; i ++)
  {
    P0CON = INPUT_OFF | i;
    P1CON = INPUT_OFF | i;
  }

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
      for (i = 0; i < 8; i ++)
      {
        P0CON = NO_PULL | i;
        P1CON = NO_PULL | i;
      }
      return;
    }
    
    // No reply, go to sleep for one becon interval, then try again
    RTC2CMP1 = US_TO_TICK(BEACON_INTERVAL_US) >> 8;   // MSB, since interval is long
    PetSlowWatchdog();    
    
    P0DIR = 255-PWR_BIT-SPI_PINS;     // Float GPIO, force power off
    P1DIR = ~0;
    P0 = SPI_PINS;        // Forcing SPI_PINS high saves power due to BMA222 restless legs (kicking SCL/SDA every 20ms)
    PWRDWN = RETENTION;   // Sleep HARD
    P0 = PWR_BIT+SPI_PINS;// Start to restore power
    PWRDWN = STANDBY;     // Doze a bit more after the alarm goes off
    PWRDWN = 0;           // Finally, I'm awake with crystal ready
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
