#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nrf24le1.h"
#include "hal.h"


// Global variables
extern volatile bool radioBusy;
extern volatile bool gDataReceived;
extern volatile u8 radioPayload[13];
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
  u8 packetCount = 0;
  volatile bool sync;
  
  volatile s8 accData[3];
  volatile u8 tapCount = 0;
  
  while(hal_clk_get_16m_source() != HAL_CLK_XOSC16M)
  {
    // Wait until 16 MHz crystal oscillator is running
  }

  // Run tests
  RunTests();
    
#ifdef DO_TRANSMITTER_BEHAVIOR
  // Initalize Timer  
  InitTimer0();
  while(1)
  {
    for(i=0; i<12; i++)
    {
      for(numRepeat=0; numRepeat<1; numRepeat++)
      {
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
        #elif
        //TransmitData();
        #endif
      }
    }
  }
#endif 
  
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
  SetLedValue(3, 0xFF); // No sync light
  gDataReceived = false;
  
  // Initalize Radio Sync Timer // XXX  
  InitTimer0();
  TR0 = 0; // Stop timer 
  
  while(sync == false)
  {
    // Process lights
    StartTimer2();
    delay_ms(1);
    StopTimer2();
    
    ReceiveData(5U); // 5 ms wait (max)

    if(gDataReceived)
    {
      sync = true;
    }
    
  }
  SetLedValue(4, 0xFF); 
  StartTimer2();
  delay_ms(1);
  LightsOff();
  
  #ifdef USE_UART
  StopTimer2();
  InitUart();
  #endif
  
  // Begin main loop. Time = 0
  while(1) 
  {    
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
    // Turn off lights timer (and lights)
    #ifndef USE_UART
    StopTimer2();  
    #endif
    
    // Receive data
    ReceiveData(RADIO_TIMEOUT_MS);
    #ifdef USE_UART
    for(i=0; i<13; i++)
    {
      puthex(radioPayload[i]);
    }
    putstring("\r\n");
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
    #ifdef USE_UART
    for(i=0; i<7; i++)
    {
      puthex(radioPayload[i]);
    }
    putstring("\r\n");
    #endif
    // Respond with accelerometer data
    TransmitData();
    // Reset Payload
    memset(radioPayload, 0, sizeof(radioPayload));
  }  
}

