/**
 ****************************************************************************************
 *
 * @file customer_prod.h
 *
 * @brief Customer production header file.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */
 
#ifndef _CUSTOMER_PROD_H_
#define _CUSTOMER_PROD_H_

#include <stdint.h>
#if HAS_AUDIO
    #include "app_audio439.h"
#endif

//HCI Basic Command Complete Event packet length
#define HCI_CCEVT_NUM_CMD_PACKETS   (0x01)

enum
{
    ///NO_TEST_RUNNING
    STATE_IDLE         						 = 0x00,
    ///START_TX_TEST
    STATE_START_TX,   						//1
    ///START_RX_TEST
    STATE_START_RX,							//2
	///DIRECT_TX_TEST					
	STATE_DIRECT_TX_TEST,					//3 activated via default hci command
	///DIRECT_RX_TEST					
	STATE_DIRECT_RX_TEST,					//4 activated via default hci command
	///CONTINUE_TX
	STATE_START_CONTINUE_TX,				//5
	///UNMODULATED_ON						
	STATE_UNMODULATED_ON,					//6
};

enum
{
    ///ES2
    ES2           = 0x20,
    ///ES3
    ES3           = 0x30,
	///ES4		  
	ES4		  	  = 0x40,
	///ES5		  
	ES5			  = 0x50,
    
};

extern volatile uint8_t test_state;
extern volatile uint8_t test_data_pattern;
extern volatile uint8_t test_freq;
extern volatile uint8_t test_data_len;

extern volatile uint16_t text_tx_nr_of_packets;
extern volatile uint16_t test_tx_packet_nr;

extern volatile uint16_t test_rx_packet_nr;
extern volatile uint16_t test_rx_packet_nr_syncerr;
extern volatile uint16_t test_rx_packet_nr_crcerr;
extern volatile uint16_t test_rx_irq_cnt;
extern volatile uint16_t rx_test_rssi_1;
extern volatile uint16_t rx_test_rssi_2;	

extern volatile uint16_t  rf_calibration_request_cbt;
//additional HCI commands for production test  //START WITH 0x40xx


#define HCI_UNMODULATED_ON_CMD_OPCODE               (0x4010)
/* Unmodulated TX/RX
 0x01   is HCI_CMD_MSG_TYPE, SOH , type ALT-1 followed by CTRL-A
 0x10 
 0x40
 0x02   = length of parameters is 2 for this command
 MODE TX/RX, T/R
 FREQ  
*/
//RETURN MESSAGE	
struct msg_unmodulated_cfm
{
	/// Status of the command reception
	uint8_t 	packet_type;
	uint8_t 	event_code;
	uint8_t		length;
	uint8_t		param0;
	uint16_t 	param_opcode;
};

#define HCI_START_PROD_RX_TEST_CMD_OPCODE           (0x4020)
struct msg_start_prod_rx_cfm
{
	/// Status of the command reception
	uint8_t 	packet_type;
	uint8_t 	event_code;
	uint8_t		length;
	uint8_t		param0;
	uint16_t 	param_opcode;
};

#define HCI_LE_END_PROD_RX_TEST_CMD_OPCODE          (0x4030)
struct msg_rx_stop_send_info_back
{
	/// Status of the command reception
	uint8_t 	packet_type;
	uint8_t 	event_code;
	uint8_t		length;
	uint8_t		param0;
	uint16_t 	param_opcode;
	uint16_t	nb_packet_received;
	uint16_t  nb_syncerr;
	uint16_t	nb_crc_error;
	uint16_t	rx_test_rssi;
};


//HCI_LE_TX_TEST_CMD_OPCODE with length 5 is added. Lenght 3 is default.
//length is 2 bytes more because of 16 bits value for nr_of_packages.
//so this msg is confirmation of the message, so packages are not transmitted yet
//after packages are transmitted there's an additional message 'HCI_TX_PACKAGES_SEND_READY'.
struct msg_tx_send_packages_cfm
{
	/// Status of the command reception
	uint8_t 	packet_type;
	uint8_t 	event_code;
	uint8_t		length;
	uint8_t		param0;
	uint16_t 	param_opcode;
};

#define HCI_TX_PACKAGES_SEND_READY                  (0x4040)
struct msg_tx_packages_send_ready
{
	/// Status of the command reception
	uint8_t 	packet_type;
	uint8_t 	event_code;
	uint8_t		length;
	uint8_t		param0;
	uint16_t 	param_opcode;
};

#define HCI_TX_START_CONTINUE_TEST_CMD_OPCODE       (0x4050)
struct msg_tx_start_continue_test_cfm
{
	/// Status of the command reception
	uint8_t 	packet_type;
	uint8_t 	event_code;
	uint8_t		length;
	uint8_t		param0;
	uint16_t 	param_opcode;
};

#define HCI_TX_END_CONTINUE_TEST_CMD_OPCODE         (0x4060)
struct msg_tx_end_continue_test_cfm
{
	/// Status of the command reception
	uint8_t 	packet_type;
	uint8_t 	event_code;
	uint8_t		length;
	uint8_t		param0;
	uint16_t 	param_opcode;
};

#define HCI_SLEEP_TEST_CMD_OPCODE                   (0x4070)
struct msg_sleep_test_cfm
{
	/// Status of the command reception
	uint8_t 	packet_type;
	uint8_t 	event_code;
	uint8_t		length;
	uint8_t		param0;
	uint16_t 	param_opcode;
};

#define HCI_XTAL_TRIM_CMD_OPCODE                    (0x4080)
struct msg_xtal_trim_cfm
{
	/// Status of the command reception
	uint8_t 	packet_type;
	uint8_t 	event_code;
	uint8_t		length;
	uint8_t   param0;
	uint16_t 	param_opcode;
	uint16_t 	xtrim_val;
};

#define HCI_OTP_RW_CMD_OPCODE                       (0x4090)
struct msg_otp_rw_cfm
{
	/// Status of the command reception
	uint8_t 	packet_type;
	uint8_t 	event_code;
	uint8_t		length;
	uint8_t   param0;
	uint16_t 	param_opcode;
	uint8_t   action;
	uint8_t 	otp_val[6];
};

#define HCI_OTP_READ_CMD_OPCODE                     (0x40A0)
#define HCI_OTP_READ_CMD_STATUS_SUCCESS             (0x00)
struct msg_otp_read_cfm
{
	uint8_t  packet_type;
	uint8_t  event_code;
	uint8_t  length;
	uint8_t  param0;
	uint16_t param_opcode;
	uint8_t  status;
	uint8_t  word_count;	
	uint32_t words[];
};

#define HCI_OTP_WRITE_CMD_OPCODE                    (0x40B0)
#define HCI_OTP_WRITE_CMD_STATUS_SUCCESS            (0x00)
struct msg_otp_write_cfm
{
	uint8_t  packet_type;
	uint8_t  event_code;
	uint8_t  length;
	uint8_t  param0;
	uint16_t param_opcode;
	uint8_t  status;
	uint8_t  word_count;	
};

#define HCI_REGISTER_RW_CMD_OPCODE                  (0x40C0)
struct msg_register_rw_cfm
{
	uint8_t  packet_type;
	uint8_t  event_code;
	uint8_t  length;
	uint8_t  param0;
	uint16_t param_opcode;
	uint8_t  operation;
	uint8_t  reserved;
	uint8_t  data[4];
};

#if HAS_AUDIO
#pragma pack(1)

enum op_audio_test {
    CMD__AUDIO_TEST_OP_START = 1,
    CMD__AUDIO_TEST_OP_STOP
};

struct param_audio_test {
    uint8_t     operation;
    uint16_t    audio_packets;
};

#define HCI_AUDIO_TEST_CMD_OPCODE                   (0x40C8)

enum audio_status {
    AUDIO_WAS_ACTIVE    = 1,
    AUDIO_WAS_INACTIVE,
    AUDIO_IMA_PACKET,
    AUDIO_ALAW_PACKET,
    AUDIO_RAW_PACKET,
    AUDIO_UNKNOWN_OPERATION
};

struct audio_start_results {
    uint8_t     status;
};

#define AUDIO_BLOCKS 8
struct audio_packet {
    uint8_t     type;
    uint32_t    number;
    uint8_t     encoded_bytes[AUDIO_BLOCKS * AUDIO_ENC_SIZE];
};

struct audio_stop_results {
    uint8_t	    status;
    uint32_t    packets;
};

struct msg_audio_test_cfm {
    uint8_t     packet_type;
    uint8_t     event_code;
    uint8_t     length;
    uint8_t     num_hci_cmd_packets;
    uint16_t    param_opcode;
    union {
        struct audio_start_results  audio_start;
        struct audio_packet         audio_packet;
        struct audio_stop_results   audio_stop;
        uint8_t                     status;
    }results;
};

#pragma pack()
#endif

#define HCI_CUSTOM_ACTION_CMD_OPCODE                (0x40D0)
struct msg_custom_action_cfm 
 {
    uint8_t     packet_type;
    uint8_t     event_code;
    uint8_t     length;
    uint8_t     param0;
    uint16_t    param_opcode;
    uint16_t    data;
};

#define HCI_PLAY_TONE_CMD_OPCODE                    (0x40D3)
struct msg_play_tone_cfm
{
    uint8_t     packet_type;
    uint8_t     event_code;
    uint8_t     length;
    uint8_t     param0;
    uint16_t    param_opcode;
    uint8_t     data;
};

#define HCI_RDTESTER_CMD_OPCODE                     (0x40E0)
typedef enum __rdtester_op 
{
    RDTESTER_INIT               = 0,
    RDTESTER_UART_CONNECT       = 1,
    RDTESTER_UART_LOOPBACK      = 2,
    RDTESTER_VBAT_CNTRL         = 3,
    RDTESTER_VPP_CNTRL          = 4,
    RDTESTER_RST_PULSE          = 5,
    RDTESTER_UART_PULSE         = 6,
    RDTESTER_XTAL_PULSE         = 7,
    RDTESTER_PULSE_WIDTH        = 8
}_rdtester_op;

struct msg_rdtest_cfm
{
	uint8_t  	packet_type;
	uint8_t  	event_code;
	uint8_t  	length;
	uint8_t  	param0;
	uint16_t	param_opcode;
};

#define HCI_FIRMWARE_VERSION_CMD_OPCODE             (0x40F0)
struct msg_fw_version_action
{
	uint8_t  	packet_type;
	uint8_t  	event_code;
	uint8_t  	length;
	uint8_t  	param0;
	uint16_t	param_opcode;
	uint8_t		ble_ver_len;
	uint8_t		app_ver_len;
	char		ble_ver[32];
	char		app_ver[32];
};
#define HCI_CHANGE_UART_PINS_CMD_OPCODE             (0x40FD)
struct msg_uart_pins_cfm {
	uint8_t  	packet_type;
	uint8_t  	event_code;
	uint8_t  	length;
	uint8_t  	param0;
	uint16_t	param_opcode;
    uint16_t	data;
};

#define SENSOR_TEST_CMD_OPCODE                      (0x40F1)
struct sensor_test_cfm
{
    uint8_t     packet_type;
    uint8_t     event_code;
    uint8_t     length;
    uint8_t     param0;
    uint16_t    param_opcode;
    uint16_t    data;
};


void set_state_stop(void);
void set_state_start_tx(void);
void set_state_start_rx(void);
void set_state_start_continue_tx(void);
void hci_start_prod_rx_test(uint8_t*);
void hci_tx_send_nb_packages(uint8_t*);
void hci_tx_start_continue_test_mode(uint8_t*);
void hci_unmodulated_cmd(uint8_t*);
void hci_end_tx_continuous_test_cmd(void);
void hci_end_rx_prod_test_cmd(void);
void hci_sleep_test_cmd(uint8_t*);
void hci_xtal_trim_cmd(uint8_t*);
void hci_otp_rw_cmd(uint8_t*);
void hci_otp_read_cmd(uint8_t*);
void hci_otp_write_cmd(uint8_t*);
void hci_register_rw_cmd(uint8_t*);
#if HAS_AUDIO
void hci_audio_test_cmd(struct param_audio_test*);
#endif
void hci_play_tone_cmd(uint8_t *ptr_data);
void hci_custom_action_cmd(uint8_t*);
void hci_change_uart_pins_cmd(uint8_t *ptr_data);
void hci_sensor_test_cmd(uint8_t *ptr_data);
void hci_rdtester_cmd(uint8_t*);
void hci_fw_version_cmd(void);

#endif // _CUSTOMER_PROD_H_
