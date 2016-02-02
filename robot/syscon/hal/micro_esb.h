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

#ifndef __MICRO_ESB_H
#define __MICRO_ESB_H

#include <stdint.h>
#include "nrf.h"
#include "nrf51.h"
#include "nrf51_bitfields.h"

#pragma anon_unions

// Hard coded parameters - change if necessary
#define     UESB_CORE_MAX_PAYLOAD_LENGTH    32
#define     UESB_CORE_TX_FIFO_SIZE          8
#define     UESB_CORE_RX_FIFO_SIZE          8

#define     UESB_MAX_CHANNEL                125
#define     UESB_MAX_PIPES                  8

// Interrupt flags
#define     UESB_INT_TX_SUCCESS_MSK         0x01
#define     UESB_INT_RX_DR_MSK              0x02

#define     UESB_PID_RESET_VALUE            0xFF

// General success code
#define     UESB_SUCCESS                    0x0000

// State related errors
#define     UESB_ERROR_NOT_INITIALIZED      0x0101
#define     UESB_ERROR_ALREADY_INITIALIZED  0x0102
#define     UESB_ERROR_NOT_IDLE             0x0103
#define     UESB_ERROR_NOT_IN_RX_MODE       0x0104

// Invalid parameter errors
#define     UESB_ERROR_INVALID_PARAMETERS   0x0200
#define     UESB_ERROR_DYN_ACK_NOT_ENABLED  0x0201

// FIFO related errors
#define     UESB_ERROR_TX_FIFO_FULL         0x0301
#define     UESB_ERROR_TX_FIFO_EMPTY        0x0302
#define     UESB_ERROR_RX_FIFO_FULL         0x0304
#define     UESB_ERROR_RX_FIFO_EMPTY        0x0303

// Configuration parameter definitions
enum uesb_bitrate_t {
  UESB_BITRATE_2MBPS = RADIO_MODE_MODE_Nrf_2Mbit,
  UESB_BITRATE_1MBPS = RADIO_MODE_MODE_Nrf_1Mbit,
  UESB_BITRATE_250KBPS = RADIO_MODE_MODE_Nrf_250Kbit
};

enum uesb_crc_t {
  UESB_CRC_16BIT = RADIO_CRCCNF_LEN_Two,
  UESB_CRC_8BIT  = RADIO_CRCCNF_LEN_One,
  UESB_CRC_OFF   = RADIO_CRCCNF_LEN_Disabled
};

enum uesb_tx_power_t {
  UESB_TX_POWER_4DBM     = RADIO_TXPOWER_TXPOWER_Pos4dBm,
  UESB_TX_POWER_0DBM     = RADIO_TXPOWER_TXPOWER_0dBm,
  UESB_TX_POWER_NEG4DBM  = RADIO_TXPOWER_TXPOWER_Neg4dBm,
  UESB_TX_POWER_NEG8DBM  = RADIO_TXPOWER_TXPOWER_Neg8dBm,
  UESB_TX_POWER_NEG12DBM = RADIO_TXPOWER_TXPOWER_Neg12dBm,
  UESB_TX_POWER_NEG16DBM = RADIO_TXPOWER_TXPOWER_Neg16dBm,
  UESB_TX_POWER_NEG20DBM = RADIO_TXPOWER_TXPOWER_Neg20dBm,
  UESB_TX_POWER_NEG30DBM = RADIO_TXPOWER_TXPOWER_Neg30dBm
};

// Internal state definition
enum uesb_mainstate_t {
  UESB_STATE_UNINITIALIZED,
  UESB_STATE_IDLE,
  UESB_STATE_PTX,
  UESB_STATE_PRX
};

// Main UESB configuration struct, contains all radio parameters
struct uesb_address_desc_t
{
  uint8_t                 rf_channel;
  uint32_t                base0;
  uint32_t                base1;
  uint8_t                 prefix[8];
  uint8_t                 rx_pipes_enabled;
};

struct uesb_config_t
{
  // General RF parameters
  uesb_bitrate_t          bitrate;
  uesb_crc_t              crc;
  uesb_tx_power_t         tx_output_power;
  uint8_t                 payload_length;
  uint8_t                 rf_addr_length;

  uint8_t                 radio_irq_priority;

  uesb_address_desc_t     rx_address;
};

struct uesb_payload_t
{
  // This is information regarding the state of the radio
  uint8_t length;
  uint8_t pipe;
  uesb_address_desc_t address;
  int8_t  rssi;

  // This is the payload data
  uint8_t pid;
  uint8_t null;
  uint8_t data[UESB_CORE_MAX_PAYLOAD_LENGTH];
};

struct uesb_payload_fifo_t
{
  uesb_payload_t  payload[UESB_CORE_TX_FIFO_SIZE];
  uint32_t        entry_point;
  uint32_t        exit_point;
  uint32_t        count;
};

uint32_t uesb_init(const uesb_config_t *parameters);
uint32_t uesb_disable(void);
bool     uesb_is_idle(void);
uint32_t uesb_write_tx_payload(const uesb_address_desc_t *address, uint8_t pipe, const void *payload, uint8_t length);
uint32_t uesb_read_rx_payload(uesb_payload_t *payload);
uint32_t uesb_start(void);
uint32_t uesb_stop(void);
uint32_t uesb_flush_tx(void);
uint32_t uesb_flush_rx(void);
uint32_t uesb_get_clear_interrupts(void);
uint32_t uesb_set_rx_address(const uesb_address_desc_t *addr);
uint32_t uesb_set_tx_power(uesb_tx_power_t tx_output_power);
extern "C" void uesb_event_handler(void);

#endif
