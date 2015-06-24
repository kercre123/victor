#include <stdio.h>
#include <stdint.h>
#include "nrf24le1.h"
#include "hal.h"

//#define DO_SIMPLE_LED_TEST
//#define DO_LED_TEST
#define DO_RECEIVER_BEHAVIOR
//#define DO_TRANSMITTER_BEHAVIOR

// Global variables
static volatile bool radioBusy;
static volatile bool gDataReceived;
static volatile u8 payload[13];
static volatile u8 radioTimerState = 0;

void InitTimer0()
{  
  TMOD &= 0xF0;
  TMOD |=  0x01; // Mode 1, 16 bit counter/timer
  
  TL0 = 0xBF; // 40k ticks till overflow 
  TH0 = 0x63;
  
  ET0 = 1; // enable timer 1 interrupt
  EA = 1; // enable global interrupts
  
  TR0 = 1; // Start timer 
}

/*
void InitTimer1()
{  
  TMOD &= 0x0F;
  TMOD |=  (0x01 << 4); // Mode 1, 16 bit counter/timer
  
  TL1 = 0x00; // 40k ticks till overflow 
  TH1 = 0x00;
  
  //TR1 = 1; // Start timer 
}
*/
void PowerDownRadio()
{
  hal_nrf_set_power_mode(HAL_NRF_PWR_DOWN);
  // XXX disable RF interrupt
}

void InitPRX()
{
  /*
   * The following default radio parameters are being used:
 * - RF channel 2
 * - 2 Mbps data rate
 * - RX address 0xE7E7E7E7E7 (pipe 0) and 0xC2C2C2C2C2 (pipe 1)
 * - 1 byte CRC
  */
  
  // Enable the radio clock
  RFCKEN = 1;
  // Enable RF interrupt
  RF = 1;
  // Enable global interrupt
  EA = 1;
  // Configure radio as primary receiver (PRX)
  hal_nrf_set_operation_mode(HAL_NRF_PRX);
  // Set payload width to 13 bytes
  hal_nrf_set_rx_payload_width((int)HAL_NRF_PIPE0, 13U);
  // Power up radio
  hal_nrf_set_power_mode(HAL_NRF_PWR_UP);
  // Enable receiver
  CE_HIGH();
}

void ReceiveData(u8 msTimeout)
{
  //char now;
  //now = TH0;
  
  // Initialize as primary receiver
  InitPRX();
  
  // 1333 ticks per ms
  // ~5.2 per unit of TH0!!!!! XXX XXX everything on radio should be timer 0! 
  TR0 = 0; // Stop timer 
  TL0 = 0x00;
  TH0 = 0x00;
  TR0 = 1; // Start timer 
  
  
  // Wait for a packet, or time out
  radioBusy = true;
  while(radioBusy)
  {    
    if(TH0>(5*msTimeout))
    {
      radioBusy = false;
    }
  }
  PowerDownRadio();
}

void InitPTX()
{
  #ifdef MCU_NRF24LU1P
  // Enable radio SPI
  RFCTL = 0x10U;
  #endif
  
  // Enable the radio clock
  RFCKEN = 1U;

  // Enable RF interrupt
  RF = 1U;

  // Enable global interrupt
  EA = 1U;

  hal_nrf_set_operation_mode(HAL_NRF_PTX);
  
  // Power up radio
  hal_nrf_set_power_mode(HAL_NRF_PWR_UP);
}
  
void TransmitData()
{
  InitPTX();
   // Write payload to radio TX FIFO
  hal_nrf_write_tx_payload(payload, 13U); // XXX _noack

  // Toggle radio CE signal to start transmission
  CE_PULSE();

  radioBusy = true;
  // Wait for radio operation to finish
  while (radioBusy)
  {
  }
  PowerDownRadio();
}

void main(void)
{
  int i;
  char led;
  volatile bool sync;
  s8 accData[3];
  u8 tapCount;
 
  
  #ifdef DO_SIMPLE_LED_TEST
  while(1)
  {
    for(led = 0; led<13; led++)
    {
      LightOn(led);
      delay_ms(333);
    }
  }
  #endif
  
  #ifdef DO_LED_TEST
    InitTimer2();
  for(i = 0; i<13; i++)
  {
    SetLedValue(i,0);
  }
  while(1)
  {
    for(i=0; i<255; i++)
    {
      SetLedValue(4, i);
      SetLedValue(3, 0xFF-i);
      SetLedValue(5, 0xFF-i);
      delay_ms(5);
    }
    for(i=255; i>0; i--)
    {
      SetLedValue(4, i);
      SetLedValue(3, 0xFF-i);
      SetLedValue(5, 0xFF-i);
      delay_ms(5);
    }
  }
  #endif
  
/*  
  while(1)
  {
    ReadAcc(accData);
    delay_ms(5);

  }
*/
  
  while(hal_clk_get_16m_source() != HAL_CLK_XOSC16M)
  {
    // Wait until 16 MHz crystal oscillator is running
  }
  
  
  // listen in loop
  // process LED command
  
  //InitTimer1();

  
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
        payload[led] = 0;
      }
      payload[i] = 255;
      if (radioTimerState == 1) // 30 ms
      {
        radioTimerState = 0; // 30 ms
        // Set to 30 mS
        TR0 = 0; // Stop timer 
        TL0 = 0xBF;
        TH0 = 0x63;
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
  // wait 30 ms (led and accelerometer)
  
#ifdef DO_RECEIVER_BEHAVIOR
  // Initialize accelerometer
  InitAcc();
  
  // Initialize payload and LED values to zeros
  for(led = 0; led<13; led++)
  {
    payload[led] = 0;
    SetLedValue(led, 0);
  }
  
  // Initialize LED timer
  InitTimer2();
  StopTimer2();
  
  // Initialize radio timer
  InitTimer0();
  
  // Synchronize with transmitter
  sync = false;
  SetLedValue(3, 128); // No sync light
  gDataReceived = false;
  while(sync == false)
  {
    // Process lights
    StartTimer2();
    delay_ms(1);
    StopTimer2();
    /*LightOn(3);
    delay_ms(1);*/
    LightsOff();
    
    // Initalize Radio Sync Timer // XXX  
    InitTimer0();
    /*
    TR0 = 0; // Stop timer 
    TL0 = 0xBF; // 40k ticks till overflow 
    TH0 = 0x63;
    TR0 = 1; // Start timer 
    */
    ReceiveData(5U); // 5 ms wait (max)
    for(led = 0; led<13; led++)
    {
      if(payload[led] != 0)
      {
        sync = true;
        break;
      }
    }
  }
  //LightOn(4);
  SetLedValue(4, 128); 
  StartTimer2();
  delay_ms(1);
  
  // Begin main loop. Time = 0
  while(1) 
  {    
    // Spin during dead time until ready to receive
    // Turn on lights
    StartTimer2();
    while(radioTimerState == 0)
    {
      // doing lights stuff
    }
    radioTimerState = 0; 
    // Accelerometer read
    ReadAcc(accData);
    // Turn off lights timer
    StopTimer2();  
    // Make sure lights are off before using the radio
    LightsOff(); // XXX should already be done if StopTimer2() is called

    // Receive data
    ReceiveData(10); // timeout = 4ms
    /*
    if( gDataReceived == true)
    {
      for(led = 0; led<13; led++)
      {
        SetLedValue(led, 0);
      }
      SetLedValue(5, 255);
    }
    gDataReceived = false;
    */
    
    // Copy packet to LEDs
    for(led = 0; led<13; led++)
    {
      SetLedValue(led, payload[led]);
    }
    
    
    // Fill payload with a response 
    payload[0] = accData[0];
    payload[1] = accData[1];
    payload[2] = accData[2];
    // Respond with accelerometer data
    TransmitData();
    payload[0] = 0;
    payload[1] = 0;
    payload[2] = 0;
    
  }
#endif
}

// Radio interrupt
NRF_ISR()
{
  uint8_t irq_flags;
  
  // Read and clear IRQ flags from radio
  irq_flags = hal_nrf_get_clear_irq_flags();
  
  switch(irq_flags)
  {
    // Data received
    case ((1<<(uint8_t)HAL_NRF_RX_DR)):
      // Read payload
      while(!hal_nrf_rx_fifo_empty())
      {
        hal_nrf_read_rx_payload(payload);
      }
      //LightOn(10);
      //while(1);
      // Process payload
      radioBusy = false;
      gDataReceived = true;
      
      // Set timer for ~28 ms from now
      TR0 = 0; // Stop timer 
      TL0 = 0xBF;
      TH0 = 0x63+10;
      TR0 = 1; // Start timer 
      
      break;
      
    // Transmission success
    case (1 << (uint8_t)HAL_NRF_TX_DS):
      radioBusy = false;
      // Data has been sent
      break;
    // Transmission failed (maximum re-transmits)
    case (1 << (uint8_t)HAL_NRF_MAX_RT):
      // When a MAX_RT interrupt occurs the TX payload will not be removed from the TX FIFO.
      // If the packet is to be discarded this must be done manually by flushing the TX FIFO.
      // Alternatively, CE_PULSE() can be called re-starting transmission of the payload.
      // (Will only be possible after the radio irq flags are cleared)
      hal_nrf_flush_tx();
      radioBusy = false;
      break;
    default:
      break;
  }
}


// Timer 0 overflow interrupt
// Overflow flag auto-resets
T0_ISR()
{
  radioTimerState = 1;  
}

// Timer 0 overflow interrupt
// Overflow flag auto-resets
/*T1_ISR()
{
  radioBusy = false;
  TR1 = 0; // Stop timer 
}*/
