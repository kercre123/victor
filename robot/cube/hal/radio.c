#include "hal.h"
#include "hal_nrf.h"

// Global variables
volatile bool radioBusy;
volatile bool gDataReceived = false;
volatile u8 radioPayload[13];
volatile enum eRadioTimerState radioTimerState = radioSleep;
volatile u8 gMissedPacketCount = 0;
volatile u8 cumMissedPacketCount = 0;
volatile u8 radioTimerCounter;
volatile u8 ADDRESS_RX_DAT[5];
volatile u8 gCubeState;

volatile struct RadioStruct radioStruct = 
{
  ADV_CHANNEL, // COMM_CHANNEL
  0xB6, // RADIO_INTERVAL_DELAY
  15, // RADIO_TIMEOUT_MSB ~ 3ms (change to 5 later)
  8, // RADIO_WAKUP_OFFSET
  ADDRESS_TX, // ADDRESS_TX_PTR 
  ADDRESS_RX_ADV // ADDRESS_RX_PTR
};

void InitTimer0()
{  
  TMOD &= 0xF0;
  TMOD |=  0x01; // Mode 1, 16 bit counter/timer
  
  TL0 = 0; 
  TH0 = 0;
  
  ET0 = 1; // enable timer 1 interrupt
  EA = 1; // enable global interrupts
  
  //TR0 = 1; // Start timer 
}

void PowerDownRadio()
{
  hal_nrf_set_power_mode(HAL_NRF_PWR_DOWN);
  // XXX disable RF interrupt
}


// TODO make use of multiple pipes?
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
  hal_nrf_set_address(HAL_NRF_PIPE1, radioStruct.ADDRESS_RX_PTR);
  // Set datarate
  hal_nrf_set_datarate(HAL_NRF_1MBPS);
  // Set channel
  hal_nrf_set_rf_channel(radioStruct.COMM_CHANNEL);
  // Set radioPayload width to 13 bytes
  hal_nrf_set_rx_payload_width((int)HAL_NRF_PIPE1, 13U);
  // Flush RX FIFO
  hal_nrf_flush_rx();
  // Power up radio
  hal_nrf_set_power_mode(HAL_NRF_PWR_UP);
  // Enable receiver
  CE_HIGH();
}


/*
Wait for receive to establish sync
arguments:
  u8 timeout50msTicks - number of ~50ms ticks, 16b counter / 16 MHz / 12x prescaler = 49.2 ms
                        counter is handled in timer overflow interrupt
Returns true if sync established
*/
bool ReceiveDataSync(u8 timeout50msTicks) 
{
  // Set radio as busy
  radioBusy = true;
  
  // Initialize radio as primary receiver
  InitPRX();
  
  // Reset Timer
  TR0 = 0; // Stop timer
  TL0 = 0;
  TH0 = 0;
  TR0 = 1; // Start timer
  
  // Reset radio timer counter (50ms Ticks)
  radioTimerCounter = 0;
  
  // Wait for received data, or timeout
  while(radioBusy) // XXX eventually it would be nice for better radio states
  {
    // Pet watchdog
    WDSV = 128; // 1 second // XXX
    WDSV = 0;
    delay_us(10); // XXX
    if( radioTimerCounter >= timeout50msTicks ) // timeout condition
    {
      PowerDownRadio(); // Turn off radio
      radioBusy = false; // exit loop 
      TR0 = 0; // Stop timer
    }
  }
  
  // Sync timing is configured in radio receive interrupt
  
  PowerDownRadio(); // Turn off radio
  return true;
}


/*
Receive data, with timeout
arguments:
  u8 timerMsbTimeout - tick =  8b counter / 16 MHz / 12x prescaler = 192us
                       counter is high byte of 16b counter
Returns true if sync established
*/
void ReceiveData(u8 timerMsbTimeout) 
{
  u8 now;
  #ifdef EMULATE_BODY
  u8 addr[5];
  u8 i;
  #endif
  
  // Get timer MSB
  now = TH0;

  // Set radio as busy
  radioBusy = true;
  
  // Initialize as primary receiver
  InitPRX();
  
  #ifdef EMULATE_BODY
  hal_nrf_get_address(HAL_NRF_PIPE1, addr);
  /*PutString("Receiving on PIPE1 address: ");
  for(i=0; i<5; i++)
  {
    PutHex(addr[i]);
  }
  PutString("\r\n");*/
  #endif

  // Wait for received data, or timeout
  while(radioBusy)
  {   
    if((TH0-now+1)>(timerMsbTimeout) && timerMsbTimeout !=0)  // timeout condition
    {
      gMissedPacketCount++; // increment missed packet counters 
      cumMissedPacketCount++; 
      radioBusy = false; // exit loop
    }
  }
 
  // Power down radio
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
  hal_nrf_set_datarate(HAL_NRF_1MBPS);
  // Turn off auto-retransmit
  hal_nrf_set_auto_retr(0, 0);
  // Set power
  hal_nrf_set_output_power(HAL_NRF_0DBM);
    // Set address
  hal_nrf_set_address(HAL_NRF_TX, radioStruct.ADDRESS_TX_PTR); // cube to body
  // Set channel
  hal_nrf_set_rf_channel(radioStruct.COMM_CHANNEL);
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
  //gDataReceived = true;  // can't have this with advertising
  switch(irq_flags)
  {
    // Data received
    case ((1<<(uint8_t)HAL_NRF_RX_DR)):
      // Set timer for ~35-offset ms from now
      TR0 = 0; // Stop timer 
      TL0 = 0; 
      TH0 = 0xFF - radioStruct.RADIO_INTERVAL_DELAY + radioStruct.RADIO_WAKEUP_OFFSET;
      TR0 = 1; // Start timer
      // Read payload
      while(!hal_nrf_rx_fifo_empty())
      {
        hal_nrf_read_rx_payload(radioPayload);
      }
      radioBusy = false;
      gDataReceived = true; 
      gMissedPacketCount = 0;
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
  if(gCubeState == eSync)
  {
    radioTimerCounter++;
  }
  else
  {
    
    EA = 0; // disable interrupts
    radioTimerState = radioWakeup; 
    // set for 35ms wakeup
    TR0 = 0; // Stop timer 
    TL0 = 0; 
    TH0 = 0xFF - radioStruct.RADIO_INTERVAL_DELAY;
    TR0 = 1; // Start timer 
    EA = 1; // enable interrupts  
  }
}

/*
If we miss a packet, we're still on for 30 Hz. 
If we receive a packet, we wake up at 30 Hz - offset
*/
