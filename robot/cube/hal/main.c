#include "nrf24le1.h"
#include "hal.h"


// Global variables
extern volatile bool radioBusy;
extern volatile bool gDataReceived;
extern volatile u8 xdata radioPayload[13];
extern volatile enum eRadioTimerState radioTimerState;
extern volatile u8 gMissedPacketCount;
extern volatile u8 cumMissedPacketCount;
extern volatile struct RadioStruct radioStruct;
extern volatile u8 ADDRESS_RX_DAT[5];
extern volatile u8 gCubeState;


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


void BreakPointLight(u8 led)
{
    // Pet watchdog
  WDSV = 0; // 2 seconds // TODO: update this value // TODO add a macro for watchdog
  WDSV = 1;
  StopTimer2();
  LightOn(led);
  while(1);
}

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
    
    // Pet watchdog
    WDSV = 0; // 2 seconds // TODO: update this value // TODO add a macro for watchdog
    WDSV = 1;
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
  // Initialize radioPayload and LED values to zeros
  simple_memset(radioPayload, 0, sizeof(radioPayload));
  // Copy packet to LEDs
  SetLedValues(radioPayload);
  
  // Initialize LED timer // TODO clean up
  InitTimer2();
  StartTimer2(); // Start LED timer  
  
  gDataReceived = false;

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
  
  #ifndef EMULATE_BODY
  // Initialize watchdog watchdog
  WDSV = 0; // 2 seconds to get through startup
  WDSV = 1;  
  #endif
  
   #ifdef USE_UART
  InitUart();
  #endif
  #if !defined(USE_EVAL_BOARD) && !defined(IS_CHARGER)
  // Initialize accelerometer
  InitAcc();
  #endif
  
  // Initalize Radio Timer 
  InitTimer0();
  TR0 = 1; // Start radio timer 
  
  #ifndef EMULATE_BODY
  // Cube radio state machine
  gCubeState = eAdvertise;
  while(1)
  {
    switch(gCubeState)
    {
      case eAdvertise:
        // Advertise loop
        Advertise();
        WDSV = 0; // 2 seconds to get through startup
        WDSV = 1;       
        gCubeState = eSync;
        break;
      
      case eSync:
        // Listen for up to 150ms
        if(ReceiveDataSync(3)) // 3x50mx ticks = 150ms
        {
          //TR0 = 1; // Start timer  
          // Initialize accelerometer, lights, etc
          gCubeState = eInitializeMain;
          //TransmitData(); // send something back, for kicks
        }
        else
        {
          gCubeState = eAdvertise;
        }
        break;
        
      case eInitializeMain:
          gCubeState = eMainLoop;
          InitializeMainLoop();
        break;
        
      case eMainLoop:
        WDSV = 0; // 2 seconds to get through startup
        WDSV = 1;       
        MainLoop();
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
  #else
  InitUart();

  gDataReceived = false;
  gCubeState = eSync;
  
  // Cube radio state machine
  gCubeState = eScan;
  while(1){
    switch(gCubeState)
    {
      case eScan:
        PutString("Scanning...\r\n");
        // Set up advertising parameters
        radioStruct.ADDRESS_RX_PTR = ADDRESS_TX;
        radioStruct.ADDRESS_TX_PTR = ADDRESS_RX_ADV;
        radioStruct.COMM_CHANNEL = ADV_CHANNEL;
        ReceiveData(200);
        WDSV = 128; // 1 seconds // TODO: update this value // TODO add a macro for watchdog
        WDSV = 0;
        if(gDataReceived)
        {
          gCubeState = eRespond;
          gDataReceived = 0;
        }
        break;
      case eRespond:
        // set up payload
        radioPayload[0] = 37;
        radioPayload[1] = 52; // 9.984 ms
        radioPayload[2] = 0xA7;
        radioPayload[3] = 0xA7;
        radioPayload[4] = 0xA7;
        radioPayload[5] = 0xA7;
        radioPayload[6] = 0xA7;
        delay_us(200);
        TransmitData();
        WDSV = 128; // 1 seconds // TODO: update this value // TODO add a macro for watchdog
        WDSV = 0;
        PutString("Responded\r\n");
        gCubeState = eMainLoop;
        radioStruct.ADDRESS_RX_PTR = ADDRESS_TX;
        radioStruct.ADDRESS_TX_PTR = ADDRESS_X;
        radioStruct.COMM_CHANNEL = 37;
        WDSV = 0; // 4 seconds // TODO: update this value // TODO add a macro for watchdog
        WDSV = 2;
        break;
      case eMainLoop:
        simple_memset(radioPayload, 0xFF, sizeof(radioPayload));
        TransmitData();
        gDataReceived = false;
        ReceiveData(5);
        PutHex(gDataReceived);
        delay_ms(8);
        delay_us(1);
        break;
      default:
        break;
    }
  }
  #endif

  return;
}

