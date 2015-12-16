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

// TX FIFO
static uesb_payload_fifo_t      m_tx_fifo;

// RX FIFO
static uesb_payload_fifo_t      m_rx_fifo;
static uint8_t                  m_rx_buffer[UESB_CORE_MAX_PAYLOAD_LENGTH+2];

// Run time variables
static volatile uint32_t        m_interrupt_flags       = 0;
static uint32_t                 m_pid[UESB_MAX_PIPES];
static volatile uint8_t         m_last_rx_packet_pid    = UESB_PID_RESET_VALUE;
static volatile uint32_t        m_last_rx_packet_crc    = 0xFFFFFFFF;

static uesb_mainstate_t         m_uesb_mainstate        = UESB_STATE_UNINITIALIZED;

// Macros
#define                         DISABLE_RF_IRQ      NVIC_DisableIRQ(RADIO_IRQn)
#define                         ENABLE_RF_IRQ       NVIC_EnableIRQ(RADIO_IRQn)

static void update_rf_payload_format(uint32_t payload_length)
{
  NRF_RADIO->PCNF0 =  (1 << RADIO_PCNF0_S0LEN_Pos) | 
                      (0 << RADIO_PCNF0_LFLEN_Pos) | 
                      (1 << RADIO_PCNF0_S1LEN_Pos);

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

static void initialize_fifos()
{
  memset(&m_tx_fifo, 0, sizeof(m_tx_fifo));
  memset(&m_rx_fifo, 0, sizeof(m_rx_fifo));
}

static void tx_fifo_remove_last()
{
  if(m_tx_fifo.count > 0)
  {
    DISABLE_RF_IRQ;
    m_tx_fifo.count--;
    m_tx_fifo.exit_point++;
    if(m_tx_fifo.exit_point >= UESB_CORE_TX_FIFO_SIZE) m_tx_fifo.exit_point = 0;
    ENABLE_RF_IRQ;
  }
}

uint32_t uesb_init(const uesb_config_t *parameters)
{
  if(m_uesb_mainstate != UESB_STATE_UNINITIALIZED) return UESB_ERROR_ALREADY_INITIALIZED;
  memcpy(&m_config_local, parameters, sizeof(uesb_config_t));

  m_interrupt_flags    = 0;
  memset(m_pid, 0, sizeof(m_pid));
  m_last_rx_packet_pid = 0xFF;
  m_last_rx_packet_crc = 0xFFFFFFFF;

  update_radio_parameters();

  NRF_RADIO->POWER = 1;

  NRF_RADIO->INTENCLR    = 0xFFFFFFFF;
  NRF_RADIO->SHORTS      = RADIO_SHORTS_READY_START_Msk | 
                           RADIO_SHORTS_END_DISABLE_Msk |
                           RADIO_SHORTS_ADDRESS_RSSISTART_Msk |
                           RADIO_SHORTS_DISABLED_RSSISTOP_Msk;
  NRF_RADIO->INTENSET    = RADIO_INTENSET_DISABLED_Msk;

  initialize_fifos();

  NVIC_SetPriority(RADIO_IRQn, m_config_local.radio_irq_priority & 0x03);

  m_uesb_mainstate = UESB_STATE_IDLE;

  return UESB_SUCCESS;
}

uint32_t uesb_disable(void)
{
  if(m_uesb_mainstate != UESB_STATE_IDLE) return UESB_ERROR_NOT_IDLE;
  m_uesb_mainstate = UESB_STATE_UNINITIALIZED;

  NRF_RADIO->SHORTS = 0;
  NRF_RADIO->INTENCLR = 0xFFFFFFFF;
  NRF_RADIO->EVENTS_DISABLED = 0;

  NRF_RADIO->POWER = 0;

  return UESB_SUCCESS;
}

uint32_t uesb_write_tx_payload(const uesb_address_desc_t *address, uint8_t pipe, const void *data, uint8_t length)
{
  if(m_uesb_mainstate == UESB_STATE_UNINITIALIZED) return UESB_ERROR_NOT_INITIALIZED;
  if(m_tx_fifo.count >= UESB_CORE_TX_FIFO_SIZE) return UESB_ERROR_TX_FIFO_FULL;

  DISABLE_RF_IRQ;
  
  uesb_payload_t *payload = &m_tx_fifo.payload[m_tx_fifo.entry_point++];
  if(m_tx_fifo.entry_point >= UESB_CORE_TX_FIFO_SIZE) m_tx_fifo.entry_point = 0;
  m_tx_fifo.count++;

  payload->pipe = pipe;
  payload->length = length;
  memcpy(&payload->address, address, sizeof(uesb_address_desc_t));
  memcpy(payload->data, data, length);
  
  if (m_uesb_mainstate == UESB_STATE_PRX) {
    uesb_stop();
  }
  ENABLE_RF_IRQ;

  uesb_start();

  return UESB_SUCCESS;
}

uint32_t uesb_read_rx_payload(uesb_payload_t *payload)
{
  if(m_uesb_mainstate == UESB_STATE_UNINITIALIZED) return UESB_ERROR_NOT_INITIALIZED;
  if(m_rx_fifo.count == 0) return UESB_ERROR_RX_FIFO_EMPTY;

  DISABLE_RF_IRQ;
  payload->length = m_rx_fifo.payload[m_rx_fifo.exit_point].length;
  payload->pipe   = m_rx_fifo.payload[m_rx_fifo.exit_point].pipe;
  payload->rssi   = m_rx_fifo.payload[m_rx_fifo.exit_point].rssi;

  memcpy(payload->data, m_rx_fifo.payload[m_rx_fifo.exit_point].data, payload->length);

  if(++m_rx_fifo.exit_point >= UESB_CORE_RX_FIFO_SIZE) m_rx_fifo.exit_point = 0;
  
  m_rx_fifo.count--;
  ENABLE_RF_IRQ;

  uesb_start();

  return UESB_SUCCESS;
}

static void configure_addresses(const uesb_address_desc_t *address) {
  // Physical addresses
  NRF_RADIO->PREFIX0 = bytewise_bit_swap(address->prefix[3] << 24 |
                                         address->prefix[2] << 16 |
                                         address->prefix[1] << 8 |
                                         address->prefix[0]);
  NRF_RADIO->PREFIX1 = bytewise_bit_swap(address->prefix[7] << 24 |
                                         address->prefix[6] << 16 |
                                         address->prefix[5] << 8 |
                                         address->prefix[4]);

  NRF_RADIO->BASE0   = bytewise_bit_swap(address->base0);
  NRF_RADIO->BASE1   = bytewise_bit_swap(address->base1);

  // Set radio transmission buffer
  NRF_RADIO->FREQUENCY = address->rf_channel;
}

uint32_t uesb_start() {
  if(m_uesb_mainstate != UESB_STATE_IDLE) return UESB_ERROR_NOT_IDLE;
    
  update_radio_parameters();

  uesb_payload_t *current_payload = &m_tx_fifo.payload[m_tx_fifo.exit_point];

  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->EVENTS_ADDRESS = 0;
  NRF_RADIO->EVENTS_PAYLOAD = 0;
  //NRF_RADIO->EVENTS_END = 0;
  //NRF_RADIO->EVENTS_RSSIEND = 0;

  if (m_tx_fifo.count > 0) {
    // Dequeue fifo
    uint8_t pipe = current_payload->pipe;
    
    if (pipe >= UESB_MAX_PIPES) {
      return UESB_ERROR_INVALID_PARAMETERS;
    }

    m_pid[pipe] = (m_pid[pipe] + 1) % 4;
  
    current_payload->pid = 0xCC | m_pid[pipe];
    current_payload->null = 0;

    m_uesb_mainstate       = UESB_STATE_PTX;

    update_rf_payload_format(m_config_local.payload_length); // NOTE: Ignore payload length for now
    configure_addresses(&current_payload->address);

    NRF_RADIO->TXADDRESS   = current_payload->pipe;
    NRF_RADIO->PACKETPTR   = (uint32_t)&current_payload->pid;
    
    NRF_RADIO->TASKS_TXEN  = 1;

  } else if (m_rx_fifo.count < UESB_CORE_RX_FIFO_SIZE) {
    m_uesb_mainstate       = UESB_STATE_PRX;
    
    update_rf_payload_format(m_config_local.payload_length);
    configure_addresses(&m_config_local.rx_address);
    
    NRF_RADIO->RXADDRESSES = m_config_local.rx_pipes_enabled;
    NRF_RADIO->PACKETPTR = (uint32_t)&m_rx_buffer;
  
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

uint32_t uesb_flush_tx(void)
{
  if(m_uesb_mainstate != UESB_STATE_IDLE) return UESB_ERROR_NOT_IDLE;
  DISABLE_RF_IRQ;
  m_tx_fifo.count = 0;
  m_tx_fifo.entry_point = m_tx_fifo.exit_point = 0;
  ENABLE_RF_IRQ;

  return UESB_SUCCESS;
}

uint32_t uesb_flush_rx(void)
{
  DISABLE_RF_IRQ;
  m_rx_fifo.count = 0;
  m_last_rx_packet_pid = UESB_PID_RESET_VALUE;
  m_rx_fifo.entry_point = m_rx_fifo.exit_point = 0;
  ENABLE_RF_IRQ;

  return UESB_SUCCESS;
}

uint32_t uesb_get_clear_interrupts(void)
{
  DISABLE_RF_IRQ;
  uint32_t interrupts = m_interrupt_flags;
  m_interrupt_flags = 0;
  ENABLE_RF_IRQ;

  return interrupts;
}

uint32_t uesb_set_rx_address(const uesb_address_desc_t *addr)
{
  memcpy(&m_config_local.rx_address, addr, sizeof(uesb_address_desc_t));
  
  return UESB_SUCCESS;
}

uint32_t uesb_set_tx_power(uesb_tx_power_t tx_output_power)
{
  if(m_uesb_mainstate != UESB_STATE_IDLE) return UESB_ERROR_NOT_IDLE;
  
  if ( m_config_local.tx_output_power == tx_output_power ) return UESB_SUCCESS;
  m_config_local.tx_output_power = tx_output_power;
  
  update_radio_parameters();
  
  return UESB_SUCCESS;
}

extern "C" void RADIO_IRQHandler()
{   
  if(!NRF_RADIO->EVENTS_DISABLED || (~NRF_RADIO->INTENSET & RADIO_INTENSET_DISABLED_Msk)) {
    return ;
  }

  NRF_RADIO->EVENTS_DISABLED = 0;

  switch(m_uesb_mainstate)
  {
  case UESB_STATE_PRX:
    // CRC Passed, not a re-transmit
    if(NRF_RADIO->CRCSTATUS != 0 && (NRF_RADIO->RXCRC != m_last_rx_packet_crc || m_rx_buffer[1] >> 1 != m_last_rx_packet_pid))
    {
      uesb_payload_t *m_rx_payload = &m_rx_fifo.payload[m_rx_fifo.entry_point];
      
      m_interrupt_flags |= UESB_INT_RX_DR_MSK;

      if(++m_rx_fifo.entry_point >= UESB_CORE_RX_FIFO_SIZE) m_rx_fifo.entry_point = 0;
      m_rx_fifo.count++;

      memcpy(&m_rx_payload->pid, m_rx_buffer, sizeof(m_rx_buffer));
    
      m_rx_payload->length = m_config_local.payload_length;
      m_rx_payload->pipe = NRF_RADIO->RXMATCH;
      m_rx_payload->rssi = NRF_RADIO->RSSISAMPLE;
    
      m_last_rx_packet_pid = m_rx_payload->null >> 1;
      m_last_rx_packet_crc = NRF_RADIO->RXCRC;
    }
    
    break ;
  case UESB_STATE_PTX:
    // Naively assume we don't need an ACK
    m_interrupt_flags |= UESB_INT_TX_SUCCESS_MSK;
    tx_fifo_remove_last();

    break ;
  default:
    break ;
  }
  
  m_uesb_mainstate = UESB_STATE_IDLE;
  uesb_event_handler();
  uesb_start();
}
