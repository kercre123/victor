/**
 ****************************************************************************************
 *
 * @file host_hci.h
 *
 * @brief Connection Manager HCI library header file.
 *
 * Copyright (C) 2013. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _HOST_HCI_H_
#define _HOST_HCI_H_

#include <stdint.h>

#include "stdbool.h"



typedef struct {
  unsigned short opcode;
  unsigned char length;
} hci_cmd_header_t;

typedef struct {
  unsigned short opcode;
  unsigned char length;
  unsigned char parameters[1];
} hci_cmd_t;


typedef struct {
  unsigned char event;
  unsigned char length;
} hci_evt_header_t;

typedef struct {
  unsigned char event;
  unsigned char length;
  unsigned char parameters[1];
} hci_evt_t;

// Opcodes for custom HCI commands

#define HCI_UNMODULATED_ON_CMD_OPCODE	(0x4010) 	
#define HCI_START_PROD_RX_TEST_CMD_OPCODE 	(0x4020)	
#define HCI_LE_END_PROD_RX_TEST_CMD_OPCODE 	(0x4030)	
#define HCI_TX_CONTINUE_TEST_CMD_OPCODE 	(0x4050)	
#define HCI_TX_END_CONTINUE_TEST_CMD_OPCODE  	(0x4060)	
#define HCI_REGISTER_RW_CMD_OPCODE              (0x40C0)

// otp command operations
#define CMD__OTP_OP_RD_XTRIM  0x00 // read XTAL16M
#define CMD__OTP_OP_WR_XTRIM  0x01 // write XTAL16M
#define CMD__OTP_OP_RD_BDADDR 0x02 // read BDADDR
#define CMD__OTP_OP_WR_BDADDR 0x03 // write BDADDR
#define CMD__OTP_OP_RE_XTRIM  0x04 // read enable bit XTAL16M
#define CMD__OTP_OP_WE_XTRIM  0x05 // write enable bit XTAL16M

// register read/write command operations
#define CMD__REGISTER_RW_OP_READ_REG32   (0)
#define CMD__REGISTER_RW_OP_WRITE_REG32  (1)
#define CMD__REGISTER_RW_OP_READ_REG16   (2)
#define CMD__REGISTER_RW_OP_WRITE_REG16  (3)

hci_evt_t *hci_recv_event_wait(unsigned int millis);
void handle_hci_event( hci_evt_t * evt);


/* HCI TEST MODE commands*/
bool __stdcall hci_reset();
bool __stdcall hci_tx_test(uint8_t frequency, uint8_t length, uint8_t payload);
bool __stdcall hci_rx_test(uint8_t frequency);
bool __stdcall hci_test_end();

bool __stdcall hci_dialog_tx_test(uint8_t frequency, uint8_t length, uint8_t payload, uint16_t number_of_packets);
bool __stdcall hci_dialog_rx_readback_test(uint8_t frequency);
bool __stdcall hci_dialog_rx_readback_test_end();
bool __stdcall hci_dialog_unmodulated_rx_tx(uint8_t mode, uint8_t frequency);
bool __stdcall hci_dialog_tx_continuous_start(uint8_t frequency, uint8_t payload_type);
bool __stdcall hci_dialog_tx_continuous_end();
bool __stdcall hci_dialog_sleep(uint8_t sleep_type, uint8_t minutes, uint8_t seconds);
bool __stdcall hci_dialog_xtal_trimming(uint8_t operation, uint16_t trim_value_or_delta);

bool __stdcall hci_dialog_otp_rd_xtrim(void);
bool __stdcall hci_dialog_otp_wr_xtrim (uint16_t trim_value);
bool __stdcall hci_dialog_otp_rd_bdaddr(void);
bool __stdcall hci_dialog_otp_wr_bdaddr(uint8_t bdaddr[6]);
bool __stdcall hci_dialog_otp_re_xtrim(void);
bool __stdcall hci_dialog_otp_we_xtrim(void);

bool __stdcall hci_dialog_otp_read(uint16_t otp_address, uint8_t word_count);
bool __stdcall hci_dialog_otp_write(uint16_t otp_address, uint32_t *words, uint8_t word_count);

bool __stdcall hci_dialog_read_reg32(uint32_t reg_addr);
bool __stdcall hci_dialog_write_reg32(uint32_t reg_addr, uint32_t value);
bool __stdcall hci_dialog_read_reg16(uint32_t reg_addr);
bool __stdcall hci_dialog_write_reg16(uint32_t reg_addr, uint16_t value);

#endif //_HOST_HCI_H_