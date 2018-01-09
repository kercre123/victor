/**
 ****************************************************************************************
 *
 * @file llm.h
 *
 * @brief Main API file for the Link Layer manager
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */

#ifndef LLM_H_
#define LLM_H_

/**
 ****************************************************************************************
 * @addtogroup CONTROLLER
 * @ingroup ROOT
 * @brief BLE Lower Layers
 *
 * The CONTROLLER contains the modules allowing the physical link establishment,
 * maintenance and management.
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup LLM LLM
 * @ingroup CONTROLLER
 * @brief Link Layer Manager
 *
 * The link layer manager contains the modules allowing the physical link establishment,
 * and all the non-connected states.
 * @{
 ****************************************************************************************
 */




/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "co_bt.h"
#include "co_math.h"
#include "co_utils.h"
#include "co_buf.h"
#include "llm_task.h"
#include "rwip.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#if (BLE_CHNL_ASSESS)
/// Default Channel Assessment Timer duration (20s - Multiple of 10ms)
#define LLM_CHNL_ASSESS_DFLT_TIMER_DUR      (2000)
/// Default Channel Reassessment Timer duration (Multiple of Channel Assessment Timer duration)
#define LLM_CHNL_REASSESS_DFLT_TIMER_DUR    (6)
/// Default Minimal RSSI Threshold - -48dBm
#define LLM_CHNL_ASSESS_DFLT_MIN_RSSI       (-48)
/// Default number of packets to receive for statistics
#define LLM_CHNL_ASSESS_DFLT_NB_PKT         (100)
/// Default number of bad packets needed to remove a channel
#define LLM_CHNL_ASSESS_DFLT_NB_BAD_PKT     (LLM_CHNL_ASSESS_DFLT_NB_PKT/2)
#endif //(BLE_CHNL_ASSESS)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
/// Advertising parameters
struct advertising_pdu_params
{
#ifdef __DA14581__
	/// Pointer on the data adv request
	struct ke_msg * adv_data_req;
#endif
    /// Connection interval min
    uint16_t intervalmin;
    /// Connection interval max
    uint16_t intervalmax;
    /// Channel mapping
    uint8_t channelmap;
    /// Filtering policy
    uint8_t filterpolicy;
    /// Advertising type
    uint8_t type;
    /// Data length
    uint8_t datalen;
    /// Scan RSP length
    uint8_t scanrsplen;
    /// Local address type
    uint8_t own_addr_type;
};

///Scanning parameters
struct scanning_pdu_params
{
    /// Scan interval
    uint16_t interval;
    /// Scan window
    uint16_t window;
    /// Filtering policy
    uint8_t filterpolicy;
    /// Scanning type
    uint8_t type;
    /// Duplicate the advertising report
    uint8_t filter_duplicate;
    /// Local address type
    uint8_t own_addr_type;
};

///Access address generation structure
struct access_addr_gen
{
    /// random
    uint8_t intrand;
    /// index 1
    uint8_t ct1_idx;
    /// index 2
    uint8_t ct2_idx;
};

/// advertising report list
struct adv_device_list
{
    /// Advertising type
    uint8_t adv_type;
    /// Advertising device address
    struct bd_addr adv_addr;
};

//advertising pdu
///structure adv undirected
struct llm_pdu_adv
{
    /// advertising address
    struct bd_addr  adva;
    /// advertising data
    uint8_t         *adva_data;
};
///structure adv directed
struct llm_pdu_adv_directed
{
    /// advertising address
    struct bd_addr  adva;
    /// initiator address
    struct bd_addr  inita;
};

//scanning pdu
///structure scan request
struct llm_pdu_scan_req
{
    /// scanning address
    struct bd_addr  scana;
    /// advertising address
    struct bd_addr  adva;
};
///structure scan response
struct llm_pdu_scan_rsp
{
    /// advertising address
    struct bd_addr  adva;
    /// scan response data
    uint8_t         *scan_data;

};
///initiating pdu
///structure connection request reception
struct llm_pdu_con_req_rx
{
    /// initiator address
    struct bd_addr      inita;
    /// advertiser address
    struct bd_addr      adva;
    /// access address
    struct access_addr  aa;
    /// CRC init
    struct crc_init     crcinit;
    /// window size
    uint8_t             winsize;
    /// window offset
    uint16_t            winoffset;
    /// interval
    uint16_t            interval;
    /// latency
    uint16_t            latency;
    /// timeout
    uint16_t            timeout;
    /// channel mapping
    struct le_chnl_map  chm;
    /// hopping
    uint8_t             hop_sca;
};
///structure connection request transmission
struct llm_pdu_con_req_tx
{
    /// access address
    struct access_addr  aa;
    /// CRC init
    struct crc_init     crcinit;
    /// window size
    uint8_t             winsize;
    /// window offset
    uint16_t            winoffset;
    /// interval
    uint16_t            interval;
    /// latency
    uint16_t            latency;
    /// timeout
    uint16_t            timeout;
    /// channel mapping
    struct le_chnl_map  chm;
    /// hopping
    uint8_t             hop_sca;
};

///structure for the test mode
struct llm_test_mode
{
    /// flag indicating the end of test
    bool end_of_tst;
    /// Direct test type
    uint8_t  directtesttype;
    /// Number of packet received
    uint16_t nb_pkt_rcv;
};

/// LLM environment structure to be saved
struct llm_le_env_tag
{
    /// List of encryption requests
    struct co_list enc_req;

    /// Advertising parameters
    struct advertising_pdu_params advertising_params;

    ///Scanning parameters
    struct scanning_pdu_params scanning_params;

    /// Event mask
    struct evt_mask eventmask;

    /// Access address
    struct access_addr_gen aa;

    ///protection for the command
    bool llm_le_set_host_ch_class_cmd_sto;

    /// Roles
    uint8_t role;

    /// conhdl_allocated
    uint16_t conhdl_alloc;

    #if (BLE_CHNL_ASSESS)
    /// Duration of channel assessment timer
    uint16_t chnl_assess_timer;
    /// Max number of received packets
    uint16_t chnl_assess_nb_pkt;
    /// Max number of received bad packets
    uint16_t chnl_assess_nb_bad_pkt;
    #endif // (BLE_CHNL_ASSESS)

    /// event pointer used for advertising/scanning/initiating
    struct lld_evt_tag *evt;

    ///encryption pending
    bool enc_pend;

    ///test mode
    struct llm_test_mode test_mode;

    /// Active link counter
    uint8_t cpt_active_link;

    /// Current channel map
    struct le_chnl_map ch_map;

    /// random bd_address
    struct bd_addr rand_add;

    /// public bd_address
    struct bd_addr public_add;

    /// current @type in the register
    uint8_t curr_addr_type;

    #if (BLE_CHNL_ASSESS)
    /// Minimal received signal strength
    int8_t chnl_assess_min_rssi;
    /// Counter value used for channel reassessment
    uint8_t chnl_reassess_cnt_val;
    /// Counter used for channel reassessment
    uint8_t chnl_reassess_cnt;
    #endif //(BLE_CHNL_ASSESS)
#ifdef __DA14581__
#if (BLE_CENTRAL || BLE_OBSERVER)
    /// advertising reports filter policy
    struct adv_device_list adv_list[BLE_DUPLICATE_FILTER_MAX];
#endif // BLE_CENTRAL || BLE_OBSERVER
#endif
};

#if (BLE_CENTRAL || BLE_OBSERVER)
/// LLM environment structure not to be saved
struct llm_le_env_tag_lpus
{
    /// advertising reports filter policy
    struct adv_device_list adv_list[BLE_DUPLICATE_FILTER_MAX];

};
#endif // BLE_CENTRAL || BLE_OBSERVER


/// Flow cntl structure
struct flow_control_ble
{
    /// flow control enabled
    bool flow_cntl_en;
    /// host packet size max
    uint16_t pkt_size;
    /// host packet number max
    uint16_t pkt_nb;
    /// current packet available
    uint16_t curr_pkt_nb;
};

///LLM environment tag BT
struct llm_bt_env_tag
{
    /// Flow control
    struct flow_control_ble flow_cntl;
};

extern const struct supp_cmds llm_local_cmds;
extern const struct le_features llm_local_le_feats;
extern const struct le_states llm_local_le_states;

/*
 * DEFINES
 ****************************************************************************************
 */
/// Advertising channel TX power
#define LLM_ADV_CHANNEL_TXPWR                   rwip_rf.txpwr_max

/// Advertising set parameters range min
#define LLM_ADV_INTERVAL_MIN                    32//(0x20)
/// Advertising set parameters range max
#define LLM_ADV_INTERVAL_MAX                    16384//(0x4000)

/// Scanning set parameters range min
#define LLM_SCAN_INTERVAL_MIN                    4//(0x4)
/// Scanning set parameters range max
#define LLM_SCAN_INTERVAL_MAX                    16384//(0x4000)

/// Scanning set parameters range min
#define LLM_SCAN_WINDOW_MIN                    4//(0x4)
/// Scanning set parameters range max
#define LLM_SCAN_WINDOW_MAX                    16384//(0x4000)


/// In case where the adv_type is set to non connectable or discoverable
#define LLM_ADV_INTERVAL_MIN_NONCON_DISC        160 //(0xA0)

/// Time out value for the advertising direct event
#define LLM_LE_ADV_TO_DIRECTED                  1024  //10,24 s -> 1024 ticks(10ms)

/// Default value for the number of advertising report
#define LLM_LE_ADV_REPORT_DFT                   0x1

/// Frequency max for the receiver test mode
#define RX_TEST_FREQ_MAX                        39

/// Length max for the receiver test mode
#define TX_TEST_LEN_MAX                         37

/// Number max of good channel
#define LE_NB_CH_MAP_MAX                        37

/// default irq interrupt threshold
#define RX_THR_DFT                              1

/// Index dedicated for the advertising pdu
enum
{
    LLM_LE_ADV_DUMMY_IDX = (BLE_TX_BUFFER_DATA + BLE_TX_BUFFER_CNTL - 1),
    #if (BLE_OBSERVER || BLE_PERIPHERAL || BLE_CENTRAL)
    LLM_LE_SCAN_CON_REQ_ADV_DIR_IDX,
    #endif // BLE_OBSERVER || BLE_PERIPHERAL || BLE_CENTRAL
    #if (BLE_BROADCASTER || BLE_PERIPHERAL)
    LLM_LE_SCAN_RSP_IDX,
    #endif // BLE_BROADCASTER || BLE_PERIPHERAL
    LLM_LE_ADV_IDX
};

/// Advertising Access Address
#define LLM_LE_ADV_AA    0x8E89BED6

/// Scanning default interval (10ms)
#define LLM_LE_SCAN_INTERV_DFLT       16 //(0X10)

/// Advertising default interval (1,28s)
#define LLM_LE_ADV_INTERV_DFLT        2048 //(0X800)

/// Advertising default channel map (ch37, ch38, ch39)
#define LLM_LE_ADV_CH_MAP_DFLT        0X7


/// Features byte 0
#define BLE_FEATURES_BYTE0  BLE_ENC_FEATURE
/// Features byte 1
#define BLE_FEATURES_BYTE1  0x00
/// Features byte 2
#define BLE_FEATURES_BYTE2  0x00
/// Features byte 3
#define BLE_FEATURES_BYTE3  0x00
/// Features byte 4
#define BLE_FEATURES_BYTE4  0x00
/// Features byte 5
#define BLE_FEATURES_BYTE5  0x00
/// Features byte 6
#define BLE_FEATURES_BYTE6  0x00
/// Features byte 7
#define BLE_FEATURES_BYTE7  0x00

/// States byte 0
#define BLE_STATES_BYTE0    ( BLE_NON_CON_ADV_STATE | BLE_DISC_ADV_STATE\
                            | BLE_CON_ADV_STATE | BLE_DIRECT_ADV_STATE\
                            | BLE_PASS_SCAN_STATE | BLE_ACTIV_SCAN_STATE\
                            | BLE_INIT_MASTER_STATE | BLE_CON_SLAVE_STATE)
/// States byte 1
#define BLE_STATES_BYTE1    0x00
/// States byte 2
#define BLE_STATES_BYTE2    ( BLE_NON_CON_ADV_MASTER_STATE | BLE_DISC_ADV_MASTER_STATE\
                            | BLE_NON_CON_ADV_SLAVE_STATE | BLE_DISC_ADV_SLAVE_STATE)
/// States byte 3
#define BLE_STATES_BYTE3    ( BLE_PASS_SCAN_MASTER_STATE | BLE_ACTIV_SCAN_MASTER_STATE\
                            | BLE_PASS_SCAN_SLAVE_STATE | BLE_ACTIV_SCAN_SLAVE_STATE\
                            | BLE_INIT_MASTER_MASTER_STATE)
/// States byte 4
#define BLE_STATES_BYTE4    0x0
/// States byte 5
#define BLE_STATES_BYTE5    0x0
/// States byte 6
#define BLE_STATES_BYTE6    0x0
/// States byte 7
#define BLE_STATES_BYTE7    0x0

/// BLE supported commands

//byte1
#define BLE_CMDS_BYTE0      BLE_DISC_CMD
//byte2
#define BLE_CMDS_BYTE2      BLE_RD_REM_VERS_CMD
//byte5
#define BLE_CMDS_BYTE5      (BLE_SET_EVT_MSK_CMD | BLE_RESET_CMD)
//byte10
#define BLE_CMDS_BYTE10     BLE_HL_NB_CMP_PKT_CMD | BLE_RD_TX_PWR_CMD
//byte14
#define BLE_CMDS_BYTE14     (BLE_RD_LOC_VERS_CMD | BLE_RD_LOC_SUP_FEAT_CMD\
                            |BLE_RD_BUF_SIZE_CMD)
//byte15
#define BLE_CMDS_BYTE15     (BLE_RD_BD_ADDR_CMD | BLE_RD_RSSI_CMD)
//byte25
#define BLE_CMDS_BYTE25     (BLE_LE_SET_EVT_MSK_CMD | BLE_LE_RD_BUF_SIZE_CMD\
                            |BLE_LE_RD_LOC_SUP_FEAT_CMD | BLE_LE_SET_RAND_ADDR_CMD\
                            |BLE_LE_SET_ADV_PARAM_CM | BLE_LE_RD_ADV_TX_PWR_CMD\
                            |BLE_LE_SET_ADV_DATA_CMD)
//byte26
#define BLE_CMDS_BYTE26     (BLE_LE_SET_SC_RSP_DATA_CMD | BLE_LE_SET_ADV_EN_CMD\
                            |BLE_LE_SET_SC_PARAM_CMD | BLE_LE_SET_SC_EN_CMD\
                            |BLE_LE_CREAT_CNX_CMD | BLE_LE_CREAT_CNX_CNL_CMD\
                            |BLE_LE_RD_WL_SIZE_CMD | BLE_LE_CLEAR_WL_CMD)
//byte27
#define BLE_CMDS_BYTE27     (BLE_LE_ADD_DEV_WL_CMD | BLE_LE_REM_DEV_WL_CMD\
                            |BLE_LE_CNX_UPDATE_CMD | BLE_LE_SET_HL_CH_CLASS_CMD\
                            |BLE_LE_RD_CH_MAP_CMD | BLE_LE_RD_REM_USED_FEAT_CMD\
                            |BLE_LE_ENCRYPT_CMD | BLE_LE_RAND_CMD)
//byte28
#define BLE_CMDS_BYTE28     (BLE_LE_START_ENC_CMD | BLE_LE_LTK_REQ_RPLY_CMD\
                            |BLE_LE_LTK_REQ_NEG_RPLY_CMD | BLE_LE_RD_SUPP_STATES_CMD\
                            |BLE_LE_RX_TEST_CMD | BLE_LE_TX_TEST_CMD\
                            |BLE_LE_STOP_TEST_CMD)

///Le Initiating enables
enum
{
    INIT_DIS                  = 0x00,
    INIT_EN,
    INIT_EN_END
};
///Le Direct test types
enum
{
    TEST_TX                  = 0x00,
    TEST_RX,
    TEST_RXTX,
    TEST_END
};

///Advertising pdu Type
enum
{
    LLM_ADV_CONN_UNDIR                = 0x00,
    LLM_ADV_CONN_DIR,
    LLM_ADV_NONCONN_UNDIR,
    LLM_SCAN_REQ,
    LLM_SCAN_RSP,
    LLM_CONNECT_REQ,
    LLM_ADV_DISC_UNDIR,
    LLM_ADV_END
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
extern struct llm_le_env_tag llm_le_env;
#if (BLE_CENTRAL || BLE_OBSERVER)
extern struct llm_le_env_tag_lpus llm_le_env_lpus;
#endif // BLE_CENTRAL || BLE_OBSERVER
extern struct llm_bt_env_tag llm_bt_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialization of the BLE LLM task
 *
 * This function initializes the the LLC task, as well as the environment of the LLM
 *
 ****************************************************************************************
 */
void llm_init(void);

/**
 ****************************************************************************************
 * @brief Sends the NOP event.
 *
 * This function sends the No Operation command completed event to the host when all the
 * Initializations are done.
 *
 ****************************************************************************************
 */
void llm_ble_ready(void);

/**
 ****************************************************************************************
 * @brief Handle the command clear the white list.
 *
 * This function clear the public and private white lists.
 *
 ****************************************************************************************
 */
void llm_wl_clr(void);

/**
 ****************************************************************************************
 * @brief Handle the command set advertising parameters.
 *
 * This function checks the parameters , fulfill the advertising packet with the useful
 * parameters
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to setup the advertising mode.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_set_adv_param(struct llm_le_set_adv_param_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command set advertising data.
 *
 * This function checks the parameters , fulfill the advertising packet with the data
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to setup the advertising data.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_set_adv_data(struct llm_le_set_adv_data_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command set advertising enable.
 *
 * This function checks the parameters , starts or stops the requested advertising mode.
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to start or stop the advertising mode.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_set_adv_en(struct llm_le_set_adv_en_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command set scanning parameters.
 *
 * This function checks the parameters , fulfill the scanning packet with the useful
 * parameters
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to setup the scanning mode.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_set_scan_param(struct llm_le_set_scan_param_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command set scanning enable.
 *
 * This function checks the parameters , starts or stops the requested scanning mode.
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to start or stop the scanning mode.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_set_scan_en(struct llm_le_set_scan_en_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command set scanning response data.
 *
 * This function checks the parameters , fulfill the scanning packet with the data
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to setup the scanning response data.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_set_scan_rsp_data(struct llm_le_set_scan_rsp_data_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command start transmit test mode.
 *
 * This function checks the parameters , set the transmit mode parameters, turn on the
 * mode and set the LLM state.
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to setup the transmit test mode.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_test_mode_start_tx(struct llm_le_tx_test_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command start receive test mode.
 *
 * This function checks the parameters , set the receive mode parameters, turn on the
 * mode and set the LLM state.
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to setup the receive test mode.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_test_mode_start_rx(struct llm_le_rx_test_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command add device in the white list.
 *
 * This function adds the device in the white list, according to the type (public or
 * private)
 *
 * @param[in] param        Pointer on the structure which contains the type and the
 *                         address of the device to add.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_wl_dev_add(struct llm_le_add_dev_to_wlst_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command remove device in the white list.
 *
 * This function removes the device in the white list, according to the type (public or
 * private)
 *
 * @param[in] param        Pointer on the structure which contains the type and the
 *                         address of the device to remove.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_wl_dev_rem(struct llm_le_add_dev_to_wlst_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command create connection.
 *
 * This function checks the parameters, fulfill the connect_req pdu with the useful
 * parameters, changes the state of the LLM task and requests the LLD to schedule the
 * connection initiation.
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to create the link.
 *
 * @return The status of the parameters validity
 *
 ****************************************************************************************
 */
uint8_t llm_create_con(struct llm_le_create_con_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the command start encryption.
 *
 * This function sets the key and the data to be encrypted in the encryption engine and
 * start it.
 *
 * @param[in] param        Pointer on the structure which contains all the parameters
 *                         needed to star tthe encryption engine.
 *
 ****************************************************************************************
 */
void llm_encryption_start(struct llm_le_enc_cmd const *param);

/**
 ****************************************************************************************
 * @brief Handle the end of the encryption process.
 *
 * This function provide to the host or the LLC the data encrypted by the engine.
 *
 ****************************************************************************************
 */
void llm_encryption_done(void);

/**
 ****************************************************************************************
 * @brief Sends the command complete event.
 *
 * This function notify the host that the command is completed.
 *
 * @param[in] msg_id        Opcode the command
 * @param[in] dest_id       Destination of the event (Source of the command).
 * @param[in] src_id        Source of the command (Destination of the command).
 * @param[in] status        Status on the completion of the command.
 ****************************************************************************************
 */
void llm_common_cmd_complete_send(ke_msg_id_t msg_id,
                                  ke_task_id_t dest_id,
                                  ke_task_id_t src_id,
                                  uint8_t status);

/**
 ****************************************************************************************
 * @brief Sends the command status event.
 *
 * This function notify the host that the command is understood.
 *
 * @param[in] msg_id        Opcode the command
 * @param[in] dest_id       Destination of the event (Source of the command).
 * @param[in] src_id        Source of the command (Destination of the command).
 * @param[in] status        Status on the understanding of the command.
 ****************************************************************************************
 */
void llm_common_cmd_status_send(ke_msg_id_t msg_id,
                                  ke_task_id_t dest_id,
                                  ke_task_id_t src_id,
                                  uint8_t status);

/**
 ****************************************************************************************
 * @brief Sets the value for the advertising report.
 *
 * This function gets the information from the received advertising packet and sets the
 * values in the advertising report event.
 *
 * @param[out] desc        Pointer on the received advertising.
 * @param[out] event       Pointer on the advertising report event.
 *
 ****************************************************************************************
 */
void llm_adv_report_set(struct llm_le_adv_report_evt *event, struct co_buf_rx_desc *desc);

/**
 ****************************************************************************************
 * @brief Handles the connection request pdu.
 *
 * This function extracts the parameters from the packet received and takes the
 * associated actions (new pdu generation, new state , parameters update, etc...).
 *
 * @param[in] rxdesc         Pointer on the received pdu packet.
 *
 ****************************************************************************************
 */
void llm_con_req_ind(struct co_buf_rx_desc *rxdesc);

#if (BLE_CENTRAL || BLE_OBSERVER)
/**
 ****************************************************************************************
 * @brief Handles the advertising packet.
 *
 * This function extracts the parameters from the advertising packet received and
 * generates the event if needed
 *
 * @param[in] rxdesc         Pointer on the received advertising packet.
 *
 ****************************************************************************************
 */
void llm_le_adv_report_ind(struct co_buf_rx_desc *rxdesc);
#endif // BLE_CENTRAL || BLE_OBSERVER

/**
 ****************************************************************************************
 * @brief Handles the transmission confirmation of the packet.
 *
 * This function sends the connection complete event to the host and start the LLC task
 * for the dedicated conhdl.
 *
 * @param[in] rxdesc         Pointer on the received advertising packet who has generated
 *                           the sending of the connect_req pdu.
 *
 ****************************************************************************************
 */
void llm_con_req_tx_cfm(struct co_buf_rx_desc *rxdesc);

#if (BLE_CHNL_ASSESS)
/**
 ****************************************************************************************
 * @brief This function returns the number of packets to receive on a channel before being
 * able to disable a channel.
 *
 ****************************************************************************************
 */
uint16_t llm_get_chnl_assess_nb_pkt(void);

/**
 ****************************************************************************************
 * @brief This function returns the number of bad packets to receive on a channel before being
 * able to disable a channel.
 *
 ****************************************************************************************
 */
uint16_t llm_get_chnl_assess_nb_bad_pkt(void);

/**
 ****************************************************************************************
 * @brief This function returns the minimal RSSI value used in the Channel Assessment
 * mechanism.
 *
 ****************************************************************************************
 */
int8_t llm_get_min_rssi(void);
#endif //(BLE_CHNL_ASSESS)


/**
 * Delete the connection less event.
 * remove requested by LLD
 */
void llm_delete_event(void);

/// @} LLM

#endif // LLM_H_
