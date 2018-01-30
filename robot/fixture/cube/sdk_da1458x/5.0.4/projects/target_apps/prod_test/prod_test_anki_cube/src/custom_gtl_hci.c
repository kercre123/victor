/**
 ****************************************************************************************
 *
 * @file gtl_hci.c
 *
 * @brief HCI part of the Generic Transport Layer
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup GTL
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"      // SW configuration

#if (GTL_ITF && BLE_EMB_PRESENT)

#include "co_bt.h"            // BT standard definitions
#include "co_endian.h"
#include "co_utils.h"
#include "co_error.h"

#include "gtl.h"
#include "gtl_env.h"
#include "gtl_hci.h"
#include "gtl_eif.h"
#include "gtl_task.h"

#include "llm_task.h"

#include "arch.h"
#include "pll_vcocal_lut.h"
#include "rf_580.h"

#include "customer_prod.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * STRUCTURES
 ****************************************************************************************
 */


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
//extern uint16_t last_temp_count; /// temperature counter


/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */


/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void gtl_hci_rx_header(void)
{
		typedef void (*my_function)( void);
    my_function PtrFunc;
    PtrFunc = (my_function)(jump_table_struct[gtl_hci_rx_header_func_pos]);
    PtrFunc();
}

void gtl_hci_rx_header_func(void)
{
    uint16_t msgid = 0;
    uint16_t dest_id = TASK_LLM;
    uint16_t opcode = co_btohs(co_read16p(&gtl_env.curr_hdr_buff[0]));
    uint8_t length_rx = gtl_env.curr_hdr_buff[2];
    uint8_t alloc_length = gtl_env.curr_hdr_buff[2];

    // Check opcode
    switch(opcode)
    {
        case HCI_RESET_CMD_OPCODE:
            msgid = LLM_RESET_CMD;
            break;

        case HCI_LE_RX_TEST_CMD_OPCODE:
            msgid = LLM_LE_TEST_RX_CMD;
            break;

        case HCI_LE_TX_TEST_CMD_OPCODE:
            msgid = LLM_LE_TEST_TX_CMD;
            break;

        case HCI_LE_TEST_END_CMD_OPCODE: 
            msgid = LLM_LE_TEST_END_CMD; 
            set_state_stop();
            break;

        case HCI_START_PROD_RX_TEST_CMD_OPCODE:
            break;

        case HCI_TX_START_CONTINUE_TEST_CMD_OPCODE:
            break;

        case HCI_TX_END_CONTINUE_TEST_CMD_OPCODE: // additional hci command for customer prod. 
                                                  // with no params, so handles here
            if (test_state == STATE_START_CONTINUE_TX)
            {
                hci_end_tx_continuous_test_cmd();
            }
            break;

        case HCI_LE_END_PROD_RX_TEST_CMD_OPCODE:    // additional hci command for customer prod. 
                                                    // with no params, so handles here
            if (test_state == STATE_START_RX)
            {
                hci_end_rx_prod_test_cmd();
            }
            break;

        case HCI_UNMODULATED_ON_CMD_OPCODE: 
            break;

        case HCI_FIRMWARE_VERSION_CMD_OPCODE:
            hci_fw_version_cmd();
            break;

        default:
        {
            msgid = GTL_HCI_UNKNOWN_CMD;
            dest_id = TASK_GTL;
            // add one more byte to prevent memory corruption if length_rx = 0
            // 1 byte of parameter length is needed for sending Command Complete
            alloc_length = length_rx + 1;
        }
        break;
    }

    // Allocate the kernel message
    if ((opcode != HCI_UNMODULATED_ON_CMD_OPCODE) && (opcode != HCI_LE_END_PROD_RX_TEST_CMD_OPCODE)
        && (opcode != HCI_START_PROD_RX_TEST_CMD_OPCODE)
        && (opcode != HCI_TX_START_CONTINUE_TEST_CMD_OPCODE) && (opcode != HCI_TX_END_CONTINUE_TEST_CMD_OPCODE)
        && !(opcode==HCI_LE_TX_TEST_CMD_OPCODE && length_rx==5) 
        && !(opcode==HCI_SLEEP_TEST_CMD_OPCODE && length_rx==3)
        && !(opcode==HCI_XTAL_TRIM_CMD_OPCODE) && !(opcode==HCI_OTP_RW_CMD_OPCODE)
        && !(opcode==HCI_OTP_READ_CMD_OPCODE) && !(opcode==HCI_OTP_WRITE_CMD_OPCODE)
        && !(opcode==HCI_REGISTER_RW_CMD_OPCODE) && !(opcode==HCI_CUSTOM_ACTION_CMD_OPCODE)
        && !(opcode==HCI_RDTESTER_CMD_OPCODE) && !(opcode==HCI_FIRMWARE_VERSION_CMD_OPCODE)
        && !(opcode==HCI_CHANGE_UART_PINS_CMD_OPCODE)
        && !(opcode==SENSOR_TEST_CMD_OPCODE)
#if HAS_AUDIO
        && !(opcode==HCI_AUDIO_TEST_CMD_OPCODE)
#endif
        && !(opcode==HCI_PLAY_TONE_CMD_OPCODE))
    {
        gtl_env.p_msg_rx = ke_param2msg(ke_msg_alloc(msgid, dest_id, TASK_GTL, alloc_length));
        gtl_env.p_msg_rx->param_len = length_rx;
    }
    else //@WR coming in this loop means we handle the customer production commands, they are not send to stack (not part of it
        // and will be handled in 'gtl_hci_rx_header' or in 'gtl_hci_rx_header and gtl_hci_rx_payload'
    {
        gtl_env.p_msg_rx = ke_param2msg(ke_msg_alloc(msgid, dest_id, TASK_GTL, alloc_length));
        gtl_env.p_msg_rx->id = opcode;
        gtl_env.p_msg_rx->param_len = length_rx;
    }
}

void gtl_hci_rx_payload(void)
{
    typedef void (*my_function)( void);
    my_function PtrFunc;
    PtrFunc = (my_function)(jump_table_struct[gtl_hci_rx_payload_func_pos]);
    PtrFunc();
}

void gtl_hci_rx_payload_func(void)
{
    void* ptr_data = gtl_env.p_msg_rx->param;

    switch(gtl_env.p_msg_rx->id)
    {
        case LLM_RESET_CMD:
            break;

        case LLM_LE_TEST_END_CMD:
            break;

        case GTL_HCI_UNKNOWN_CMD:
            break;

        case LLM_LE_TEST_RX_CMD:
            test_state = STATE_DIRECT_RX_TEST;
            break;

        case HCI_START_PROD_RX_TEST_CMD_OPCODE:
            hci_start_prod_rx_test((uint8_t*) ptr_data);
            break;

        case LLM_LE_TEST_TX_CMD:
            test_state = STATE_DIRECT_TX_TEST;
            break;

        case HCI_LE_TX_TEST_CMD_OPCODE:
            if(gtl_env.p_msg_rx->param_len == 5) 
            {
                hci_tx_send_nb_packages((uint8_t*) ptr_data);
            }
            break;

        case HCI_TX_START_CONTINUE_TEST_CMD_OPCODE:
            hci_tx_start_continue_test_mode((uint8_t*) ptr_data);
            break;

        case HCI_UNMODULATED_ON_CMD_OPCODE:
            hci_unmodulated_cmd((uint8_t*) ptr_data);
            break;

        case HCI_SLEEP_TEST_CMD_OPCODE:
            if(gtl_env.p_msg_rx->param_len == 3) 
            {
                hci_sleep_test_cmd((uint8_t*) ptr_data);
            }
            break;

        case HCI_XTAL_TRIM_CMD_OPCODE:
            hci_xtal_trim_cmd((uint8_t*) ptr_data);
            break;

        case HCI_RDTESTER_CMD_OPCODE:
            hci_rdtester_cmd((uint8_t*) ptr_data);
            break;

#if HAS_AUDIO
        case HCI_AUDIO_TEST_CMD_OPCODE:
            hci_audio_test_cmd((struct param_audio_test*) ptr_data);
            break;
#endif

        case HCI_PLAY_TONE_CMD_OPCODE:
            hci_play_tone_cmd((uint8_t*) ptr_data);
            break;

        case HCI_CUSTOM_ACTION_CMD_OPCODE:
            hci_custom_action_cmd(ptr_data);
            break;

        case HCI_CHANGE_UART_PINS_CMD_OPCODE:
            hci_change_uart_pins_cmd((uint8_t*) ptr_data);
            break;

        case SENSOR_TEST_CMD_OPCODE:
            hci_sensor_test_cmd((uint8_t*) ptr_data);
            break;

        case HCI_OTP_RW_CMD_OPCODE:
            hci_otp_rw_cmd((uint8_t*) ptr_data);
            break;

        case HCI_OTP_READ_CMD_OPCODE:
            hci_otp_read_cmd((uint8_t*) ptr_data);
            break;

        case HCI_OTP_WRITE_CMD_OPCODE:
            hci_otp_write_cmd((uint8_t*) ptr_data);
            break;

        case HCI_REGISTER_RW_CMD_OPCODE:
            hci_register_rw_cmd((uint8_t*) ptr_data);
            break;

        default: ASSERT_INFO(0, gtl_env.p_msg_rx->id, 0); break;
    }
}

uint8_t * gtl_hci_tx_evt(uint8_t * length)
{
    typedef uint8_t * (*my_function)( uint8_t *);
    my_function PtrFunc;
    PtrFunc = (my_function)(jump_table_struct[gtl_hci_tx_evt_func_pos]);
    return PtrFunc(length);
}


uint8_t * gtl_hci_tx_evt_func(uint8_t * length)
{
    uint16_t opcode = 0;
    uint8_t * pk = (uint8_t *)(&gtl_env.p_msg_tx->dest_id)+ 1;
    uint8_t param_len = HCI_CCEVT_HDR_PARLEN;

    // Check parameters length and alignment
    switch(gtl_env.p_msg_tx->id)
    {
        case LLM_RESET_CMP_EVT       : opcode = HCI_RESET_CMD_OPCODE;      param_len += HCI_CCEVT_BASIC_RETPAR_LEN; break;
        case LLM_LE_TEST_RX_CMP_EVT  : opcode = HCI_LE_RX_TEST_CMD_OPCODE; param_len += HCI_CCEVT_BASIC_RETPAR_LEN; break;
        case LLM_LE_TEST_TX_CMP_EVT  : opcode = HCI_LE_TX_TEST_CMD_OPCODE; param_len += HCI_CCEVT_BASIC_RETPAR_LEN; break;
    case LLM_LE_TEST_END_CMP_EVT:
        {
            //map structure for command return parameters on the ke msg param field - not aligned
            struct llm_test_end_cmd_complete * s = (struct llm_test_end_cmd_complete * )(gtl_env.p_msg_tx->param);
        opcode = HCI_LE_TEST_END_CMD_OPCODE;
        param_len += HCI_CCEVT_LETSTEND_RETPAR_LEN;
            co_write16p(pk + 6, co_htobs(s->nb_packet_received));
        }
        break;
    case GTL_HCI_UNKNOWN_CMD:
        {
            opcode = co_btohs(co_read16p(&gtl_env.curr_hdr_buff[0]));
        param_len += HCI_CCEVT_BASIC_RETPAR_LEN;
            *(pk + 5) = CO_ERROR_UNKNOWN_HCI_COMMAND;
        }
        break;
        default: ASSERT_INFO(0, gtl_env.p_msg_tx->id, 0); break;
    }

    //pack event code
    *pk++ = (uint8_t) HCI_CMD_CMPL_EVT_CODE;

    //pack event parameter length
    *pk++ = param_len;

    //pack the number of h2c packets
    *pk++ = 1;

    //pack the command opcode
    co_write16p(pk++, co_htobs(opcode));

    // Return the event length (incl. header / not incl. packet type)
    *length = HCI_EVT_HDR_LEN + param_len;

    return ((uint8_t *)(&gtl_env.p_msg_tx->dest_id)+ 1);
}

#endif //GTL_ITF && BLE_EMB_PRESENT

/// @} GTL_EIF
