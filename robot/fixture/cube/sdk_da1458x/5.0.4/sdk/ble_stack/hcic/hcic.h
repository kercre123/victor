/**
 ****************************************************************************************
 *
 * @file hcic.h
 *
 * @brief This file contains definitions related to the HCI module.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef HCI_H_
#define HCI_H_

/**
 ****************************************************************************************
 * @addtogroup HCI HCI
 * @ingroup ROOT
 * @brief HCI module handling communication between lower and higher layers in split
 * architecture.
 *
 *@{
 * This is the following convention for HCI transport functions:
 *     - Functions referring to Commands/events to be sent through HCI transport layer
 *       contain 'send' in their name.
 *
 *     - Functions referring to Commands/events to be from HCI transport layer reception
 *       to FW targets contain 'dispatch' in their name.
 *
 *     - Suffixes 'pk' and 'unpk' are used for command/event packing and unpacking
 *       functions, which generally bear a similar name to the command name in the SIG
 *       specification.
 *
 *     - All commands and events are described by structures, defined in llm/c_task.h.
 *       The structures may sometimes present padded space for alignment, therefore
 *       for those messages described with such structures, specific packing/unpacking
 *       functions can be found. All the rest are handled by 'basic' or 'aligned'
 *       named functions.
 *
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_HCIC_ITF)

#include <stddef.h>          // standard definition
#include <stdint.h>          // standard integer
#include "llm_task.h"        // link layer manager task declaration
#include "llc_task.h"        // link layer controller task declaration
#include "dbg_task.h"        // debug task declaration
#include "hcic_msg.h"         // hci message definition
#include "hcic_eif.h"        // hci uart definition


/*
 * DEFINES
 ****************************************************************************************
 */
/// HCI command packets
#define HCI_NB_CMD_PKTS            10

///Maximum size for the command parameters
#define HCI_MAX_CMD_LENGTH         0x20

///Maximum size for the trash buffer
#define HCI_MAX_TRASH_LENGTH       38

///Maximum number of HCI commands
#if (RW_BLE_WLAN_COEX)
#if (RW_BLE_WLAN_COEX_TEST)
#define HCI_CMD_OPCODE_NB_MAX      73
#else
#define HCI_CMD_OPCODE_NB_MAX      72
#endif //RW_BLE_WLAN_COEX_TEST
#else
///Maximum number of HCI commands
#define HCI_CMD_OPCODE_NB_MAX      71
#endif //RW_BLE_WLAN_COEX

///Maximum length for LLM command Message IDs related tables
#define LLM_CMDID_MAX              (LLM_MSG_ID_EVT_FIRST - LLM_MSG_ID_CMD_FIRST)

///Maximum length for LLC command Message IDs related tables
#define LLC_CMDID_MAX              (LLC_MSG_ID_EVT_FIRST - LLC_MSG_ID_CMD_FIRST)

///Maximum length for DBG command Message IDs related tables
#define DBG_CMDID_MAX              (DBG_MSG_ID_EVT_FIRST - DBG_MSG_ID_CMD_FIRST)

///Maximum length for LLM event Message IDs related tables
#define LLM_EVTID_MAX              (LLM_MSG_ID_EVT_LAST - LLM_MSG_ID_EVT_FIRST)

///Maximum length for LLC event Message IDs related tables
#define LLC_EVTID_MAX              (LLC_MSG_ID_EVT_LAST - LLC_MSG_ID_EVT_FIRST)

///Maximum length for LLC event Message IDs related tables
#define DBG_EVTID_MAX              (DBG_MSG_ID_EVT_LAST - DBG_MSG_ID_EVT_FIRST)

/*
 * MACROS
 ****************************************************************************************
 */

/// Macro converting an LLM command Message ID into a table index
#define LLM_CMDID2IDX(msgid)       ((msgid) - LLM_MSG_ID_CMD_FIRST)

/// Macro converting an LLC command Message ID into a table index
#define LLC_CMDID2IDX(msgid)       ((msgid) - LLC_MSG_ID_CMD_FIRST)

/// Macro converting a DBG command Message ID into a table index
#define DBG_CMDID2IDX(msgid)       ((msgid) - DBG_MSG_ID_CMD_FIRST)

/// Macro converting an LLM event Message ID into a table index
#define LLM_EVTID2IDX(msgid)       ((msgid) - LLM_MSG_ID_EVT_FIRST)

/// Macro converting an LLC event Message ID into a table index
#define LLC_EVTID2IDX(msgid)       ((msgid) - LLC_MSG_ID_EVT_FIRST)

/// Macro converting a DBG event Message ID into a table index
#define DBG_EVTID2IDX(msgid)       ((msgid) - DBG_MSG_ID_EVT_FIRST)

/// Macro to extract OCF from OPCODE
#define HCI_OP2OCF(opcode)        ((opcode) & 0x03FF)

/// Macro to extract OGF from OPCODE
#define HCI_OP2OGF(opcode)        ((opcode) >> 10 & 0x003F)

/// Macro to create OPCODE from OGF and OCF
#define HCI_OPCODE(ocf, ogf)      (((ogf) << 10) | ocf)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

///HCI enumeration of possible Command OGF values.
enum
{
    ///HCI Link Control Commands Group OGF code
    LK_CNTL_OGF = 0x01,
    ///HCI Link Policy Commands Group OGF code
    LK_POL_OGF,
    ///HCI Controller and Baseband Commands Group OGF code
    CNTLR_BB_OGF,
    ///HCI Information Parameters Commands Group OGF code
    INFO_PAR_OGF,
    ///HCI Status Commands Group OGF code
    STAT_PAR_OGF,
    ///HCI Test Commands Group OGF code
    TEST_OGF,
    ///HCI Low Energy Commands Group OGF code
    LE_CNTLR_OGF=0x08,
    ///HCI Debug Commands Group OGF code
    DBG_OGF = 0x3F,
    MAX_OGF
};

///HCI TX states
enum HCI_STATE_TX
{
     ///HCI TX Start State - when packet is ready to be sent
     HCI_STATE_TX_ONGOING,
     ///HCI TX Done State - TX ended with no error
     HCI_STATE_TX_IDLE,
};

///HCI RX states
enum HCI_STATE_RX
{
    ///HCI RX Start State - receive message type
    HCI_STATE_RX_START,
    ///HCI RX Header State - receive message header
    HCI_STATE_RX_HDR,
    ///HCI RX Header State - receive (rest of) message payload
    HCI_STATE_RX_PAYL,
    ///HCI RX Out Of Sync state - receive message type
    HCI_STATE_RX_OUT_OF_SYNC
};

///Event types for HCI message handler to determine pointer to start of packed event
///in the kernel message space.
enum
{
    ///Command Complete Event type will have packed event starting at ke_msg->dest_id +1
    TYPE_CCEVT = -5,
    ///Command Status Event type will have packed event starting at ke_msg->dest_id +1
    TYPE_CSEVT = -5,
    ///Rest of the events will have packed event starting at ke_msg->param_len
    TYPE_OTHEREVT = -2,
};


///HCI command unpack function pointer type definition
typedef void (*hci_cmd_unpk_func_t)(uint8_t * bufptr);

///HCI event pack function pointer type definition
typedef int  (*hci_evt_pk_func_t)(struct ke_msg * msg);


///HCI event destination and source structure
struct hci_evt_pk_util
{
    ///Event type: Command Complete or other, for pointer position in msg_send_handler
    int8_t  evt_type;
    ///Event parameter length
    uint8_t  parlen;
    ///Event opcode - when valid, for CCEVT or CSEVT
    uint16_t opcode;
    ///Pointer to event packing handler
    hci_evt_pk_func_t func;
};



///HCI Command header components structure
struct hci_cmd_hdr
{
    ///OGF - command group code
    uint8_t ogf;
    ///OCF - command index in group
    uint8_t ocf;
    ///Flag for opcode known or not at its reception in LL
    uint8_t known_opcode;
    ///Parameter length - the number of bytes of the command parameters
    uint8_t parlen;
};

///HCI ACL data packets header structure
struct hci_acl_hdr
{
    ///Connection handle
    uint16_t hdl;
    ///Broadcasts Flag (always 0) and Packet Boundary Flag - first packet or fragment ind
    uint8_t  bcpb_flag;
    ///Data length in number of bytes
    uint16_t datalen;
    ///Pointer to tx descriptor allocated for current lower layer reception
    struct co_buf_tx_node * txtag;
    ///Pointer to rx descriptor allocated for current higher layer reception
    struct co_buf_rx_desc * rxtag;
};

/// HCI out of synchronization recovery variables
struct hci_out_of_sync_tag
{
    uint8_t byte;
    uint8_t index;
};

///HCI Environment context structure
struct hci_env_tag
{
    /// pointer to External interface api
    const struct rwip_eif_api* ext_if;

    ///Rx state - can be receiving message type, header, payload or error
    uint8_t  rx_state;
    ///Tx state - either transmitting, done or error.
    uint8_t  tx_state;

    ///Allowed number of commands from Host to controller (sent from controller to host)
    uint8_t  nb_h2c_cmd_pkts;
    ///Latest received message type: CMD/EVT/ACL.
    uint8_t  curr_msg_type;
    ///Latest received message header, 4 byte buffer for eother configuration.
    uint8_t  curr_hdr_buff[4];
    ///Pointer to space reserved for received payload.
    uint8_t *curr_payl_buff;
    ///Space reserved for unknown commands reception , with params;
    uint8_t  unk_cmd_buf[HCI_MAX_TRASH_LENGTH];
    ///Space reserved for unknown command ccevt to send;
    uint8_t  unk_cmd_ccevt_buf[HCI_TRANSPORT_HDR_LEN + HCI_EVT_HDR_LEN+4];

    ///Last command header received
    struct   hci_cmd_hdr chdr;
    ///Last ACL header received
    struct   hci_acl_hdr ahdr;

    ///Queue of kernel messages corresponding to packets sent through HCI
    struct co_list queue_tx;

    /// HCI synchronization error parameters
    struct hci_out_of_sync_tag out_of_sync;
};


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
///HCI environment structure external global variable declaration
extern struct hci_env_tag hci_env;

extern const uint16_t hci_cmd_opcodes[HCI_CMD_OPCODE_NB_MAX];

extern const struct hci_evt_pk_util hci_llm_epk[LLM_EVTID_MAX];
#if (BLE_CENTRAL || BLE_PERIPHERAL)
extern const struct hci_evt_pk_util hci_llc_epk[LLC_EVTID_MAX];
#endif // (BLE_CENTRAL || BLE_PERIPHERAL)
#if (BLE_DEBUG)
extern const struct hci_evt_pk_util hci_dbg_epk[DBG_EVTID_MAX];
#endif //BLE_DEBUG


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
****************************************************************************************
* @brief HCI initialization function: initializes states and transport.
*
* @param[in] eif External Interface API
*****************************************************************************************
*/
void hci_init(const struct rwip_eif_api* eif);


/**
 ****************************************************************************************
 * @brief Check received HCI command size
 *
 * Size is checked using tables storing parameter size for each known command.
 *
 * Because some commands have variable size (e.g. HCI_Set_event_Filter,
 * HCI_Write_Stored_Link_Key, HCI_Write_Location_Data), and to avoid processing variable
 * sizes as specific cases, received size is considered ok if less or equal maximum size
 * defined by standard.
 *
 * @return 0 : size not ok - 1 : size ok
 *
 *****************************************************************************************
 */
bool hci_cmd_parameter_size_check(void);

/**
 ****************************************************************************************
 * @brief Check received byte in out of sync state
 *
 * This function is the algorithm to check that received byte stream in out of sync state
 * corresponds to HCI_reset command.
 *
 * Level of reception is incremented when bytes of HCI_reset_cmd are detected.
 *
 *****************************************************************************************
 */
void hci_out_of_sync_check(void);

/**
 ****************************************************************************************
 * @brief Function called in HCIH EIF reception routine to allocate a KE message for a
 *        command.
 *
 * Uses the hci_env command header OGF and OCF components.
 *
 * @return Pointer to the allocated kernel message parameter space.
 *****************************************************************************************
 */
uint8_t *hci_cmd_alloc(void);

/**
 ****************************************************************************************
 * @brief Function called in HCIH EIF reception routine to allocate a KE message for a
 *        data.
 *****************************************************************************************
 */
void hci_data_alloc(void);
/**
 ****************************************************************************************
 * @brief Function called in HCIH EIF reception routine to dispatch a KE message for a
 *        command with no parameters.
 *
 * Uses the hci_env command header OGF and OCF components.
 *****************************************************************************************
 */
void hci_cmd_dispatch_basic(void);


/**
 ****************************************************************************************
 * @brief Function called in HCIH EIF reception routine to dispatch a KE message for a
 *        command.
 *
 * Uses the hci_env command header OGF and OCF components.
 *
 * @param[in]  payl Pointer to receiver buffer payload
 *****************************************************************************************
 */
void hci_cmd_dispatch(uint8_t * payl );


/**
 ****************************************************************************************
 * @brief Function called in HCIH EIF reception routine to dispatch a KE message for data
 *        reception, once payload reception has been completed (for max BLE_DATA_LEN).
 *
 * ***************************************************************************************
 */
void hci_data_dispatch(uint8_t * payl);

/**
 ****************************************************************************************
 * @brief Trigger sending of HCI event
 *
 *  Function called to trigger sending of HCI event in current event buffer.
 *
 * @param[in]  Kernel message carrying HCI event
 *****************************************************************************************
 */
void hci_push(struct ke_msg *msg);

/**
 ****************************************************************************************
 * @brief Restart HCI flow to exit from sleep mode
 *****************************************************************************************
 */
void hci_exit_sleep(void);

#endif //BLE_HCIC_ITF

/**
 ****************************************************************************************
 * @brief Stop HCI flow to enter in sleep mode
 *
 * WARNING FOR CHIPS WITHOUT EXTERNAL WAKE UP !!!
 * This function also disable the RTS signal on the UART side if UART is used as physical
 * interface.
 * After sleep, RTS should be re-enabled.
 *
 * @return true if HCI has entered sleep, false otherwise
 *****************************************************************************************
 */
bool hci_enter_sleep(void);

/// @} HCI

#endif // HCI_H_
