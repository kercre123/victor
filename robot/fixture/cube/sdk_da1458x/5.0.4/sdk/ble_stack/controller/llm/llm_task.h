/**
 ****************************************************************************************
 *
 * @file llm_task.h
 *
 * @brief LLM task header file
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */

#ifndef LLM_TASK_H_
#define LLM_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup LLMTASK LLMTASK
 * @ingroup LLM
 * @brief Link Layer Manager Task
 *
 * The LLM task is responsible for managing link layer actions not related to a
 * specific connection with a peer (e.g. scanning, advertising, etc.). It implements the
 * state machine related to these actions.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "co_bt.h"
#include "rwip_config.h" // stack configuration
#include "ke_task.h"


/*
 * INSTANCES
 ****************************************************************************************
 */
/// Maximum number of instances of the LLM task
#define LLM_IDX_MAX 1

/// LE events subcodes
#define LLM_LE_EVT_ADV_REPORT       0x02
/*
 * STATES
 ****************************************************************************************
 */
/// Possible states of the LLM task
enum
{
    /// IDLE state
    LLM_IDLE,
    /// ADVERTISING state
    LLM_ADVERTISING,
    /// SCANNING state
    LLM_SCANNING,
    /// INITIATING state
    LLM_INITIATING,
    /// STOPPING state
    LLM_STOPPING,
    /// TEST states.
    LLM_TEST,
    /// Number of states.
    LLM_STATE_MAX
};

/*
 * MESSAGES
 ****************************************************************************************
 */
/// Message API of the LLM task
enum llm_msg_id
{
    /*
     * ************** Msg HL->LLM****************
     */

    /// Command received from the HCI.
    LLM_MSG_ID_CMD_FIRST = KE_FIRST_MSG(TASK_LLM),

    /// Legacy commands
    LLM_SET_EVT_MASK_CMD = LLM_MSG_ID_CMD_FIRST,
    LLM_RESET_CMD,
    LLM_SET_CNTLR_TO_HOST_FLOW_CNTL_CMD,
    LLM_HOST_BUFFER_SIZE_CMD,
    LLM_HOST_NB_COMPLETED_PKTS_CMD,
    LLM_RD_LOCAL_VER_INFO_CMD,
    LLM_RD_LOCAL_SUPP_CMDS_CMD,
    LLM_RD_LOCAL_SUPP_FEATS_CMD,
    LLM_RD_BUFF_SIZE_CMD,
    LLM_RD_BD_ADDR_CMD,

    ///LE Commands
    LLM_LE_SET_EVT_MASK_CMD,
    LLM_LE_RD_BUFF_SIZE_CMD,
    LLM_LE_RD_LOCAL_SUPP_FEATS_CMD,
    LLM_LE_SET_RAND_ADDR_CMD,
    LLM_LE_SET_ADV_PARAM_CMD,
    LLM_LE_RD_ADV_CHNL_TX_PW_CMD,
    LLM_LE_SET_ADV_DATA_CMD,
    LLM_LE_SET_SCAN_RSP_DATA_CMD,
    LLM_LE_SET_ADV_EN_CMD,
    LLM_LE_SET_SCAN_PARAM_CMD,
    LLM_LE_SET_SCAN_EN_CMD,
    LLM_LE_CREATE_CON_CMD,
    LLM_LE_CREATE_CON_CANCEL_CMD,
    LLM_LE_RD_WLST_SIZE_CMD,
    LLM_LE_CLEAR_WLST_CMD,
    LLM_LE_ADD_DEV_TO_WLST_CMD,
    LLM_LE_RMV_DEV_FROM_WLST_CMD,
    LLM_LE_SET_HOST_CHNL_CLASSIF_CMD,
    LLM_LE_ENC_CMD,
    LLM_LE_RAND_CMD,
    LLM_LE_RD_SUPP_STATES_CMD,
    LLM_LE_TEST_RX_CMD,
    LLM_LE_TEST_TX_CMD,
    LLM_LE_TEST_END_CMD,

    LLM_MSG_ID_EVT_FIRST,

    /*
     * ************** Msg LLM -> HL****************
     */
    /// Legacy Commands Command Complete Events sent to HCI
    LLM_NO_OPERATIONS = LLM_MSG_ID_EVT_FIRST,
    LLM_SET_EVT_MASK_CMP_EVT,
    LLM_RESET_CMP_EVT,
    LLM_SET_CNTLR_TO_HOST_FLOW_CNTL_CMP_EVT,
    LLM_HOST_BUFFER_SIZE_CMP_EVT,
    LLM_HOST_NB_COMPLETED_PKTS_CMP_EVT,
    LLM_RD_LOCAL_VER_INFO_CMP_EVT,
    LLM_RD_LOCAL_SUPP_CMDS_CMP_EVT,
    LLM_RD_LOCAL_SUPP_FEATS_CMP_EVT,
    LLM_RD_BUFF_SIZE_CMP_EVT,
    LLM_RD_BD_ADDR_CMP_EVT,

    ///Special Legacy event
    LLM_HW_ERROR_EVT,

    /// LE Commands Command Complete Events sent to HCI
    LLM_LE_SET_EVT_MASK_CMP_EVT,
    LLM_LE_RD_BUFF_SIZE_CMP_EVT,
    LLM_LE_RD_LOCAL_SUPP_FEATS_CMP_EVT,
    LLM_LE_SET_RAND_ADDR_CMP_EVT,
    LLM_LE_SET_ADV_PARAM_CMP_EVT,
    LLM_LE_RD_ADV_CHNL_TX_PW_CMP_EVT,
    LLM_LE_SET_ADV_DATA_CMP_EVT,
    LLM_LE_SET_SCAN_RSP_DATA_CMP_EVT,
    LLM_LE_SET_ADV_EN_CMP_EVT,
    LLM_LE_SET_SCAN_PARAM_CMP_EVT,
    LLM_LE_SET_SCAN_EN_CMP_EVT,
    LLM_LE_CREATE_CON_CANCEL_CMP_EVT,
    LLM_LE_RD_WLST_SIZE_CMP_EVT,
    LLM_LE_CLEAR_WLST_CMP_EVT,
    LLM_LE_ADD_DEV_TO_WLST_CMP_EVT,
    LLM_LE_RMV_DEV_FROM_WLST_CMP_EVT,
    LLM_LE_SET_HOST_CHNL_CLASSIF_CMP_EVT,
    LLM_LE_ENC_CMP_EVT,
    LLM_LE_RAND_CMP_EVT,
    LLM_LE_RD_SUPP_STATES_CMP_EVT,
    LLM_LE_TEST_RX_CMP_EVT,
    LLM_LE_TEST_TX_CMP_EVT,
    LLM_LE_TEST_END_CMP_EVT,

    ///LE Command Status Events
    LLM_LE_CREATE_CON_STAT_EVT,

    ///LE META events
    LLM_LE_CREATE_CON_CMP_EVT,
    LLM_LE_ADV_REPORT_EVT,

    /// last event message id
    LLM_MSG_ID_EVT_LAST,

    /*
     * ************** Msg LLM->LLM****************
     */
    /// Time out message
    /// to authorize the reception of the cmd
    LLM_LE_SET_HOST_CH_CLASS_CMD_STO,

    /*
     * ************** Msg LLM->LLC****************
     */
    /// request changing map
    LLM_LE_SET_HOST_CHNL_CLASSIF_REQ,
    /// channel map req indication
    LLM_LE_SET_HOST_CHNL_CLASSIF_REQ_IND,
    /*
     * ************** Msg LLC->LLM****************
     */
    /// link layer disconnection indication
    LLC_DISCO_IND,

    /*
     * ************** Channel Assessment ********
     */
    /// Channel Assessment Timer
    LLM_LE_CHNL_ASSESS_TIMER,
    /// Channel Assessment - Generate Channel classification
    LLM_GEN_CHNL_CLS_CMD
};

// LE command structures

///LLM LE Set Event Mask Command parameters structure
struct llm_le_set_evt_mask_cmd
{
    ///LE Event Mask
    struct evt_mask le_mask;
};

///LLM LE Set Random Address Command parameters structure
struct llm_le_set_rand_addr_cmd
{
    ///Random address to set
    struct bd_addr rand_addr;
};

///LLM LE Set Advertising Parameters Command parameters structure
struct llm_le_set_adv_param_cmd
{
    ///Minimum interval for advertising
    uint16_t       adv_intv_min;
    ///Maximum interval for advertising
    uint16_t       adv_intv_max;
    ///Advertising type
    uint8_t        adv_type;
    ///Own address type: public=0x00 /random = 0x01
    uint8_t        own_addr_type;
    ///Direct address type: public=0x00 /random = 0x01
    uint8_t        direct_addr_type;
    ///Direct Bluetooth device address
    struct bd_addr direct_addr;
    ///Advertising channel map
    uint8_t        adv_chnl_map;
    ///Advertising filter policy
    uint8_t        adv_filt_policy;
};

///LLM LE Set Advertising Data Command parameters structure
struct llm_le_set_adv_data_cmd
{
    ///Advertising data length
    uint8_t         adv_data_len;
    ///Advertising data - maximum 31 bytes
    struct adv_data data;
};

///LLM LE Set Scan Response Data Command parameters structure
struct llm_le_set_scan_rsp_data_cmd
{
    ///Scan response data length
    uint8_t              scan_rsp_data_len;
    ///Scan response data - maximum 31 bytes
    struct scan_rsp_data data;
};

///LLM LE Set Advertise Enable Command parameters structure
struct llm_le_set_adv_en_cmd
{
    ///Advertising enable - 0=disabled, 1=enabled
    uint8_t        adv_en;
};

///LLM LE Set Scan Parameters Command parameters structure
struct llm_le_set_scan_param_cmd
{
    ///Scan type - 0=passive / 1=active
    uint8_t        scan_type;
    ///Scan interval
    uint16_t       scan_intv;
    ///Scan window size
    uint16_t       scan_window;
    ///Own address type - 0=public, 1=random
    uint8_t        own_addr_type;
    ///Scan filter policy
    uint8_t        scan_filt_policy;
};

///LLM LE Set Scan Enable Command parameters structure
struct llm_le_set_scan_en_cmd
{
    ///Scan enable - 0=disabled, 1=enabled
    uint8_t        scan_en;
    ///Enable for duplicates filtering - 0 =disabled/ 1=enabled
    uint8_t        filter_duplic_en;
};

///LLM LE Create Connection Command parameters structure
struct llm_le_create_con_cmd
{
    ///Scan interval
    uint16_t       scan_intv;
    ///Scan window size
    uint16_t       scan_window;
    ///Initiator filter policy
    uint8_t        init_filt_policy;
    ///Peer address type - 0=public/1=random
    uint8_t        peer_addr_type;
    ///Peer BD address
    struct bd_addr peer_addr;
    ///Own address type - 0=public/1=random
    uint8_t        own_addr_type;
    ///Minimum of connection interval
    uint16_t       con_intv_min;
    ///Maximum of connection interval
    uint16_t       con_intv_max;
    ///Connection latency
    uint16_t       con_latency;
    ///Link supervision timeout
    uint16_t       superv_to;
    ///Minimum CE length
    uint16_t       ce_len_min;
    ///Maximum CE length
    uint16_t       ce_len_max;
};

///LLM LE Add Device to White List Command parameters structure
struct llm_le_add_dev_to_wlst_cmd
{
    ///Type of address of the device to be added to the White List - 0=public/1=random
    uint8_t        dev_addr_type;
    ///Address of device to be added to White List
    struct bd_addr dev_addr;
};

///LLM LE Remove Device from White List Command parameters structure
struct llm_le_rmv_dev_from_wlst_cmd
{
    ///Type of address of the device to be removed from the White List - 0=public/1=random
    uint8_t        dev_addr_type;
    ///Address of device to be removed from White List
    struct bd_addr dev_addr;
};


///LLM LE Set Host Channel Classification Command parameters structure
struct llm_le_set_host_chnl_classif_cmd
{
    ///Channel map
    struct le_chnl_map chmap;
};


///LLM LE Receiver Test Command parameters structure
struct llm_le_rx_test_cmd
{
    ///RX frequency for Rx test
    uint8_t        rx_freq;
};

///LLM LE Transmitter Test Command parameters structure
struct llm_le_tx_test_cmd
{
    ///TX frequency for Tx test
    uint8_t        tx_freq;
    ///TX test data length
    uint8_t        test_data_len;
    ///TX test payload type - see enum
    uint8_t        pk_payload_type;
};

///LLM LE Encrypt Command parameters structure
struct llm_le_enc_cmd
{
    ///Long term key structure
    struct ltk     key;
    ///Pointer to buffer with plain data to encrypt - 16 bytes
    uint8_t        plain_data[16];
};

// LE event structures
/// LLM_EVENT_CMD_COMPLETE parameters structure description.
struct llm_event_cmd_complete
{
    /// Event type
    uint8_t uc_event;
    /// Event length
    uint8_t uc_length;
    /// Cmd Opcode
    uint16_t uw_opcode;
    /// Event parameters
    uint8_t auc_params[30];
};
/// LLM complete event with status only.
struct llm_event_common_cmd_complete
{
    /// Status of the command completion
    uint8_t status;
};

/// LLM complete event with status only.
struct llm_event_common_cmd_status
{
    /// Status of the command reception
    uint8_t status;
};
/// LLM command complete event structure for the Read Local Supported Features
struct llm_rd_local_supp_feats_cmd_complete
{
    /// Status of the command reception
    uint8_t             status;
    ///Local LE supported features
    struct le_features  feats;
};

/// LLM command complete event structure for the Read Advertising Channel Tx Power Command
struct llm_rd_adv_chnl_tx_pw_cmd_complete
{
    /// Status of the command reception
    uint8_t     status;
    ///Advertising channel Tx power level
#ifndef __DA14581__
    uint8_t     adv_tx_pw_lvl;
#else
    int8_t     adv_tx_pw_lvl;
#endif
};

///LLM command complete event structure for the Read White List Size Command
struct llm_rd_wlst_size_cmd_complete
{
    /// Status of the command reception
    uint8_t     status;
    ///White List size
    uint8_t     wlst_size;
};

///LLM command complete event structure for the Read Buffer Size Command
struct llm_le_rd_buff_size_cmd_complete
{
    /// Status of the command reception
    uint8_t     status;
    ///ACL data packet length that can be sent from host to controller
    uint16_t    hc_data_pk_len;
    ///Total number of ACL data packets that can be sent from host to controller.
    uint8_t     hc_tot_nb_data_pkts;
};

///LLM command complete event structure for Rand Command
struct llm_rand_cmd_complete
{
    /// Status of the command reception
    uint8_t             status;
    ///Random number
    struct rand_nb      nb;
};

///LLM LE Encrypt complete event structure
struct llm_le_enc_cmp_evt
{
    /// Status of the command reception
    uint8_t status;
    ///Encrypted data to return to command source.
    uint8_t encrypted_data[16];
};

///LLM command complete event structure for Read Supported States Command
struct llm_rd_supp_states_cmd_complete
{
    /// Status of the command reception
    uint8_t             status;
    ///LE supported states response
    struct le_states    states;
};

///LLM command complete event structure for Test End
struct llm_test_end_cmd_complete
{
    /// Status of the command reception
    uint8_t             status;
    ///Number of RX packets - null if TX test was the ended one
    uint16_t            nb_packet_received;
};

///LLM LE advertising report event structure
struct llm_le_adv_report_evt
{
    ///LE Subevent code
    uint8_t             subevent_code;
    ///Number of advertising reports in this event
    uint8_t             nb_reports;
    ///Advertising reports structures array
    struct adv_report   adv_rep[BLE_ADV_REPORTS_MAX];
};

///LLM le advertising indirect, non_connectable, discoverable pdu structure
struct llm_adv_pdu
{
    /// advertiser address
    struct bd_addr adv_addr;
    /// advertising data
    uint16_t * adv_data;
};

///LLM le advertising direct pdu structure
struct llm_adv_direct_pdu
{
    /// advertiser address
    struct bd_addr adv_addr;
    /// initiator address
    struct bd_addr init_addr;
};

/// rx descriptor structure
struct llm_adv_rx_desc
{
    /// RX pointer
    struct llm_adv_rx_desc *rxptr;
    /// RX status
    uint16_t rxstatce;
    /// RX header
    uint16_t rxphadv;
    /// RX buffer
    uint8_t *rxbuf;
};

///LLM command complete event structure for read bd address
struct llm_rd_bd_addr_cmd_complete
{
    /// Status of the command reception
    uint8_t             status;
    ///BD address
    struct bd_addr      local_addr;
};

///LLM command complete event structure for read local version information
struct llm_rd_local_ver_info_cmd_complete
{
    /// Status of the command reception
    uint8_t     status;
    ///HCI version number
    uint8_t     hci_ver;
    ///HCI revision number
    uint16_t    hci_rev;
    ///LMP version
    uint8_t     lmp_ver;
    ///manufacturer name
    uint16_t    manuf_name;
    ///LMP Subversion
    uint16_t    lmp_subver;
};

///LLM command complete event structure for read local supported commands
struct llm_rd_local_supp_cmds_cmd_complete
{
    /// Status of the command reception
    uint8_t             status;
    ///Supported Commands structure
    struct supp_cmds    local_cmds;
};

///LLM set event mask command structure
struct llm_set_evt_msk_cmd
{
    ///Event Mask
    struct evt_mask    event_mask;
};

///LLM set controller to host flow control command
struct llm_set_cntl_host_flow_cmd
{
    ///Flow control enable for controller
    uint8_t flow_cntl;
};

///LLM host buffer size command
struct llm_host_buff_size_cmd
{
    ///Host ACL packet length
    uint16_t    acl_pkt_len;
    ///Host synchronous packet length
    uint8_t     sync_pkt_len;
    ///Host Total number of ACL packets allowed
    uint16_t    nb_acl_pkts;
    ///Host total number of synchronous packets allowed
    uint16_t    nb_sync_pkts;
};

///LLM host number of completed packets command
struct llm_host_nb_cmp_pkts_cmd
{
    ///Number of handles for which the completed packets number is given
    uint8_t     nb_of_hdl;
    ///Array of connection handles
    uint16_t    con_hdl[BLE_CONNECTION_MAX];
    ///Array of number of completed packets values for connection handles.
    uint16_t    nb_comp_pkt[BLE_CONNECTION_MAX];
};

///LLM command complete event structure for the Read Buffer Size Command
struct llm_rd_buff_size_cmd_complete
{
    /// Status of the command reception
    uint8_t     status;
    ///ACL data packet length controller can receive from host
    uint16_t     hc_data_pk_len;
    ///Synchronous data packet length controller can receive from host
    uint8_t      hc_sync_pk_len;
    ///Total number of ACL data packets controller can receive from host
    uint16_t     hc_tot_nb_data_pkts;
    ///Total number of synchronous data packets controller can receive from host
    uint16_t     hc_tot_nb_sync_pkts;
};

/*
 * API LLM --> LLC
 */

///LLM Hardware Error Event parameters structure
struct llm_hw_error_evt
{
    /// HW error code
    uint8_t hw_code;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler llm_state_handler[LLM_STATE_MAX];
extern const struct ke_state_handler llm_default_handler;
extern ke_state_t llm_state[LLM_IDX_MAX];

/// @} LLMTASK

#endif // LLM_TASK_H_
