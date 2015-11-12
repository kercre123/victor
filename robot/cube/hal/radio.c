#include "hal.h"
#include "hal_nrf.h"

// Global variables
volatile bool radioBusy;
volatile bool gDataReceived = false;
volatile u8 xdata radioPayload[13];
volatile enum eRadioTimerState radioTimerState = radioSleep;
volatile u8 missedPacketCount = 0;
volatile u8 cumMissedPacketCount = 0;
#ifdef VERIFY_TRANSMITTER
volatile u8 TH1now, TL1now;
#endif


void InitTimer0()
{  
  TMOD &= 0xF0;
  TMOD |=  0x01; // Mode 1, 16 bit counter/timer
  
  TL0 = 0xFF - TIMER35MS_L; 
  TH0 = 0xFF - TIMER35MS_H;
  
  ET0 = 1; // enable timer 1 interrupt
  EA = 1; // enable global interrupts
  
  TR0 = 1; // Start timer 
}

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
  // Set address
  hal_nrf_set_address(HAL_NRF_PIPE1, ADDRESS);
  // Set datarate
  hal_nrf_set_datarate(HAL_NRF_250KBPS);
  // Set channel
  hal_nrf_set_rf_channel(COMM_CHANNEL);
  // Set radioPayload width to 13 bytes
  hal_nrf_set_rx_payload_width((int)HAL_NRF_PIPE1, 13U);
  // Flush RX FIFO
  hal_nrf_flush_rx();
  // Power up radio
  hal_nrf_set_power_mode(HAL_NRF_PWR_UP);
  // Enable receiver
  CE_HIGH();
}

void ReceiveDataSync()
{
  // Initialize as primary receiver
  radioBusy=true;
  InitPRX();
  while(radioBusy)
  {
    // Pet watchdog
    WDSV = 128; // 1 second
    WDSV = 0;
    delay_us(10);
  }
  TR0 = 1; // turn timer back on
  PowerDownRadio();
}

void ReceiveData(u8 msTimeout, bool syncMode)
{
  u8 now;
  // get time
  now = TH0;

  // Wait for a packet, or time out
  radioBusy = true;
  // Initialize as primary receiver
  InitPRX();
  
  #ifndef LISTEN_FOREVER
  if(missedPacketCount<MAX_MISSED_PACKETS) // do timeout if less than MAX_MISSED_PACKETS, else listen forever
  {
    while(radioBusy)
    {   
      if((TH0-now+1)>(5*msTimeout)) 
      {
        // we timed out
        if(!syncMode)
        {
          missedPacketCount++;
          cumMissedPacketCount++; 
          #if defined(DO_MISSED_PACKET_TEST)
          PutChar('\t');
          PutHex(missedPacketCount);
          #endif
        }
        radioBusy = false;
      }
    }
  }else if(missedPacketCount == MAX_MISSED_PACKETS)
  {
    TR0 = 0; // turn off timer
    #if defined(DO_MISSED_PACKET_TEST)
      PutString("\tL\r\n");
    #endif
    while(radioBusy)
    {
    }
    // timer reset in radio interrupt
     TR0 = 1; // turn timer back on
  }
  #endif
  #ifdef LISTEN_FOREVER
  while(radioBusy)
  {
    delay_ms(1);
    PutChar('.');
  }
  #endif
 
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
  // Set datarate
  hal_nrf_set_datarate(HAL_NRF_250KBPS);
  // Turn off auto-retransmit
  hal_nrf_set_auto_retr(0, 0);
  // Set power
  hal_nrf_set_output_power(HAL_NRF_0DBM);
  // Set channel
  hal_nrf_set_rf_channel(COMM_CHANNEL);
  // Set address (only if acting as transmitter)
  #ifdef DO_TRANSMITTER_BEHAVIOR
  hal_nrf_set_address(HAL_NRF_TX, ADDRESS);
  #endif
  // Power up radio
  hal_nrf_set_power_mode(HAL_NRF_PWR_UP);
}
  

void TransmitData()
{
  InitPTX();
   // Write payload to radio TX FIFO
  hal_nrf_write_tx_payload_noack(radioPayload, 13U);

  // Toggle radio CE signal to start transmission
  CE_PULSE();

  radioBusy = true;
  // Wait for radio operation to finish
  while (radioBusy)
  {
  }
  PowerDownRadio();
  radioTimerState = radioSleep; 
}

// Radio interrupt
NRF_ISR()
{
  uint8_t irq_flags;
  
  EA = 0; // disable interrupts
  
  // Read and clear IRQ flags from radio
  irq_flags = hal_nrf_get_clear_irq_flags();
  gDataReceived = true; 
  switch(irq_flags)
  {
    // Data received
    case ((1<<(uint8_t)HAL_NRF_RX_DR)):
      // Set timer for ~35-offset ms from now
      TR0 = 0; // Stop timer 
      TL0 = 0xFF - TIMER35MS_L; 
      TH0 = 0xFF - TIMER35MS_H + WAKEUP_OFFSET;
      TR0 = 1; // Start timer
      #ifdef VERIFY_TRANSMITTER
      TH1now = TH1;
      TL1now = TL1;
      TH1 = 0;
      TL1 = 0;
      #endif
      // Read payload
      while(!hal_nrf_rx_fifo_empty())
      {
        hal_nrf_read_rx_payload(radioPayload);
      }
      radioBusy = false;
      gDataReceived = true; 
      missedPacketCount = 0;
      break;
      
    // Transmission success
    case (1 << (uint8_t)HAL_NRF_TX_DS):
      radioBusy = false;
      // Data has been sent
      break;
    // Transmission failed (maximum re-transmits)
    case (1 << (uint8_t)HAL_NRF_MAX_RT):
      // When a MAX_RT interrupt occurs the TX radioPayload will not be removed from the TX FIFO.
      // If the packet is to be discarded this must be done manually by flushing the TX FIFO.
      // Alternatively, CE_PULSE() can be called re-starting transmission of the radioPayload.
      // (Will only be possible after the radio irq flags are cleared)
      hal_nrf_flush_tx();
      radioBusy = false;
      break;
    default:
      break;
  }
  EA = 1; // enable interrupts
}


// Timer 0 overflow interrupt
// Overflow flag auto-resets
T0_ISR()
{
  EA = 0; // disable interrupts
  radioTimerState = radioWakeup; 
  // set for 35ms wakeup
  TR0 = 0; // Stop timer 
  TL0 = 0xFF - TIMER35MS_L; 
  TH0 = 0xFF - TIMER35MS_H;
  TR0 = 1; // Start timer 
  EA = 1; // enable interrupts  
}

/*
If we miss a packet, we're still on for 30 Hz. 
If we receive a packet, we wake up at 30 Hz - offset
*/
