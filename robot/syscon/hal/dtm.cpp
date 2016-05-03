#include <string.h>

extern "C" {
  #include "nrf.h"
}

#include "dtm.h"

#include "clad/robotInterface/messageEngineToRobot.h"

static bool dtm_initalized  = false;

static const int         DTM_PAYLOAD_MAX_SIZE = 37;
static const int         DTM_HEADER_SIZE      = 2;

static const int32_t     m_tx_power          = RADIO_TXPOWER_TXPOWER_Pos4dBm;
static const uint8_t     m_radio_mode        = RADIO_MODE_MODE_Ble_1Mbit;
static const uint32_t    m_endian            = RADIO_PCNF1_ENDIAN_Little;
static const uint32_t    m_whitening         = RADIO_PCNF1_WHITEEN_Disabled;
static const uint8_t     m_crcConfSkipAddr   = 1;
static const uint8_t     m_packetHeaderLFlen = 8;
static const uint8_t     m_packetHeaderS0len = 1;
static const uint8_t     m_packetHeaderS1len = 0;
static const uint32_t    m_address           = 0x71764129;
static const uint8_t     m_crcLength         = RADIO_CRCCNF_LEN_Three;
static const uint8_t     m_static_length     = 0;
static const uint32_t    m_balen             = 3;
static const uint32_t    m_crc_poly          = 0x0000065B;
static const uint32_t    m_crc_init          = 0x00555555;
static const uint32_t    m_txIntervaluS      = 625;

static uint8_t           m_content[DTM_HEADER_SIZE + DTM_PAYLOAD_MAX_SIZE];

static void radio_reset(void)
{
  NRF_PPI->CHENCLR = PPI_CHENCLR_CH8_Msk;

  NRF_RADIO->SHORTS          = 0;
  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->TASKS_DISABLE   = 1;

  while (NRF_RADIO->EVENTS_DISABLED == 0) ;

  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->TASKS_RXEN      = 0;
  NRF_RADIO->TASKS_TXEN      = 0;
}

static void radio_init(void)
{
  // Handle BLE Radio tuning parameters from production for DTM if required.
  // Only needed for DTM without SoftDevice, as the SoftDevice normally handles this.
  // PCN-083.
  if ( ((NRF_FICR->OVERRIDEEN) & FICR_OVERRIDEEN_BLE_1MBIT_Msk) == FICR_OVERRIDEEN_BLE_1MBIT_Override)
  {
    NRF_RADIO->OVERRIDE0 = NRF_FICR->BLE_1MBIT[0];
    NRF_RADIO->OVERRIDE1 = NRF_FICR->BLE_1MBIT[1];
    NRF_RADIO->OVERRIDE2 = NRF_FICR->BLE_1MBIT[2];
    NRF_RADIO->OVERRIDE3 = NRF_FICR->BLE_1MBIT[3];
    NRF_RADIO->OVERRIDE4 = NRF_FICR->BLE_1MBIT[4]| (RADIO_OVERRIDE4_ENABLE_Pos << RADIO_OVERRIDE4_ENABLE_Enabled);
  }

  // Turn off radio before configuring it
  radio_reset();

  NRF_RADIO->TXPOWER = m_tx_power;
  NRF_RADIO->MODE    = m_radio_mode << RADIO_MODE_MODE_Pos;

  // Set the access address, address0/prefix0 used for both Rx and Tx address
  NRF_RADIO->PREFIX0    &= ~RADIO_PREFIX0_AP0_Msk;
  NRF_RADIO->PREFIX0    |= (m_address >> 24) & RADIO_PREFIX0_AP0_Msk;
  NRF_RADIO->BASE0       = m_address << 8;
  NRF_RADIO->RXADDRESSES = RADIO_RXADDRESSES_ADDR0_Enabled << RADIO_RXADDRESSES_ADDR0_Pos;
  NRF_RADIO->TXADDRESS   = (0x00 << RADIO_TXADDRESS_TXADDRESS_Pos) & RADIO_TXADDRESS_TXADDRESS_Msk;

  // Configure CRC calculation
  NRF_RADIO->CRCCNF = (m_crcConfSkipAddr << RADIO_CRCCNF_SKIP_ADDR_Pos) |
                      (m_crcLength << RADIO_CRCCNF_LEN_Pos);

  NRF_RADIO->PCNF0 = (m_packetHeaderS1len << RADIO_PCNF0_S1LEN_Pos) |
                     (m_packetHeaderS0len << RADIO_PCNF0_S0LEN_Pos) |
                     (m_packetHeaderLFlen << RADIO_PCNF0_LFLEN_Pos);

  NRF_RADIO->PCNF1 = (m_whitening          << RADIO_PCNF1_WHITEEN_Pos) |
                     (m_endian             << RADIO_PCNF1_ENDIAN_Pos)  |
                     (m_balen              << RADIO_PCNF1_BALEN_Pos)   |
                     (m_static_length      << RADIO_PCNF1_STATLEN_Pos) |
                     (DTM_PAYLOAD_MAX_SIZE << RADIO_PCNF1_MAXLEN_Pos);
}

static void timer_init(void)
{
  NRF_TIMER0->TASKS_STOP        = 1;                      // Stop timer, if it was running
  NRF_TIMER0->TASKS_CLEAR       = 1;
  NRF_TIMER0->MODE              = TIMER_MODE_MODE_Timer;  // Timer mode (not counter)
  NRF_TIMER0->EVENTS_COMPARE[0] = 0;                      // clean up possible old events
  NRF_TIMER0->EVENTS_COMPARE[1] = 0;
  NRF_TIMER0->EVENTS_COMPARE[2] = 0;
  NRF_TIMER0->EVENTS_COMPARE[3] = 0;

  // Timer is polled, but enable the compare0 interrupt in order to wakeup from CPU sleep
  NRF_TIMER0->INTENSET    = TIMER_INTENSET_COMPARE0_Msk;
  NRF_TIMER0->SHORTS      = 1 << TIMER_SHORTS_COMPARE0_CLEAR_Pos;  // Clear the count every time timer reaches the CCREG0 count
  NRF_TIMER0->PRESCALER   = 4;                                     // Input clock is 16MHz, timer clock = 2 ^ prescale -> interval 1us
  NRF_TIMER0->CC[0]       = m_txIntervaluS;                        // 625uS with 1MHz clock to the timer
  NRF_TIMER0->TASKS_START = 1;                                     // Start the timer - it will be running continuously
}

static void radio_prepare(bool rx, uint8_t ch)
{
  NRF_RADIO->TEST         = 0;
  NRF_RADIO->CRCPOLY      = m_crc_poly;
  NRF_RADIO->CRCINIT      = m_crc_init;
  NRF_RADIO->FREQUENCY    = ch;
  NRF_RADIO->PACKETPTR    = (uint32_t)&m_content;
  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->SHORTS       = (1 << RADIO_SHORTS_READY_START_Pos) |
                            (1 << RADIO_SHORTS_END_DISABLE_Pos);

  if (rx)
  {
    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->TASKS_RXEN = 1;  // shorts will start radio in RX mode when it is ready
  }
  else // tx
  {
    NRF_RADIO->TXPOWER = m_tx_power;
  }
}

static void radio_done(void)
{
  NRF_RADIO->TEST = 0;
  NRF_PPI->CHENCLR = PPI_CHENCLR_CH8_Msk;
  NRF_PPI->CH[8].EEP = 0;     // Break connection from timer to radio to stop transmit loop
  NRF_PPI->CH[8].TEP = 0;

  radio_reset();
}

void DTM::start(void) {
  if (dtm_initalized) {
    return ;
  }

  NRF_TIMER0->POWER = 1;
  timer_init();

  NRF_RADIO->POWER = 1;
  radio_init();
  
  dtm_initalized = true;
}

void DTM::enterTestMode(uint8_t mode, uint8_t channel, const void* payload, int length) {
  using namespace Anki::Cozmo::RobotInterface;

  // You cannot use DTM test modes while the process is disabled
  if (!dtm_initalized) {
    return ;
  }

  switch(mode) {
    case DTM_DISABLED:
      radio_done();
      break ;
    case DTM_LISTEN:
      memset(&m_content, 0, sizeof(m_content));
      radio_prepare(true, channel);

      break ;
    case DTM_CARRIER:
      radio_prepare(false, channel);
      NRF_RADIO->TEST = (RADIO_TEST_PLL_LOCK_Enabled << RADIO_TEST_PLL_LOCK_Pos) |
                        (RADIO_TEST_CONST_CARRIER_Enabled << RADIO_TEST_CONST_CARRIER_Pos);

      NRF_RADIO->SHORTS = 1 << RADIO_SHORTS_READY_START_Pos;
      NRF_RADIO->TASKS_TXEN = 1;

      break ;
    case DTM_TRANSMIT:
      memcpy(m_content, payload, length);
      radio_prepare(false, channel);

      NRF_PPI->CH[8].EEP = (uint32_t)&NRF_TIMER0->EVENTS_COMPARE[0];
      NRF_PPI->CH[8].TEP = (uint32_t)&NRF_RADIO->TASKS_TXEN;
      NRF_PPI->CHENSET   = PPI_CHENCLR_CH8_Msk;
      break ;
  }
}

void DTM::stop(void) {
  if (!dtm_initalized) {
    return ;
  }

  radio_done();  
  NRF_RADIO->POWER = 0;
  NRF_TIMER0->POWER = 0;

  dtm_initalized = false;
}
