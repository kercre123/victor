/**
 ****************************************************************************************
 *
 * @file hcih.h
 *
 * @brief This file contains definitions related to the HCI module.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef HCIH_H_
#define HCIH_H_

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

#if (BLEHL_HCIH_ITF)

#include <stddef.h>          // standard definition
#include <stdint.h>          // standard integer
#include "llm_task.h"        // link layer manager task declaration
#include "llc_task.h"        // link layer controller task declaration
#include "dbg_task.h"        // debug task declaration
#include "hcih_msg.h"         // hci message definition
#include "hcih_eif.h"        // hci uart definition

/*
 * DEFINES
 ****************************************************************************************
 */
/// HCI command packets
#define HCIH_NB_CMD_PKTS            10

///Maximum size for the command parameters
#define HCIH_MAX_CMD_LENGTH         0x20

///Maximum size for the trash buffer
#define HCIH_MAX_TRASH_LENGTH       38

///Maximum number of HCI commands
#if (BLEHL_WLAN_COEX)
#if (BLEHL_WLAN_COEX_TEST)
#define HCIH_CMD_OPCODE_NB_MAX      72
#else
#define HCIH_CMD_OPCODE_NB_MAX      71
#endif //BLEHL_WLAN_COEX_TEST
#else
///Maximum number of HCI commands
#define HCIH_CMD_OPCODE_NB_MAX      70
#endif //BLEHL_WLAN_COEX

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
#define HCIH_OP2OCF(opcode)        ((opcode) & 0x03FF)

/// Macro to extract OGF from OPCODE
#define HCIH_OP2OGF(opcode)        ((opcode) >> 10 & 0x003F)

/// Macro to create OPCODE from OGF and OCF
#define HCIH_OPCODE(ocf, ogf)      (((ogf) << 10) | ocf)

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


///HCI command pack function pointer type definition
typedef int  (*hcih_cmd_pk_func_t)(struct ke_msg * msg);

///HCI event unpack function pointer type definition
typedef void (*hcih_evt_unpk_func_t)(uint8_t * bufptr);


///HCI command packing details structure
struct hcih_cmd_pk_util
{
    ///Command opcode
    uint16_t opcode;
    ///Command parameter length
    uint8_t  parlen;
    ///Command packing handler
    hcih_cmd_pk_func_t func;
};

///HCI event unpacking details structure
struct hcih_evt_unpk_util
{
    ///Message ID of event
    uint16_t msgid;
    ///Destination of event
    uint16_t destid;
    ///Size of event parameters
    uint16_t size;
    ///Pointer to event unpack handler
    hcih_evt_unpk_func_t func;
};

///HCI Command header components structure
struct hcih_cmd_hdr
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

///HCI Event header components structure - contains all details possible in an event
struct hcih_evt_hdr
{
    ///Event code
    uint8_t  code;
    ///Event parameters length
    uint8_t  parlen;
    ///Event subcode if le event, otherwise invalid
    uint8_t  subcode;
    ///OGF of command which it corresponds to - if CCEVT
    uint8_t  ogf;
    ///OCF of command which it corresponds to - if CCEVT
    uint8_t  ocf;
    ///Status for command status events
    uint8_t  status;

};

///HCI ACL data packets header structure
struct hcih_acl_hdr
{
    ///Connection handle
    uint16_t hdl;
    ///Broadcasts Flag (always 0) and Packet Boundary Flag - first packet or fragment ind
    uint8_t  bcpb_flag;
    ///Data length in number of bytes
    uint16_t datalen;
    ///Pointer to tx descriptor allocated for current lower layer reception
    struct co_buf_tx_node * txtag;
    /// Index of the RX descriptor
    uint8_t rxhdl;
};

///HCI Environment context structure
struct hcih_env_tag
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
    ///Last event header received
    struct   hcih_evt_hdr ehdr;
    ///Last ACL header received
    struct   hcih_acl_hdr ahdr;

    ///Queue of kernel messages corresponding to packets sent through HCI
    struct co_list queue_tx;

    ///Index of the current free RX buffer
    uint8_t rx_current;
};


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
///HCI environment structure external global variable declaration
extern struct hcih_env_tag hcih_env;

extern const uint16_t hcih_cmd_opcodes[HCIH_CMD_OPCODE_NB_MAX];

//For Higher layers use
extern const struct hcih_cmd_pk_util hcih_llm_cpk[LLM_CMDID_MAX];
#if (BLE_CENTRAL || BLE_PERIPHERAL)
extern const struct hcih_cmd_pk_util hcih_llc_cpk[LLC_CMDID_MAX];
#endif // (BLE_CENTRAL || BLE_PERIPHERAL)
#if (BLE_DEBUG)
extern const struct hcih_cmd_pk_util hcih_dbg_cpk[DBG_CMDID_MAX];
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
void hcih_init(const struct rwip_eif_api* eif);


/**
 ****************************************************************************************
 * @brief Trigger sending of HCI command
 *
 *  Function called to trigger sending of HCI command in current event buffer.
 *
 * @param[in]  Kernel message carrying HCI command
 *****************************************************************************************
 */
void hcih_push(struct ke_msg *msg);


/**
 ****************************************************************************************
 * @brief Function called in HCIH EIF reception routine to allocate a KE message for a
 *        command.
 *
 * Uses the hcih_env command header OGF and OCF components.
 *
 * @return Pointer to the allocated kernel message parameter space.
 *****************************************************************************************
 */
uint8_t *hcih_cmd_alloc(void);

/**
 ****************************************************************************************
 * @brief Function called in HCIH EIF reception routine to allocate a KE message for a
 *        data.
 *
 * @param[in] length length of the data buffer to allocate.
 *
 *****************************************************************************************
 */
void hcih_data_alloc(uint16_t length);
/**
 ****************************************************************************************
 * @brief Function called in HCIH EIF reception routine to dispatch a KE message for a
 *        command with no parameters.
 *
 * Uses the hcih_env command header OGF and OCF components.
 *****************************************************************************************
 */
void hcih_cmd_dispatch_basic(void);


/**
 ****************************************************************************************
 * @brief Function called in HCIH EIF reception routine to dispatch a KE message for a
 *        command.
 *
 * Uses the hcih_env command header OGF and OCF components.
 *
 * @param[in]  payl Pointer to receiver buffer payload
 *****************************************************************************************
 */
void hcih_cmd_dispatch(uint8_t * payl );


/**
 ****************************************************************************************
 * @brief Function called in HCIH EIF reception routine to allocate a KE message for an
 *        event.
 *
 * Uses the hcih_env event header code component.
 *
 * @return Pointer to the allocated kernel message parameter space.
 *****************************************************************************************
 */
uint8_t * hcih_evt_alloc(void);

/**
 ****************************************************************************************
 * @brief Function called in HCIH EIF reception routine to allocate and dispatch a KE
 *        message for a command status event.
 *
 *****************************************************************************************
 */
void hcih_csevt_dispatch(void);

/**
 ****************************************************************************************
 * @brief Function called in HCIH EIF reception routine to dispatch a KE message for all
 *      events except the command status event, once payload reception has been completed.
 *
 * @param[in]  payl Pointer to receiver buffer payload
 *
 *****************************************************************************************
 */
void hcih_evt_dispatch(uint8_t * payl);

/**
 ****************************************************************************************
 * @brief Function called in HCIH EIF reception routine to dispatch a KE message for data
 *        reception, once payload reception has been completed (for max BLE_DATA_LEN).
 *
 * ***************************************************************************************
 */
void hcih_data_dispatch(uint8_t * payl);

#endif //BLEHL_HCIH_ITF

/// @} HCI

#endif // HCIH_H_
