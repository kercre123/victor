/**
 ****************************************************************************************
 *
 * @file l2cc_pdu.h
 *
 * @brief Header file - L2CAP Controller PDU packer / unpacker
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _L2CC_PDU_H_
#define _L2CC_PDU_H_

/**
 ****************************************************************************************
 * @addtogroup L2CC_PDU L2Cap Controller PDU generator/extractor
 * @ingroup L2CC
 * @brief This module is in charge to pack or unpack L2CAP packets
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_L2CC)

#include "l2cc.h"
#include "co_bt.h"
#include "att.h"
#include "ke_task.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Minimal authorized MTU value (defined by Bluetooth SIG)
#define L2C_MIN_LE_MTUSIG                   (23)

/// L2CAP mode supported for BLE - only mode accepted
#define L2C_MODE_BASIC_L2CAP                (0x00)

/// Packet partition
#define L2C_CID_LEN                         (2)
#define L2C_LENGTH_LEN                      (2)
#define L2C_CODE_LEN                        (1)
#define L2C_HEADER_LEN                      (L2C_CID_LEN + L2C_LENGTH_LEN)

/* result for connection parameter update response */
#define L2C_CONN_PARAM_ACCEPT               0x0000
#define L2C_CONN_PARAM_REJECT               0x0001

/* command reject reasons */
#define L2C_CMD_NOT_UNDERSTOOD              0x0000
#define L2C_MTU_SIG_EXCEEDED                0x0001
#define L2C_INVALID_CID                     0x0002

/* L2CAP Connection Parameter Update:
        ConnIntervalMin = Interval Min * 1.25 ms. 6 to 3200.
        Interval Min shall be less than or equal Interval Max.
*/
#define L2C_MIN_CONN_INT                    0x07D0
/* 1600: 2 secs conn int    */
#define L2C_MIN_CONN_EVT_INT                0x0640
/*
connIntervalMax = Interval Min * 1.25 ms. 6 to 3200. Interval
Max shall be equal to or greater than the Interval Min
*/
/* 4000 ms */
#define L2C_MAX_CONN_INT                    0x0FA0
/* 3200: 4 secs conn int    */
#define L2C_MAX_CONN_EVT_INT                0x0C80
#define L2C_LATENCY                         0x0640
#define L2C_TIMEOUT                         0x0F00
#define L2C_MAX_NB_OF_CHANNEL               0x03
/* Flag to describe the packet boundary */
#define L2C_PB_START_NON_FLUSH              0x00
#define L2C_PB_CONTINUE                     0x01
#define L2C_PB_START_FLUSH                  0x02

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/** Fixed L2CAP channels id */
enum l2cc_cid
{
    /// Reserved channel id
    L2C_CID_RESERVED                   = 0x0000,
    /// Attribute channel id
    L2C_CID_ATTRIBUTE                  = 0x0004,
    /// Signaling channel id
    L2C_CID_LE_SIGNALING               = 0x0005,
    /// Security channel id
    L2C_CID_SECURITY                   = 0x0006,
};


/// signaling codes
enum l2cc_signal_code
{
    /// Reserved code
    L2C_CODE_RESERVED                  = 0x00,
    /// Reject request
    L2C_CODE_REJECT                    = 0x01,
    /// Connection request
    L2C_CODE_CONNECTION_REQ            = 0x02,
    /// Connection response
    L2C_CODE_CONNECTION_RESP           = 0x03,
    /// Configuration request
    L2C_CODE_CONFIGURATION_REQ         = 0x04,
    /// Configuration response
    L2C_CODE_CONFIGURATION_RESP        = 0x05,
    /// Disconnection request
    L2C_DISCONNECTION_REQ              = 0x06,
    /// Disconnection response
    L2C_DISCONNECTION_RESP             = 0x07,
    /// Echo request
    L2C_ECHO_REQ                       = 0x08,
    /// Echo response
    L2C_ECHO_RESP                      = 0x09,
    /// Information request
    L2C_INFORMATION_REQ                = 0x0A,
    /// Information response
    L2C_INFORMATION_RESP               = 0x0B,
    /// Create channel request
    L2C_CREATE_CHANNEL_REQ             = 0x0C,
    /// Create channel response
    L2C_CREATE_CHANNEL_RESP            = 0x0D,
    /// Move channel request
    L2C_MOVE_CHANNEL_REQ               = 0x0E,
    /// Move channel response
    L2C_MOVE_CHANNEL_RESP              = 0x0F,
    /// Move channel confirmation
    L2C_MOVE_CHANNEL_CFM               = 0x10,
    /// Move channel confirmation response
    L2C_MOVE_CHANNEL_CFM_RESP          = 0x11,
    /// Connection Parameter Update Request
    L2C_CODE_CONN_PARAM_UPD_REQ        = 0x12,
    /// Connection Parameter Update Response
    L2C_CODE_CONN_PARAM_UPD_RESP       = 0x13,

    /// max number of signaling codes
    L2C_CODE_SIGNALING_MAX,
};


/// security codes
enum l2cc_security_code
{
    /// Pairing Request
    L2C_CODE_PAIRING_REQUEST              = 0x01,
    /// Pairing Response
    L2C_CODE_PAIRING_RESPONSE             = 0x02,
    /// Pairing Confirm
    L2C_CODE_PAIRING_CONFIRM              = 0x03,
    /// Pairing Random
    L2C_CODE_PAIRING_RANDOM               = 0x04,
    /// Pairing Failed
    L2C_CODE_PAIRING_FAILED               = 0x05,
    /// Encryption Information
    L2C_CODE_ENCRYPTION_INFORMATION       = 0x06,
    /// Master Identification
    L2C_CODE_MASTER_IDENTIFICATION        = 0x07,
    /// Identity Information
    L2C_CODE_IDENTITY_INFORMATION         = 0x08,
    /// Identity Address Information
    L2C_CODE_IDENTITY_ADDRESS_INFORMATION = 0x09,
    /// Signing Information
    L2C_CODE_SIGNING_INFORMATION          = 0x0A,
    /// Security Request
    L2C_CODE_SECURITY_REQUEST             = 0x0B,

    /// max number of security codes
    L2C_CODE_SECURITY_MAX,
};


/// attribute protocol PDU codes
enum l2cc_attribute_code
{
    /// Error response
    L2C_CODE_ATT_ERR_RSP            = 0x01,
    /// Exchange MTU Request
    L2C_CODE_ATT_MTU_REQ            = 0x02,
    /// Exchange MTU Response
    L2C_CODE_ATT_MTU_RSP            = 0x03,
    /// Find Information Request
    L2C_CODE_ATT_FIND_INFO_REQ      = 0x04,
    /// Find Information Response
    L2C_CODE_ATT_FIND_INFO_RSP      = 0x05,
    /// Find By Type Value Request
    L2C_CODE_ATT_FIND_BY_TYPE_REQ   = 0x06,
    /// Find By Type Value Response
    L2C_CODE_ATT_FIND_BY_TYPE_RSP   = 0x07,
    /// Read By Type Request
    L2C_CODE_ATT_RD_BY_TYPE_REQ     = 0x08,
    /// Read By Type Response
    L2C_CODE_ATT_RD_BY_TYPE_RSP     = 0x09,
    /// Read Request
    L2C_CODE_ATT_RD_REQ             = 0x0A,
    /// Read Response
    L2C_CODE_ATT_RD_RSP             = 0x0B,
    /// Read Blob Request
    L2C_CODE_ATT_RD_BLOB_REQ        = 0x0C,
    /// Read Blob Response
    L2C_CODE_ATT_RD_BLOB_RSP        = 0x0D,
    /// Read Multiple Request
    L2C_CODE_ATT_RD_MULT_REQ        = 0x0E,
    /// Read Multiple Response
    L2C_CODE_ATT_RD_MULT_RSP        = 0x0F,
    /// Read by Group Type Request
    L2C_CODE_ATT_RD_BY_GRP_TYPE_REQ = 0x10,
    /// Read By Group Type Response
    L2C_CODE_ATT_RD_BY_GRP_TYPE_RSP = 0x11,
    /// Write Request
    L2C_CODE_ATT_WR_REQ             = 0x12,
    /// Write Response
    L2C_CODE_ATT_WR_RSP             = 0x13,
    /// Write Command
    L2C_CODE_ATT_WR_CMD_INFO        = 0x14,
    L2C_CODE_ATT_WR_CMD             = 0x52,
    /// Signed Write Command
    L2C_CODE_ATT_SIGN_WR_CMD_INFO   = 0x15,
    L2C_CODE_ATT_SIGN_WR_CMD        = 0xD2,
    /// Prepare Write Request
    L2C_CODE_ATT_PREP_WR_REQ        = 0x16,
    /// Prepare Write Response
    L2C_CODE_ATT_PREP_WR_RSP        = 0x17,
    /// Execute Write Request
    L2C_CODE_ATT_EXE_WR_REQ         = 0x18,
    /// Execute Write Response
    L2C_CODE_ATT_EXE_WR_RSP         = 0x19,
    /// Handle Value Notification
    L2C_CODE_ATT_HDL_VAL_NTF        = 0x1B,
    /// Handle Value Indication
    L2C_CODE_ATT_HDL_VAL_IND        = 0x1D,
    /// Handle Value Confirmation
    L2C_CODE_ATT_HDL_VAL_CFM        = 0x1E,

    /// max number of security codes
    L2C_CODE_ATT_MAX,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Connection Parameter Update Request
struct l2cc_update_param_req
{
    /// Signaling code - 0x12
    uint8_t  code;
    /// Packet Identifier
    uint8_t  pkt_id;
    /// data length
    uint16_t length;
    /// minimum value for the connection event interval
    uint16_t intv_min;
    /// maximum value for the connection event interval
    uint16_t intv_max;
    /// slave latency parameter
    uint16_t latency;
    /// connection timeout parameter
    uint16_t timeout;
};

/// Connection Parameter Update Response
struct l2cc_update_param_rsp
{
    /// Signaling code - 0x13
    uint8_t  code;
    /// Packet Identifier
    uint8_t  pkt_id;
    /// data length
    uint16_t length;
    /// Result field indicates the response to the Connection Parameter Update Request
    /// - 0x0000 Connection Parameters accepted
    /// - 0x0001 Connection Parameters rejected
    uint16_t response;
};


/// Command Reject
struct l2cc_reject
{
    /// Signaling code - 0x01
    uint8_t  code;
    /// Packet Identifier
    uint8_t  pkt_id;
    /// data length
    uint16_t length;
    /// The Reason field describes why the Request packet was rejected
    /// - 0x0000 Command not understood
    /// - 0x0001 Signaling MTU exceeded
    /// - 0x0002 Invalid CID in request
    uint16_t reason;
    /// Optional param 1
    uint16_t opt1;
    /// Optional param 2
    uint16_t opt2;
};


/// Pairing Request
struct l2cc_pairing_req
{
    /// security code - 0x01
    uint8_t     code;
    /// IO Capability
    uint8_t     iocap;
    /// OOB data flag
    uint8_t     oob;
    /// AuthReq
    uint8_t     auth;
    /// Maximum Encryption Key Size
    uint8_t     key_size;
    /// Initiator Key Distribution
    uint8_t     ikey_dist;
    /// Responder Key Distribution
    uint8_t     rkey_dist;
};
/// Pairing Response
struct l2cc_pairing_rsp
{
    /// security code - 0x02
    uint8_t     code;
    /// IO Capability
    uint8_t     iocap;
    /// OOB data flag
    uint8_t     oob;
    /// AuthReq
    uint8_t     auth;
    /// Maximum Encryption Key Size
    uint8_t     key_size;
    /// Initiator Key Distribution
    uint8_t     ikey_dist;
    /// Responder Key Distribution
    uint8_t     rkey_dist;
};
/// Pairing Confirm
struct l2cc_pairing_cfm
{
    /// security code - 0x03
    uint8_t     code;
    ///Confirm value
    uint8_t     cfm_val[CFM_LEN];
};
/// Pairing Random
struct l2cc_pairing_random
{
    /// security code - 0x04
    uint8_t     code;
    ///Random value
    uint8_t     random[RAND_VAL_LEN];
};
/// Pairing Failed
struct l2cc_pairing_failed
{
    /// security code - 0x05
    uint8_t     code;
    /// The Reason field indicates why the pairing failed
    uint8_t     reason;
};
/// Encryption Information
struct l2cc_encryption_inf
{
    /// security code - 0x06
    uint8_t     code;
    ///16-byte array for LTK value
    uint8_t     ltk[KEY_LEN];
};
/// Master Identification
struct l2cc_master_id
{
    /// security code - 0x07
    uint8_t     code;
    /// key diversifier
    uint16_t    ediv;
    ///8-byte array for random number
    uint8_t     nb[RAND_NB_LEN];
};
/// Identity Information
struct l2cc_identity_inf
{
    /// security code - 0x08
    uint8_t     code;
    ///16-byte array for IRK value
    uint8_t     irk[KEY_LEN];
};
/// Identity Address Information
struct l2cc_id_addr_inf
{
    /// security code - 0x09
    uint8_t     code;
    /// BD Address Type
    uint8_t     addr_type;
    /// BD Address
    struct bd_addr addr;
};
/// Signing Information
struct l2cc_signing_inf
{
    /// security code - 0x0A
    uint8_t     code;
    ///16-byte array for CSRK value
    uint8_t     csrk[KEY_LEN];
};
/// Security Request
struct l2cc_security_req
{
    /// security code - 0x0B
    uint8_t     code;
    /// AuthReq
    uint8_t     auth;
};


/* Attribute protocol PDUs */

/// Error response
struct l2cc_att_err_rsp
{
    /// Error Response - 0x01
    uint8_t     code;
    /// The request that generated this error response
    uint8_t     op_code;
    /// The attribute handle that generated this error response
    uint16_t    handle;
    ///The reason why the request has generated an error response
    uint8_t     reason;
};

/// Exchange MTU Request
struct l2cc_att_mtu_req
{
    /// Exchange MTU Request - 0x02
    uint8_t     code;
    /// Client Rx MTU size
    uint16_t    mtu_size;
};

/// Exchange MTU Response
struct l2cc_att_mtu_rsp
{
    /// Exchange MTU Response - 0x03
    uint8_t     code;
    /// Server Rx MTU size
    uint16_t    mtu_size;
};

/// Find Information Request
struct l2cc_att_find_info_req
{
    /// Find Information Request - 0x04
    uint8_t     code;
    /// First requested handle number
    uint16_t    shdl;
    /// Last requested handle number
    uint16_t    ehdl;
};

/// Find Information Response
struct l2cc_att_find_info_rsp
{
    /// Find Information Response - 0x05
    uint8_t     code;
    /// The format of the information data.
    uint8_t     format;
    /// Data length
    uint16_t    data_len;
    ///The information data whose format is determined by the Format field
    uint8_t     data[__ARRAY_EMPTY];
};

/// Find By Type Value Request
struct l2cc_att_find_by_type_req
{
    /// Find By Type Value Request - 0x06
    uint8_t     code;
    /// First requested handle number
    uint16_t    shdl;
    /// Last requested handle number
    uint16_t    ehdl;
    /// 2 octet UUID to find
    uint16_t    type;
    /// Attribute value length
    uint16_t    val_len;
    /// Attribute value to find
    uint8_t     val[__ARRAY_EMPTY];
};

/// Find By Type Value Response
struct l2cc_att_find_by_type_rsp
{
    /// Find By Type Value Response - 0x07
    uint8_t     code;
    /// data length
    uint16_t    data_len;
    /// A list of 1 or more Handle Informations.
    uint8_t     data[__ARRAY_EMPTY];
};

/// Read By Type Request
struct l2cc_att_rd_by_type_req
{
    /// Read By Type Request - 0x08
    uint8_t     code;
    /// Starting Handle
    uint16_t    shdl;
    /// Ending Handle
    uint16_t    ehdl;
    /// Attribute uuid length
    uint16_t    uuid_len;
    /// Attribute type uuid
    uint8_t     uuid[__ARRAY_EMPTY];
};

/// Read By Type Response
struct l2cc_att_rd_by_type_rsp
{
    /// Read By Type Response - 0x09
    uint8_t     code;
    /// size of each attribute handle listed
    uint8_t     each_len;
    /// Attribute Data length
    uint16_t    data_len;
    /// A list of Attribute Data.
    uint8_t     data[__ARRAY_EMPTY];
};

/// Read Request
struct l2cc_att_rd_req
{
    /// Read Request - 0x0A
    uint8_t     code;
    /// Attribute Handle
    uint16_t    handle;
};

/// Read Response
struct l2cc_att_rd_rsp
{
    /// Read Response - 0x0B
    uint8_t    code;
    /// value length
    uint16_t   value_len;
    /// The value of the attribute with the handle given
    uint8_t    value[__ARRAY_EMPTY];
};

/// Read Blob Request
struct l2cc_att_rd_blob_req
{
    /// Read Blob Request - 0x0C
    uint8_t     code;
    /// Attribute Handle
    uint16_t    handle;
    /// The offset of the first octet to be read
    uint16_t    offset;
};

/// Read Blob Response
struct l2cc_att_rd_blob_rsp
{
    /// Read Blob Response - 0x0D
    uint8_t    code;
    /// value length
    uint16_t   value_len;
    /// Part of the value of the attribute with the handle given
    uint8_t    value[__ARRAY_EMPTY];
};

/// Read Multiple Request
struct l2cc_att_rd_mult_req
{
    /// Read Multiple Request - 0x0E
    uint8_t     code;
    /// Number of handles
    uint16_t    nb_handles;
    /// A set of two or more attribute handles.
    uint16_t    handles[__ARRAY_EMPTY];
};

/// Read Multiple Response
struct l2cc_att_rd_mult_rsp
{
    /// Read Multiple Response - 0x0F
    uint8_t    code;
    /// value length
    uint16_t   value_len;
    /// A set of two or more values.
    uint8_t    value[__ARRAY_EMPTY];
};

/// Read by Group Type Request
struct l2cc_att_rd_by_grp_type_req
{
    /// Read by Group Type Request - 0x10
    uint8_t     code;
    /// First requested handle number
    uint16_t    shdl;
    /// Last requested handle number
    uint16_t    ehdl;
    /// Attribute uuid length
    uint16_t    uuid_len;
    /// Attribute type uuid (2 or 16 octet UUID)
    uint8_t     uuid[__ARRAY_EMPTY];
};

/// Read By Group Type Response
struct l2cc_att_rd_by_grp_type_rsp
{
    /// Read By Group Type Response - 0x11
    uint8_t     code;
    /// size of each attribute handle listed
    uint8_t     each_len;
    /// Attribute Data length
    uint16_t    data_len;
    /// A list of Attribute Data.
    uint8_t     data[__ARRAY_EMPTY];
};

/// Write Request
struct l2cc_att_wr_req
{
    /// Write Request - 0x12
    uint8_t     code;
    /// The handle of the attribute to be written
    uint16_t    handle;
    /// Value length
    uint16_t    value_len;
    /// The value to be written to the attribute
    uint8_t     value[__ARRAY_EMPTY];
};

/// Write Response
struct l2cc_att_wr_rsp
{
    /// Write Response - 0x13
    uint8_t     code;
};

/// Write Command
struct l2cc_att_wr_cmd
{
    /// Write Command - 0x52
    uint8_t     code;
    /// The handle of the attribute to be written
    uint16_t    handle;
    /// Value length
    uint16_t    value_len;
    /// The value to be written to the attribute
    uint8_t     value[__ARRAY_EMPTY];
};


/// Signed Write Command
struct l2cc_att_sign_wr_cmd
{
    /// Write Command - 0xD2
    uint8_t     code;
    /// The handle of the attribute to be written
    uint16_t    handle;
    /// Attribute Data length
    uint16_t    value_len;
    /// The value to be written to the attribute
    /// + 12 bytes of signatures:
    /// Authentication signature for the Attribute Upload, Attribute Handle and Attribute
    /// Value Parameters
    uint8_t     value[__ARRAY_EMPTY];
};

/// Prepare Write Request
struct l2cc_att_prep_wr_req
{
    /// Prepare Write Request - 0x16
    uint8_t     code;
    /// The handle of the attribute to be written
    uint16_t    handle;
    /// The offset of the first octet to be written
    uint16_t    offset;
    /// Value length
    uint16_t    value_len;
    /// The value to be written to the attribute
    uint8_t     value[__ARRAY_EMPTY];
};

/// Prepare Write Response
struct l2cc_att_prep_wr_rsp
{
    /// Prepare Write Response - 0x17
    uint8_t     code;
    /// The handle of the attribute to be written
    uint16_t    handle;
    /// The offset of the first octet to be written
    uint16_t    offset;
    /// Value length
    uint16_t    value_len;
    /// The value to be written to the attribute
    uint8_t     value[__ARRAY_EMPTY];
};

/// Execute Write Request
struct l2cc_att_exe_wr_req
{
    /// Execute Write Request - 0x18
    uint8_t     code;
    /// 0x00 – Cancel all prepared writes
    /// 0x01 – Immediately write all pending prepared values
    uint8_t    flags;
};

/// Execute Write Response
struct l2cc_att_exe_wr_rsp
{
    /// Prepare Write Response - 0x19
    uint8_t     code;
};

/// Handle Value Notification
struct l2cc_att_hdl_val_ntf
{
    /// Handle Value Notification - 0x1B
    uint8_t     code;
    /// The handle of the attribute to be written
    uint16_t    handle;
    /// Value length
    uint16_t    value_len;
    /// The current value of the attribute
    uint8_t     value[__ARRAY_EMPTY];
};

/// Handle Value Indication
struct l2cc_att_hdl_val_ind
{
    /// Handle Value Indication - 0x1D
    uint8_t     code;
    /// The handle of the attribute to be written
    uint16_t    handle;
    /// Value length
    uint16_t    value_len;
    /// The current value of the attribute
    uint8_t     value[__ARRAY_EMPTY];
};

/// Handle Value Confirmation
struct l2cc_att_hdl_val_cfm
{
    /// Handle Value Confirmation - 0x1E
    uint8_t     code;
};


/// Default L2Cap PDU definition
struct l2cc_pdu
{
    /// L2Cap Payload Length
    uint16_t payld_len;
    /// L2Cap Channel ID
    uint16_t chan_id;

    /// Data PDU definition
    union l2cc_pdu_data
    {
        /// L2Cap packet code.
        uint8_t code;

        /* Connection Signaling Packets  */
        /// Command Reject
        struct l2cc_reject           reject;
        /// Connection Parameter Update Request
        struct l2cc_update_param_req update_req;
        /// Connection Parameter Update Response
        struct l2cc_update_param_rsp update_rsp;

        /* Security manager PDUs */
        /// Pairing Request
        struct l2cc_pairing_req      pairing_req;
        /// Pairing Response
        struct l2cc_pairing_rsp      pairing_rsp;
        /// Pairing Confirm
        struct l2cc_pairing_cfm      pairing_cfm;
        /// Pairing Random
        struct l2cc_pairing_random   pairing_random;
        /// Pairing Failed
        struct l2cc_pairing_failed   pairing_failed;
        /// Encryption Information
        struct l2cc_encryption_inf   encryption_inf;
        /// Master Identification
        struct l2cc_master_id        master_id;
        /// Identity Information
        struct l2cc_identity_inf     identity_inf;
        /// Identity Address Information
        struct l2cc_id_addr_inf      id_addr_inf;
        /// Signing Information
        struct l2cc_signing_inf      signing_inf;
        /// Security Request
        struct l2cc_security_req     security_req;

        /* Attribute protocol PDUs */
        /// Error response
        struct l2cc_att_err_rsp             err_rsp;
        /// Exchange MTU Request
        struct l2cc_att_mtu_req             mtu_req;
        /// Exchange MTU Response
        struct l2cc_att_mtu_rsp             mtu_rsp;
        /// Find Information Request
        struct l2cc_att_find_info_req       find_info_req;
        /// Find Information Response
        struct l2cc_att_find_info_rsp       find_info_rsp;
        /// Find By Type Value Request
        struct l2cc_att_find_by_type_req    find_by_type_req;
        /// Find By Type Value Response
        struct l2cc_att_find_by_type_rsp    find_by_type_rsp;
        /// Read By Type Request
        struct l2cc_att_rd_by_type_req      rd_by_type_req;
        /// Read By Type Response
        struct l2cc_att_rd_by_type_rsp      rd_by_type_rsp;
        /// Read Request
        struct l2cc_att_rd_req              rd_req;
        /// Read Response
        struct l2cc_att_rd_rsp              rd_rsp;
        /// Read Blob Request
        struct l2cc_att_rd_blob_req         rd_blob_req;
        /// Read Blob Response
        struct l2cc_att_rd_blob_rsp         rd_blob_rsp;
        /// Read Multiple Request
        struct l2cc_att_rd_mult_req         rd_mult_req;
        /// Read Multiple Response
        struct l2cc_att_rd_mult_rsp         rd_mult_rsp;
        /// Read by Group Type Request
        struct l2cc_att_rd_by_grp_type_req  rd_by_grp_type_req;
        /// Read By Group Type Response
        struct l2cc_att_rd_by_grp_type_rsp  rd_by_grp_type_rsp;
        /// Write Request
        struct l2cc_att_wr_req              wr_req;
        /// Write Response
        struct l2cc_att_wr_rsp              wr_rsp;
        /// Write Command
        struct l2cc_att_wr_cmd              wr_cmd;
        /// Signed Write Command
        struct l2cc_att_sign_wr_cmd         sign_wr_cmd;
        /// Prepare Write Request
        struct l2cc_att_prep_wr_req         prep_wr_req;
        /// Prepare Write Response
        struct l2cc_att_prep_wr_rsp         prep_wr_rsp;
        /// Execute Write Request
        struct l2cc_att_exe_wr_req          exe_wr_req;
        /// Execute Write Response
        struct l2cc_att_exe_wr_rsp          exe_wr_rsp;
        /// Handle Value Notification
        struct l2cc_att_hdl_val_ntf         hdl_val_ntf;
        /// Handle Value Indication
        struct l2cc_att_hdl_val_ind         hdl_val_ind;
        /// Handle Value Confirmation
        struct l2cc_att_hdl_val_cfm         hdl_val_cfm;
    } data;
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * Pack PDU from firmware readable data to a L2CAP packet using generic method.
 *
 * @param[in]      p_pdu       Pointer to the unpacked PDU structure
 * @param[in|out]  p_offset    Pointer to address where the next part of PDU will be read
 * @param[in|out]  p_length    Pointer to the length of payload that has been packed
 * @param[in|out]  p_buffer    Pointer to address where the next part of PDU will be written
 * @param[in]      conidx      Connection Index
 * @param[in]      llid        LLID of the next packet to send
 *
 * @return L2C_ERR_NO_ERROR if packing succeed, else another error code.
 ****************************************************************************************
 */
uint8_t l2cc_pdu_pack(struct l2cc_pdu *p_pdu, uint16_t *p_offset, uint16_t *p_length,
                      uint8_t *p_buffer, uint8_t conidx, uint8_t llid);

/**
 ****************************************************************************************
 * Unpack L2Cap PDU in a generic format that can be used by firmware using generic method.

 * @param[in]      p_pdu       Pointer to the unpacked PDU structure
 * @param[in|out]  p_offset    Pointer to address where the next part of PDU will be written
 * @param[in|out]  p_rem_len   Pointer to the remaining length value of PDU to receive
 * @param[in|out]  p_buffer    Pointer to address where the next part of PDU will be read
 * @param[in]      pkt_length  Length of the received packet
 * @param[in]      llid        LLID of the received packet
 *
 * @return L2C_ERR_NO_ERROR if packing succeed, else another error code.
 ****************************************************************************************
 */
uint8_t l2cc_pdu_unpack(struct l2cc_pdu *p_pdu, uint16_t *p_offset, uint16_t *p_rem_len,
                        const uint8_t *p_buffer, uint16_t pkt_length, uint8_t llid);

/**
 ****************************************************************************************
 * @brief Parse L2CAP header to detect the receiver of the data
 * If the packet has erroneous channel id, it will be sent to TASK_L2CM.
 *
 * @param[in] conhdl            connection handle
 * @param[in] buf_ptr           data buffer pointer
 *
 * @return Destination task
 *
 ****************************************************************************************
 */
ke_task_id_t l2cc_detect_dest(uint8_t conidx, uint8_t *buf_ptr);

/// @} L2CC_PDU

#endif //(BLE_L2CC)

#endif /* _L2CC_PDU_H_ */
