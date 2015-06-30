#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nrf24le1.h"
#include "hal.h"

//#define LISTEN_FOREVER
//#define DO_TRANSMITTER_BEHAVIOR

// Global variables
extern volatile bool radioBusy;
extern volatile bool gDataReceived;
extern volatile u8 radioPayload[13];
extern volatile u8 radioTimerState;


void main(void)
{
  int i;
  char state = 0;
  char led;
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
      // Reset LEDs
      for(led = 0; led<13; led++)
      {
        radioPayload[led] = 0;
      }
      radioPayload[i] = 255;
      if (radioTimerState == 1) // 30 ms
      {
        radioTimerState = 0; // 30 ms
        // Set to 30 mS
        TR0 = 0; // Stop timer 
        TL0 = 0xFF - TIMER30HZ_L; 
        TH0 = 0xFF - TIMER30HZ_H;
        TR0 = 1; // Start timer 
        LightsOff();      // Turn off light
        TransmitData();
      }
      else
      {  
        LightOn(0);       // Transmitting indication light    
      }
      //ReceiveData(1);
      //ReceiveData(3);
      //delay_ms(10);
      //delay_us(333);
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
  while(sync == false)
  {
    #ifdef DO_RADIO_LED_TEST
      sync = true;
    #endif
    // Process lights
    StartTimer2();
    delay_ms(1);
    StopTimer2();
    
    // Initalize Radio Sync Timer // XXX  
    InitTimer0();
    ReceiveData(5U); // 5 ms wait (max)
    for(led = 0; led<13; led++)
    {
      if(radioPayload[led] != 0)
      {
        sync = true;
        break;
      }
    }
  }
  SetLedValue(4, 0xFF); 
  StartTimer2();
  delay_ms(1);
  
  // Begin main loop. Time = 0
  while(1) 
  {    
    // Spin during dead time until ready to receive
    // Turn on lights
    StartTimer2();
    // Accelerometer read
    ReadAcc(accData);
    tapCount += GetTaps();
    while(radioTimerState == 0)
    {
      // doing lights stuff
    }
    radioTimerState = 0; 
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

