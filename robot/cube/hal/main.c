#include "nrf24le1.h"
#include "hal.h"


// Global variables
extern volatile bool radioBusy;
extern volatile bool gDataReceived;
extern volatile u8 xdata radioPayload[13];
extern volatile enum eRadioTimerState radioTimerState;
extern volatile u8 gMissedPacketCount;
extern volatile u8 cumMissedPacketCount;


#define SOURCE_PWR      6
#define PIN_PWR         (1 << SOURCE_PWR)
#define GPIO_PWR        P0

#if defined(VERIFY_TRANSMITTER)
void InitTimer1()
{  
  TMOD = (TMOD & ~(3<<4)) | (0x01 << 4); // 16-bit
  TMOD |= (0<<6); // timer mode
  TR1 = 1; // start timer
}
#endif

void MainLoop()
{ 
  static u8 packetCount = 0;
  static volatile u8 tapCount = 0;  
  volatile s8 accData[3];
    
  // Pet watchdog
  WDSV = 128; // 1 second
  WDSV = 0;
  
  // Spin during dead time until ready to receive
  // Turn on lights
  StartTimer2();

  // Accelerometer read
  #if !defined(USE_EVAL_BOARD) && !defined(IS_CHARGER) && !defined(TIMING_SCOPE_TRIGGER)
  ReadAcc(accData);
  tapCount += GetTaps();
  #endif

  while(radioTimerState == radioSleep);
  {
    // doing lights stuff TODO: low power mode
  }
  radioTimerState = radioSleep;
  
  // Turn off lights timer (and lights)
  StopTimer2();  

  // Receive data
  ReceiveData(radioStruct.RADIO_TIMEOUT_MSB);

  if(gMissedPacketCount>0)
  {
    // Hold lights
  }
  else
  {
    // Copy packet to LEDs
    SetLedValues(radioPayload);
  }

  // Fill radioPayload with a response
  simple_memcpy(radioPayload, accData, sizeof(accData));
  radioPayload[3] = tapCount;
  radioPayload[4] = cumMissedPacketCount;
  radioPayload[5] = packetCount++;

  // Respond with accelerometer data
  TransmitData();
  
  // Reset Payload
  simple_memset(radioPayload, 0, sizeof(radioPayload));
} 


void Advertise()
{ 
  // Set up advertising parameters
  radioStruct.ADDRESS_RX_PTR = ADDRESS_RX_ADV;
  radioStruct.COMM_CHANNEL = ADV_CHANNEL;
  
  // Put cube ID in radio payload
  radioPayload[0] = CBYTE[0x3FF0];
  radioPayload[1] = CBYTE[0x3FF1];
  radioPayload[2] = CBYTE[0x3FF2];
  radioPayload[3] = CBYTE[0x3FF3];
  
  // Reset radio data received global flag
  gDataReceived = false; 
    
  // Loop advertising sequence until data has been received
  while(gDataReceived == false)
  {
    // Pet watchdog
    WDSV = 0; // 2 seconds // TODO: update this value // TODO add a macro for watchdog
    WDSV = 1;
    
    LightOn(debugLedAdvertise); // advertising light on
    
    // Transmit cube ID
    TransmitData(); 
    
    // Listen for 1ms
    ReceiveData(radioStruct.RADIO_TIMEOUT_MSB); // 1ms timeout // TODO: change to timer bits instead of ms
    LightsOff(); // advertising light off
    
    // Wait 1 second, unless we have received data
    if(gDataReceived == false)
    {
      // Wait 1 second
      // XXX low power mode  
      delay_ms(1000);
    }
  }
  
  // Set up new comm channel parameters
  radioStruct.COMM_CHANNEL = radioPayload[0];
  radioStruct.RADIO_INTERVAL_DELAY = radioPayload[1];
  simple_memcpy(ADDRESS_RX_DAT, &(radioPayload[2]) , sizeof(ADDRESS_RX_DAT));
  radioStruct.ADDRESS_RX_PTR = ADDRESS_RX_DAT;
  
  return;
}


void InitializeMainLoop()
{
  #if !defined(USE_EVAL_BOARD) && !defined(IS_CHARGER)
  // Initialize accelerometer
  InitAcc();
  #endif

  // Initialize radioPayload and LED values to zeros
  simple_memset(radioPayload, 0, sizeof(radioPayload));
  // Copy packet to LEDs
  SetLedValues(radioPayload);
  
  // Initialize LED timer // TODO clean up
  InitTimer2();
  StartTimer2(); // Start LED timer  
 
  gDataReceived = false;
  
    
  #ifdef USE_UART
  InitUart();
  #endif
}


void main(void)
{
  
  // Wait until 16 MHz crystal oscillator is running
  while(hal_clk_get_16m_source() != HAL_CLK_XOSC16M)
  {
  }
  
  // Set up low frequency clock for watchdog
  hal_clklf_set_source(HAL_CLK_RCOSC16M); 
  // Wait until 32.768 kHz RC oscillator is running
  while(hal_clklf_ready() == false)
  {
  }
  
  // Run tests
  RunTests();

  // Initialize watchdog watchdog
  WDSV = 0; // 2 seconds to get through startup
  WDSV = 1;  
  
  // Initalize Radio Timer 
  InitTimer0();
  TR0 = 1; // Start radio timer 
  
  
  // Cube radio state machine
  gCubeState = eAdvertise;
  while(1)
  {
    switch(gCubeState)
    {
      case eAdvertise:
        // Advertise loop
        Advertise();
        gCubeState = eSync;
        break;
      
      case eSync:
        // Listen for up to 150ms
        if(ReceiveDataSync(3)) // 3x50mx ticks = 150ms
        {
          // Initialize accelerometer, lights, etc
          gCubeState = eInitializeMain;
        }
        else
        {
          gCubeState = eAdvertise;
        }
        break;
        
      case eInitializeMain:
          gCubeState = eMainLoop;
        break;
        
      case eMainLoop:
        // If 3 packets are missed, go back to adverising
        if(gMissedPacketCount == MAX_MISSED_PACKETS)
        {
          // To-do: de-init accelerometer, etc.
          gCubeState = eAdvertise;
        }
        break;
        
      default:
        gCubeState = eAdvertise;
    }
  }
  
  return;
}

