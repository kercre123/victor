/**
 ****************************************************************************************
 *
 * @file atts_util.h
 *
 * @brief Header file - ATTSUTIL.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef ATTS_UTIL_H_
#define ATTS_UTIL_H_

/**
 ****************************************************************************************
 * @addtogroup ATTSUTIL Utility
 * @ingroup ATTS
 * @brief Contains utility functions and macros for ATTS
 *
 * The ATTSUTIL is responsible for providing utility functions for
 * @ref ATTS "ATTS", in order to accomplish defined Attribute Server
 * functionalities.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "atts.h"
#include "l2cm.h"
#include "l2cc_task.h"
#include "l2cm_util.h"
#include "co_error.h"
#include "co_bt.h"
#include "co_buf.h"
#include "co_utils.h"
#include "gap.h"
#if (BLE_ATTS)
/*
 * DEFINES
 ****************************************************************************************
 */
/* Attribute server fixed PDU lengths */
/// PDU size  MTU exchange response
#define ATT_MTU_EXCHANGE_RESP_LEN               0x03
/// PDU size for write response
#define ATT_WRITE_RESP_LEN                      0x01
/// PDU size for handle value confirmation
#define ATT_HANDLE_VALUE_CONFIRMATION_LEN       0x01
/// Number of searched attribute element
#define ATT_SEARCH_RESULT                       0x0A
/// Characteristic element size
#define ATT_CHAR_ELMT_SIZE                      0x05
/// Service element size
#define ATT_SVC_ELMT_SIZE                       0x02

/// Access operation to attribute element
enum
{
    /// Read type of access to element
    ATT_READ_ACCESS = 0x00,
    /// Write type of access to element
    ATT_WRITE_ACCESS,
    /// Write signed type of access to element
    ATT_WRITE_SIGNED_ACCESS,
    /// Notify type of access to element
    ATT_NOTIFY_ACCESS,
    /// Indication type of access to element
    ATT_INDICATE_ACCESS,
};
/*
 * MACRO DEFINITIONS
 ****************************************************************************************
 */


/// Allocate a PDU packet for a specific PDU type.
#define ATTS_ALLOCATE_PDU(conidx, opcode, pdu_type, value_len)\
    (struct pdu_type*) atts_allocate_pdu(conidx, opcode, value_len)


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @brief Send a PDU Attribute packet through L2CAP
 *
 * @param[in] pdu        PDU Packet
 ****************************************************************************************
 */
void atts_send_pdu(void *pdu);

/**
 ****************************************************************************************
 * @brief Handles the MTU exchange request and constructs response.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] req           Message request
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
uint8_t atts_mtu_exc_resp(uint8_t conidx, struct l2cc_att_mtu_req* req);

/**
 ****************************************************************************************
 * @brief Handles read info request and constructs response.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] req           Message request
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
uint8_t atts_find_info_resp(uint8_t conidx, struct l2cc_att_find_info_req* req);

/**
 ****************************************************************************************
 * @brief Handles find by type request and constructs response.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] req           Message request
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
uint8_t atts_find_by_type_resp(uint8_t conidx, struct l2cc_att_find_by_type_req* req);

/**
 ****************************************************************************************
 * @brief Handles read by type request and constructs response.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] req           Message request
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
uint8_t atts_read_by_type_resp(uint8_t conidx, struct l2cc_att_rd_by_type_req* req);

/**
 ****************************************************************************************
 * @brief Handles read by group type request and constructs response.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] req           Message request
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
uint8_t atts_read_by_grp_type_resp(uint8_t conidx, struct l2cc_att_rd_by_grp_type_req* req);

/**
 ****************************************************************************************
 * @brief Handles read request and constructs response.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] req           Message request
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
uint8_t atts_read_resp(uint8_t conidx, struct l2cc_att_rd_req* req);

/**
 ****************************************************************************************
 * @brief Handles read blob request and constructs response.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] req           Message request
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
uint8_t atts_read_blob_resp(uint8_t conidx, struct l2cc_att_rd_blob_req* req);

/**
 ****************************************************************************************
 * @brief Handles read multiple handle request and constructs response.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] req           Message request
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
uint8_t atts_read_mult_resp(uint8_t conidx, struct l2cc_att_rd_mult_req* req);

/**
 ****************************************************************************************
 * @brief Handles write no response and updates the attribute server.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] cmd           Message command
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
uint8_t atts_write_no_resp(uint8_t conidx, struct l2cc_att_wr_cmd* cmd);

/**
 ****************************************************************************************
 * @brief Handles write response and constructs response.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] req           Message request
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
uint8_t atts_write_resp(uint8_t conidx, struct l2cc_att_wr_req* req);

/**
 ****************************************************************************************
 * @brief Handles signed write command.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] cmd           Message command
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
uint8_t atts_signed_write_resp(uint8_t conidx, struct l2cc_att_sign_wr_cmd* cmd);

/**
 ****************************************************************************************
 * @brief Handles prepare write request and constructs response.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] req           Message request
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
uint8_t atts_prepare_write_resp(uint8_t conidx, struct l2cc_att_prep_wr_req* req);

/**
 ****************************************************************************************
 * @brief Handles execute write request and constructs response.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] req           Message request
 *
 * @return If the message was consumed or not.
 *
 ****************************************************************************************
 */
uint8_t atts_execute_write_resp(uint8_t conidx, struct l2cc_att_exe_wr_req* req);

/**
 ****************************************************************************************
 * @brief Send an attribute error response to peer.
 *
 * @param[in] conidx            Index of the connection with the peer device
 * @param[in] opcode            failing operation code
 * @param[in] uuid              attribute UUID
 * @param[in] error             error code
 ****************************************************************************************
 */
void atts_send_error(void *pdu, uint8_t opcode, uint16_t uuid, uint8_t error);

/**
 ****************************************************************************************
 * @brief Send Command complete message.
 *
 * @param[in] conidx        Index of the connection with the peer device
 * @param[in] status        status code
 *
 *****************************************************************************************
 */
void atts_send_cmd_cmp(uint8_t conidx, uint8_t status);


/**
 ****************************************************************************************
 * @brief Retrieve attribute and checks if a peer device has enough permission rights
 *        to perform the access to the specified attribute
 *
 * @param[in]  conidx   Index of the connection with the peer device
 * @param[in]  access   Type of access to be performed (read, write, indication/notify)
 * @param[in]  handle   Handle of the attribute to be accessed
 * @param[out] attm_elmt Attribute pointer to return
 *
 * @return @ref ATT_ERR_NO_ERROR if access is permitted, otherwise the ATT error code to
 * be put in the error response frame
 *
 *****************************************************************************************
 */
uint8_t atts_get_att_chk_perm(uint8_t conidx, uint8_t access, uint16_t handle,
                              struct attm_elmt** attm_elmt);

/**
 ****************************************************************************************
 * @brief Format the Write Response PDU and send it after receiving a Write Request PDU
 * @param[in] conidx Index of the connection with the peer device
 * @param[in] atthdl  Attribute handle for which to send the response
 * @param[in] status  ATT error code
 ****************************************************************************************
 */
void atts_write_rsp_send(uint8_t conidx, uint16_t atthdl, uint8_t status);

/**
 ****************************************************************************************
 * @brief Allocate PDU packet to sent through L2Cap
 *
 * @param[in] conidx      Index of the connection with the peer device
 * @param[in] opcode      Operation code of the PDU packet to send.
 * @param[in] value_len   Total length of the value
 ****************************************************************************************
 */
void* atts_allocate_pdu(uint8_t conidx, uint8_t opcode, uint16_t value_len);

#endif /* #if (BLE_ATTS) */
/// @} ATTSUTIL
#endif // ATTS_UTIL_H_
