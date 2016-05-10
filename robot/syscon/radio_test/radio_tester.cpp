extern "C" {
  #include "nrf.h"
  #include "nrf_gpio.h"
  #include "nrf_sdm.h"
}

static const uint32_t TX_PIN = 17;
static const uint32_t RX_PIN = 18;

static uint8_t measure(uint8_t frequency) {
  NRF_RADIO->FREQUENCY = frequency;
  
  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_RXEN = 1;
  while(NRF_RADIO->EVENTS_READY == 0);
  
  NRF_RADIO->EVENTS_END = 0;
  NRF_RADIO->TASKS_START = 1;

  NRF_RADIO->EVENTS_RSSIEND = 0;
  NRF_RADIO->TASKS_RSSISTART = 1;
  while(NRF_RADIO->EVENTS_RSSIEND == 0);

  uint8_t data = NRF_RADIO->RSSISAMPLE;

  NRF_RADIO->EVENTS_DISABLED = 0;    
  NRF_RADIO->TASKS_DISABLE   = 1;

  while(NRF_RADIO->EVENTS_DISABLED == 0) ;

  return data;
}

int tohex(int i)
{
  return i > 9 ? i + 'a'-10 : i + '0';
}

int main(void)
{
  // Start 16 MHz crystal oscillator.
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART    = 1;

  // Wait for the external oscillator to start up.
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) ;
  
  // Enable and configure the uart
  nrf_gpio_cfg_input(RX_PIN, NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_output(TX_PIN);

  NRF_UART0->POWER = 1;

  NRF_UART0->CONFIG = 0;
  NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
    
  NRF_UART0->BAUDRATE = UART_BAUDRATE_BAUDRATE_Baud1M;
  NRF_UART0->PSELTXD = TX_PIN;
  NRF_UART0->PSELRXD = RX_PIN;

  // Power our radio
  NRF_RADIO->POWER = 1;

  // Run forever, because we are awesome.
  for (;;) {    
    /*
    NRF_UART0->TASKS_STARTRX = 1;

    while (!NRF_UART0->EVENTS_RXDRDY) ;
    uint8_t channel = NRF_UART0->RXD;

    NRF_UART0->TASKS_STOPRX = 1;
    NRF_UART0->EVENTS_RXDRDY = 0;
    */
    
    uint8_t channel = 81;
    uint8_t reading = 0;
    for (int i = 0; i < 1000; i++)
    {
      uint8_t read = measure(channel);
      if (read > reading)
        reading = read;
    }
      
    NRF_UART0->TASKS_STARTTX = 1;

    //NRF_UART0->TXD = channel;
    NRF_UART0->TXD = tohex(reading >> 4);
    while (!NRF_UART0->EVENTS_TXDRDY) ;
    NRF_UART0->EVENTS_TXDRDY = 0;

    //NRF_UART0->TXD = reading;
    NRF_UART0->TXD = tohex(reading & 15);
    while (!NRF_UART0->EVENTS_TXDRDY) ;
    NRF_UART0->EVENTS_TXDRDY = 0;
    
    NRF_UART0->TXD = '.';
    while (!NRF_UART0->EVENTS_TXDRDY) ;
    NRF_UART0->EVENTS_TXDRDY = 0;

    NRF_UART0->TASKS_STOPTX = 1;
  }
}
