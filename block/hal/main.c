#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nrf24le1.h"
#include "hal.h"


// Global variables
extern volatile bool radioBusy;
extern volatile bool gDataReceived;
extern volatile u8 xdata radioPayload[13];
extern volatile enum eRadioTimerState radioTimerState;
extern volatile u8 missedPacketCount;
extern volatile u8 cumMissedPacketCount;

#ifdef DO_LOSSY_TRANSMITTER
const u8 skipPacket[6] = {0,0,1,1,1,1};
#endif

void main(void)
{
  u8 i;
  u8 state = 0;
  u8 led;
  u8 numRepeat;
  #ifndef DO_TRANSMITTER_BEHAVIOR
  u8 packetCount = 0;
  volatile bool sync;
  volatile s8 accData[3];
  volatile u8 tapCount = 0;
  #endif
  
  while(hal_clk_get_16m_source() != HAL_CLK_XOSC16M)
  {
    // Wait until 16 MHz crystal oscillator is running
  }
  
  // Set up low frequency clock for watchdog
  hal_clklf_set_source(HAL_CLK_RCOSC16M); 
  while(hal_clklf_ready() == false)
  {
    // Wait until 32.768 kHz RC oscillator is running
  }
  
  // Run tests
  RunTests();

  // Initialize watchdog watchdog
  WDSV = 0; // 2 seconds to get through startup
  WDSV = 1;  
  
#ifdef DO_TRANSMITTER_BEHAVIOR
  // Initalize Timer  
  InitTimer0();
  while(1)
  {
    for(i=0; i<12; i++)
    {
      for(numRepeat=0; numRepeat<1; numRepeat++)
      {
        // Pet watchdog
        WDSV = 128; // 1 second
        WDSV = 0;  
        LightOn(0);       // Transmitting indication light  
        while(radioTimerState == radioSleep);
        {
          // doing lights stuff
        }
        // Set wait period
        TR0 = 0; // Stop timer 
        TL0 = 0xFF - TIMER35MS_L; 
        TH0 = 0xFF - TIMER35MS_H;
        TR0 = 1; // Start timer 
        LightsOff();
        
        // Set payload
        memset(radioPayload, 0, sizeof(radioPayload));
        //radioPayload[i] = numRepeat*25;
        //radioPayload[12] = 128;
        radioPayload[10] = 255;
        #ifdef DO_LOSSY_TRANSMITTER
        if(skipPacket[i%6] == 0)
        {
          TransmitData();
        }
        else
        {
          radioTimerState = radioSleep;
        }
        #else
        TransmitData();
        #endif
      }
    }
  }
#else
  
  // Initialize accelerometer
  InitAcc();

  // Initialize radioPayload and LED values to zeros
  for(led = 0; led<13; led++)
  {
    radioPayload[led] = 0;
    SetLedValue(led, 0);
  }
  
  // Initialize LED timer
  InitTimer2();
  StopTimer2();
 
  // Initialize radio timer
  InitTimer0();

  // Synchronize with transmitter
  sync = false;
  gDataReceived = false;
 
  // Initalize Radio Sync Timer 
  InitTimer0();

  #ifndef DO_MISSED_PACKET_TEST
  while(sync == false)
  {
    // Process lights
    LightOn(3); // No sync light
    delay_ms(1);
    LightsOff();
    
    ReceiveData(9, true); // 5 ms wait (max)
    
    if(gDataReceived)
    {
      sync = true;
      StartTimer2(); // Start LED timer
    }
    
    // Pet watchdog
    WDSV = 128; // 1 second
    WDSV = 0;
  }
  #endif
  
  #ifdef USE_UART
  StopTimer2();
  InitUart();
  #endif
  
  // Begin main loop. Time = 0
  while(1) 
  { 
    // Pet watchdog
    WDSV = 128; // 1 second
    WDSV = 0;
    
    // Spin during dead time until ready to receive
    // Turn on lights
    #ifndef USE_UART
    StartTimer2();
    #endif
    // Accelerometer read
    ReadAcc(accData);
    tapCount += GetTaps();
    while(radioTimerState == radioSleep);
    {
      // doing lights stuff
    }
    radioTimerState = radioSleep;
    // Turn off lights timer (and lights)
    #ifndef USE_UART
    StopTimer2();  
    #endif
    
    // Receive data
    ReceiveData(RADIO_TIMEOUT_MS, false);
    #if defined(DO_MISSED_PACKET_TEST)
    PutChar('R');
    #endif
    #if defined(USE_UART)
    #ifndef DO_MISSED_PACKET_TEST
    for(i=0; i<13; i++)
    {
      PutHex(radioPayload[i]);
    }
    PutString("\r\n");
    #endif
    #endif
    if(missedPacketCount>0)
    {
      SetLedValuesByDelta();
    }
    else
    {
    // Copy packet to LEDs
      SetLedValues(radioPayload);
    }

    // Fill radioPayload with a response
    memcpy(radioPayload, accData, sizeof(accData));
    radioPayload[3] = tapCount;
    radioPayload[4] = cumMissedPacketCount;
    radioPayload[5] = packetCount++;
    radioPayload[6] = BLOCK_ID;
    #if defined(USE_UART) 
    #ifndef DO_MISSED_PACKET_TEST
    for(i=0; i<7; i++)
    {
      PutHex(radioPayload[i]);
    }
    PutString("\r\n");
    #endif
    #endif
    // Respond with accelerometer data
    #if defined(DO_MISSED_PACKET_TEST)
    PutChar('T');
    #endif
    TransmitData();
    // Reset Payload
    memset(radioPayload, 0, sizeof(radioPayload));
  }
#endif // transmitter behavior
}

