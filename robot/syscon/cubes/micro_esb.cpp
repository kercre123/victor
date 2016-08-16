/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include <string.h>

#include "micro_esb.h"


// RF parameters
static uesb_config_t            m_config_local;

// TX/RX FIFO
static uesb_payload_fifo_t      m_tx_fifo;
static uesb_payload_fifo_t      m_rx_fifo;

// Run time variables
uesb_mainstate_t                m_uesb_mainstate        = UESB_STATE_UNINITIALIZED;

// Macros
#define                         DISABLE_RF_IRQ      NVIC_DisableIRQ(RADIO_IRQn)
#define                         ENABLE_RF_IRQ       NVIC_EnableIRQ(RADIO_IRQn)

static void update_rf_payload_format(uint32_t payload_length)
{
  NRF_RADIO->PCNF0 =  (0 << RADIO_PCNF0_S0LEN_Pos) | 
                      (0 << RADIO_PCNF0_LFLEN_Pos) | 
                      (0 << RADIO_PCNF0_S1LEN_Pos);

  NRF_RADIO->PCNF1 =  (RADIO_PCNF1_WHITEEN_Disabled        << RADIO_PCNF1_WHITEEN_Pos) |
                      (RADIO_PCNF1_ENDIAN_Big              << RADIO_PCNF1_ENDIAN_Pos)  |
                      ((m_config_local.rf_addr_length - 1) << RADIO_PCNF1_BALEN_Pos)   |
                      (payload_length                      << RADIO_PCNF1_STATLEN_Pos) |
                      (payload_length                      << RADIO_PCNF1_MAXLEN_Pos);
}

// Function that swaps the bits within each byte in a uint32. Used to convert from nRF24L type addressing to nRF51 type addressing
static inline uint32_t bytewise_bit_swap(uint32_t inp)
{
  inp = (inp & 0xF0F0F0F0) >> 4 | (inp & 0x0F0F0F0F) << 4;
  inp = (inp & 0xCCCCCCCC) >> 2 | (inp & 0x33333333) << 2;
  return (inp & 0xAAAAAAAA) >> 1 | (inp & 0x55555555) << 1;
}

static void update_radio_parameters()
{  
  // TX power
  NRF_RADIO->TXPOWER   = m_config_local.tx_output_power   << RADIO_TXPOWER_TXPOWER_Pos;

  // RF bitrate
  NRF_RADIO->MODE      = m_config_local.bitrate           << RADIO_MODE_MODE_Pos;

  // CRC configuration
  NRF_RADIO->CRCCNF    = m_config_local.crc               << RADIO_CRCCNF_LEN_Pos;
  
  switch(m_config_local.crc)
  {
  case RADIO_CRCCNF_LEN_Two:
    NRF_RADIO->CRCINIT = 0xFFFFUL;      // Initial value
    NRF_RADIO->CRCPOLY = 0x11021UL;     // CRC poly: x^16+x^12^x^5+1
    break ;
  case RADIO_CRCCNF_LEN_One:
    NRF_RADIO->CRCINIT = 0xFFUL;        // Initial value
    NRF_RADIO->CRCPOLY = 0x107UL;       // CRC poly: x^8+x^2^x^1+1
    break ;
  default:
    break ;
  }
}

uint32_t uesb_init(const uesb_config_t *parameters)
{
  if(m_uesb_mainstate != UESB_STATE_UNINITIALIZED) return UESB_ERROR_ALREADY_INITIALIZED;
  
  memcpy(&m_config_local, parameters, sizeof(uesb_config_t));

  NRF_RADIO->POWER = 1;

  NRF_RADIO->INTENCLR    = 0xFFFFFFFF;
  NRF_RADIO->SHORTS      = RADIO_SHORTS_READY_START_Msk | 
                           RADIO_SHORTS_END_DISABLE_Msk |
                           RADIO_SHORTS_ADDRESS_RSSISTART_Msk |
                           RADIO_SHORTS_DISABLED_RSSISTOP_Msk;
  NRF_RADIO->INTENSET    = RADIO_INTENSET_DISABLED_Msk;

  NVIC_SetPriority(RADIO_IRQn, m_config_local.radio_irq_priority);

  m_uesb_mainstate = UESB_STATE_IDLE;

  memset(&m_tx_fifo, 0, sizeof(m_tx_fifo));
  memset(&m_rx_fifo, 0, sizeof(m_rx_fifo));
 
  return UESB_SUCCESS;
}

uint32_t uesb_disable(void)
{
  if (m_uesb_mainstate == UESB_STATE_UNINITIALIZED) {
    return UESB_SUCCESS;
  }
  
  while(m_uesb_mainstate != UESB_STATE_IDLE) { uesb_stop(); }
  m_uesb_mainstate = UESB_STATE_UNINITIALIZED;

  NRF_RADIO->SHORTS = 0;
  
  NRF_RADIO->INTENCLR = 0xFFFFFFFF;
  
  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->EVENTS_ADDRESS = 0;
  NRF_RADIO->EVENTS_PAYLOAD = 0;
  NRF_RADIO->EVENTS_END = 0;
  NRF_RADIO->EVENTS_RSSIEND = 0;
  NRF_RADIO->EVENTS_BCMATCH = 0;;

  NVIC_SetPriority(RADIO_IRQn, 0);
  NVIC_DisableIRQ(RADIO_IRQn);

  return UESB_SUCCESS;
}

uint32_t uesb_prepare_tx_payload(const uesb_address_desc_t *address, const void *data, uint8_t length)
{
  if(m_uesb_mainstate == UESB_STATE_UNINITIALIZED) return UESB_ERROR_NOT_INITIALIZED;
  if(m_tx_fifo.count >= UESB_CORE_TX_FIFO_SIZE) return UESB_ERROR_TX_FIFO_FULL;

  DISABLE_RF_IRQ;
  
  uesb_payload_t *payload = &m_tx_fifo.payload[m_tx_fifo.entry_point++];
  if(m_tx_fifo.entry_point >= UESB_CORE_TX_FIFO_SIZE) m_tx_fifo.entry_point = 0;
  m_tx_fifo.count++;

  payload->length = length;
  memcpy(&payload->address, address, sizeof(uesb_address_desc_t));
  memcpy(payload->data, data, length);
  
  if (m_uesb_mainstate == UESB_STATE_PRX) {
    uesb_stop();
  }
  ENABLE_RF_IRQ;
  
  return UESB_SUCCESS;
}

uint32_t uesb_write_tx_payload(const uesb_address_desc_t *address, const void *data, uint8_t length)
{
  uint32_t error = uesb_prepare_tx_payload(address, data, length);
  
  if (error != UESB_SUCCESS) {
    return error;
  }
  
  return uesb_start();
}

uint32_t uesb_read_rx_payload(uesb_payload_t *payload)
{
  if(m_uesb_mainstate == UESB_STATE_UNINITIALIZED) return UESB_ERROR_NOT_INITIALIZED;
  if(m_rx_fifo.count == 0) return UESB_ERROR_RX_FIFO_EMPTY;

  DISABLE_RF_IRQ;
  memcpy(payload, &m_rx_fifo.payload[m_rx_fifo.exit_point], sizeof(uesb_payload_t));

  if(++m_rx_fifo.exit_point >= UESB_CORE_RX_FIFO_SIZE) m_rx_fifo.exit_point = 0;
  
  m_rx_fifo.count--;
  ENABLE_RF_IRQ;

  uesb_start();

  return UESB_SUCCESS;
}

static void configure_addresses(const uesb_address_desc_t *address) {
  uint8_t prefix = address->address >> 24;

  // Physical addresses
  NRF_RADIO->PREFIX0 = bytewise_bit_swap(prefix);
  NRF_RADIO->BASE0   = bytewise_bit_swap(address->address << 8);

  // Set radio transmission buffer
  NRF_RADIO->FREQUENCY = address->rf_channel;
}

uint32_t uesb_start() {
  if(m_uesb_mainstate != UESB_STATE_IDLE) return UESB_ERROR_NOT_IDLE;

  update_radio_parameters();

  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->EVENTS_ADDRESS = 0;
  NRF_RADIO->EVENTS_PAYLOAD = 0;
  //NRF_RADIO->EVENTS_END = 0;
  //NRF_RADIO->EVENTS_RSSIEND = 0;

  if (m_tx_fifo.count > 0) {
    uesb_payload_t *current_payload = &m_tx_fifo.payload[m_tx_fifo.exit_point];

    // Dequeue fifo
    m_uesb_mainstate       = UESB_STATE_PTX;

    update_rf_payload_format(current_payload->length);
    configure_addresses(&current_payload->address);

    NRF_RADIO->TXADDRESS   = 0;
    NRF_RADIO->PACKETPTR   = (uint32_t)&current_payload->data;
    
    NRF_RADIO->TASKS_TXEN  = 1;
  } else if (m_rx_fifo.count < UESB_CORE_RX_FIFO_SIZE) {
    // Read to the next spot in the payload
    uesb_payload_t *m_rx_payload = &m_rx_fifo.payload[m_rx_fifo.entry_point];

    m_uesb_mainstate       = UESB_STATE_PRX;
    
    update_rf_payload_format(m_config_local.rx_address.payload_length);
    configure_addresses(&m_config_local.rx_address);
    
    // We will only be listening to base0
    NRF_RADIO->RXADDRESSES = 1;
    NRF_RADIO->PACKETPTR = (uint32_t)&m_rx_payload->data;

    NRF_RADIO->TASKS_RXEN  = 1;
  } else {
    m_uesb_mainstate = UESB_STATE_IDLE;
  }

  NVIC_ClearPendingIRQ(RADIO_IRQn);
  ENABLE_RF_IRQ;

  return UESB_SUCCESS;
}

uint32_t uesb_stop(void)
{
  DISABLE_RF_IRQ;
  NRF_RADIO->TASKS_DISABLE = 1;
  while(!NRF_RADIO->EVENTS_DISABLED) ;
  NRF_RADIO->EVENTS_DISABLED = 0;
  
  m_uesb_mainstate = UESB_STATE_IDLE;
  ENABLE_RF_IRQ;

  return UESB_SUCCESS;
}

uint32_t uesb_set_rx_address(const uesb_address_desc_t *addr)
{
  memcpy(&m_config_local.rx_address, addr, sizeof(uesb_address_desc_t));
  
  return UESB_SUCCESS;
}

extern "C" void RADIO_IRQHandler()
{   
  // Clear event
  NRF_RADIO->EVENTS_DISABLED = 0;

  switch(m_uesb_mainstate)
  {
  case UESB_STATE_PRX:
    // CRC Passed
    if (NRF_RADIO->CRCSTATUS != 0)
    {
      uesb_payload_t *m_rx_payload = &m_rx_fifo.payload[m_rx_fifo.entry_point];
      
      if(++m_rx_fifo.entry_point >= UESB_CORE_RX_FIFO_SIZE) m_rx_fifo.entry_point = 0;
      m_rx_fifo.count++;
    
      m_rx_payload->length = m_config_local.rx_address.payload_length;
      m_rx_payload->rssi = NRF_RADIO->RSSISAMPLE;
      m_rx_payload->address.rf_channel = NRF_RADIO->FREQUENCY;
      m_rx_payload->address.address = bytewise_bit_swap((NRF_RADIO->PREFIX0 << 24) | (NRF_RADIO->BASE0 >> 8));

      uesb_event_handler(UESB_INT_RX_DR_MSK);
    }
    
    break ;
  case UESB_STATE_PTX:
    // Naively assume we don't need an ACK
    if (m_tx_fifo.count > 0)
    {
      m_tx_fifo.count--;
      m_tx_fifo.exit_point++;
      if(m_tx_fifo.exit_point >= UESB_CORE_TX_FIFO_SIZE) m_tx_fifo.exit_point = 0;
    }

    uesb_event_handler(UESB_INT_TX_SUCCESS_MSK);
    break ;
  default:
    break ;
  }
  
  m_uesb_mainstate = UESB_STATE_IDLE;
  uesb_start();
}
