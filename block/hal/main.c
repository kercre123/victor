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


void main(void)
{
  u8 i;
  u8 state = 0;
  u8 led;
  u8 numRepeat;
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
    for(i=0; i<13; i++)
    {
      for(numRepeat=0; numRepeat<10; numRepeat++)
      {
        LightOn(0);       // Transmitting indication light  
        while(radioTimerState == radioSleep);
        {
          // doing lights stuff
        }
        LightsOff();
        
        // Set payload
        memset(radioPayload, 0, sizeof(radioPayload));
        //radioPayload[i] = 255;
        radioPayload[10] = 255;
        TransmitData();
        
        // Set to 30 mS wait
        TR0 = 0; // Stop timer 
        TL0 = 0xFF - TIMER30HZ_L; 
        TH0 = 0xFF - TIMER30HZ_H;
        TR0 = 1; // Start timer 
        radioTimerState = radioSleep; 
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
    #ifdef DO_RADIO_LED_TEST
      sync = true;
    #endif
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

  // Begin main loop. Time = 0
  while(1) 
  {    
    // Spin during dead time until ready to receive
    // Turn on lights
    StartTimer2();
    // Accelerometer read
    ReadAcc(accData);
    tapCount += GetTaps();
    while(radioTimerState == radioSleep);
    {
      // doing lights stuff
    }
    radioTimerState = radioSleep; 
    // Turn off lights timer (and lights)
    StopTimer2();  

    // Receive data
    ReceiveData(RADIO_TIMEOUT_MS); 
    
    // Copy packet to LEDs
    SetLedValues(radioPayload);
    #ifdef DO_RADIO_LED_TEST
    memset(radioPayload, 0, sizeof(radioPayload));
    radioPayload[0] = 0xFF;
    state++;
    #endif

    // Fill radioPayload with a response
    memcpy(radioPayload, accData, sizeof(accData));
    radioPayload[3] = tapCount;
    // Respond with accelerometer data
    TransmitData();
    // Reset Payload
    memset(radioPayload, 0, sizeof(radioPayload));
  }  
}

