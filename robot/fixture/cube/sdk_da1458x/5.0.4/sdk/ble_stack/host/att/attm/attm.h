/**
 ****************************************************************************************
 *
 * @file attm.h
 *
 * @brief Header file - ATTM.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef ATTM_H_
#define ATTM_H_

/**
 ****************************************************************************************
 * @addtogroup ATTM Attribute Manager
 * @ingroup ATT
 * @brief Attribute Manager
 *
 * The ATTM is the attribute manager of the Attribute Profile block and
 * is responsible for managing messages and providing generic attribute
 * functionalities to @ref ATTC "ATTC" and @ref ATTS "ATTS".
 *
 *
 * @{
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "ke_task.h"
#include "ke_timer.h"
#include "co_error.h"
#include "attm_cfg.h"
#include "att.h"
#include <string.h>

/*
 * DEFINES
 ****************************************************************************************
 */


/// Macro used to retrieve access permission rights
#define ATT_GET_PERM_RIGHTS(perm, access)\
        (((perm)& (PERM_ACCESS_MASK_ ## access)) >> (PERM_ACCESS_ ## access))

/// Macro used to retrieve permission value from access and rights on attribute.
#define PERM(access, right) \
    (((PERM_RIGHT_ ## right) << (PERM_ACCESS_ ## access)) & (PERM_ACCESS_MASK_ ## access))


/// Macro used know if permission is set or not.
#define PERM_IS_SET(perm, access, right) \
    (((perm) & (((PERM_RIGHT_MASK_ ## right) << (PERM_ACCESS_ ## access))) \
                & (PERM_ACCESS_MASK_ ## access)) == PERM(access, right))

/// Retrieve attribute security level from attribute right and service right
#define ATT_GET_SEC_LVL(att_right, svc_right) \
    co_max(((att_right) & PERM_RIGHT_MASK_AUTH), ((svc_right) & PERM_RIGHT_MASK_AUTH));

/// Retrieve attribute authorization from attribute right and service right
#define ATT_GET_AUTHZ(att_right, svc_right) \
    ((((att_right) & PERM_RIGHT_MASK_AUTHZ) | ((svc_right) & PERM_RIGHT_MASK_AUTHZ)) \
            == PERM_RIGHT_AUTHZ)

/// Attribute access types
enum
{
    /// Read Access
    PERM_ACCESS_RD           = 0,
    /// Write Access
    PERM_ACCESS_WR           = 3,
    /// Indication Access
    PERM_ACCESS_IND          = 6,
    /// Notification Access
    PERM_ACCESS_NTF          = 9,
    /// Check Encryption key size
    PERM_ACCESS_EKS          = 12,
    /// Hide/Show attribute
    PERM_ACCESS_HIDE         = 13,
    /// Write Signed Enabled attribute
    PERM_ACCESS_WRITE_SIGNED = 14,
};

/// Attribute access types mask
enum
 {
    /// Read Access Mask
    PERM_ACCESS_MASK_RD           = 0x0007,
    /// Write Access Mask
    PERM_ACCESS_MASK_WR           = 0x0038,
    /// Indication Access Mask
    PERM_ACCESS_MASK_IND          = 0x01C0,
    /// Notification Access Mask
    PERM_ACCESS_MASK_NTF          = 0x0E00,
    /// Check Encryption key size Mask
    PERM_ACCESS_MASK_EKS          = 0x1000,
    /// Hide/Show attribute Mask
    PERM_ACCESS_MASK_HIDE         = 0x2000,
    /// Write Signed Enabled attribute Mask
    PERM_ACCESS_MASK_WRITE_SIGNED = 0x4000,
 };

/// Service access types
enum
{
    /// Service Access
    PERM_ACCESS_SVC      = 0,
    /// Check Encryption key size for service Access
    PERM_ACCESS_SVC_EKS  = 3,
    /// Hide/Show service Access
    PERM_ACCESS_SVC_HIDE = 4,
};

/// Service access types mask
enum
 {
    /// Service Access Mask
    PERM_ACCESS_MASK_SVC       = 0x07,
    /// Check Encryption key size for service Access Mask
    PERM_ACCESS_MASK_SVC_EKS   = 0x08,
    /// Hide/Show service Access Mask
    PERM_ACCESS_MASK_SVC_HIDE  = 0x10,
 };

/// Attribute & Service access rights
enum
{
    /// Disable access
    PERM_RIGHT_DISABLE  = 0,
    /// Enable access
    PERM_RIGHT_ENABLE   = 1,
    /// Access Requires Unauthenticated link
    PERM_RIGHT_UNAUTH   = 2,
    /// Access Requires Authenticated link
    PERM_RIGHT_AUTH     = 3,
    /// Access Requires authorization
    PERM_RIGHT_AUTHZ    = 4,
};

/// Attribute & Service access rights mask
enum
{
    /// Disable access mask
    PERM_RIGHT_MASK_DISABLE  = 0x3,
    /// Enable access mask
    PERM_RIGHT_MASK_ENABLE   = 0x3,
    /// Access Requires Unauthenticated lin kmask
    PERM_RIGHT_MASK_UNAUTH   = 0x3,
    /// Access Requires Authenticated link mask
    PERM_RIGHT_MASK_AUTH     = 0x3,
    /// Access Requires authorization mask
    PERM_RIGHT_MASK_AUTHZ    = 0x4,
};


/// Attribute UUID Type
enum
{
    /// 16 Bits UUID (takes 2 bytes in ATTS_DB)
    TYPE_UUID_16,
    /// 32 Bits UUID (takes 4 bytes in ATTS_DB)
    TYPE_UUID_32,
    /// 128 Bits BT UUID (takes 2 bytes in ATTS_DB, only significant bits are stored in
    ///                   attribute database).
    TYPE_UUID_128_BT,
    /// 128 Bits Proprietary UUID (takes 16 bytes in ATTS_DB)
    TYPE_UUID_128_PROP,
};

/*
 * ATTRIBUTE BLOCK Defines
 */
/// Attribute role definition
enum
{
    /// Attribute server role
    ATT_SERVER = 0x00,
    /// Attribute client role
    ATT_CLIENT
};

/// execute flags
enum
{
    /// Cancel All the Reliable Writes
    ATT_CANCEL_ALL_PREPARED_WRITES = 0x00,
    /// Write All the Reliable Writes
    ATT_EXECUTE_ALL_PREPARED_WRITES
};



/*
 * DATA STRUCTURES
 ****************************************************************************************
 */

/// Service information
struct attm_svc_info
{
    /// Service start handle
    uint16_t start_hdl;
    /// Service end handle
    uint16_t end_hdl;
    /// Service task_id
    uint16_t task_id;
    /// Service permission
    uint8_t perm;
};

 /// Attribute element in database
 struct attm_elmt
 {
     /// Element UUID
     uint16_t uuid;
     /// Attribute permission
     uint16_t perm;
     /// Maximum length of the element
     /// 2 MSB bits are used to know if UUID is a 128 bits UUID and if its a BT UUID
     /// max length = 2^14 bytes
     att_size_t max_length;
     /// Current length of the element
     att_size_t length;
     /// Element value array
     uint8_t value[__ARRAY_EMPTY];
 };

 /// Attribute description (used to create database)
 struct attm_desc
 {
     /// Element UUID
     uint16_t uuid;
     /// Attribute permission
     uint16_t perm;
     /// Maximum length of the element
     att_size_t max_length;
     /// Current length of the element
     att_size_t length;
     /// Element value array
     uint8_t* value;
 };

/// Service database subset
struct attm_svc_db
{
    /// Next DB
    struct attm_svc_db* next_db;
    /// Start Handle
    uint16_t start_hdl;
    /// Last Handle
    uint16_t last_hdl;
    /// Task that manage service db
    uint16_t task_id;
    /// Number of attributes
    uint8_t nb_att;
    /// Service permissions
    uint8_t perm;
    /// Array of pointer to attribute elements
    struct attm_elmt* att_elems[__ARRAY_EMPTY];
};


/// ATTM General Information Manager
struct attm_env_tag
{
    #if(BLE_ATTS)
    /**
     * **************************************************************************************
     * @brief Attribute database
     *
     * The Attribute database is a list of attribute services sorted by handle number.
     * This database shall be initiate by GAP, GATT, profiles and application process at
     * startup and must not change during runtime.
     *
     * Database initialization shall be deterministic in order to always have service handle
     * at same position in database during all product life-cycle. This is required since
     * database client can save position of services in database to not perform service
     * discovery at each connection.
     ***************************************************************************************
     */
    struct attm_svc_db * db;

    /**
     ***************************************************************************************
     * Last attribute service searched.
     *
     * Used as a cached variable, it's used to reduce handle search duration.
     ***************************************************************************************
     */
    struct attm_svc_db * last_svc;
    #else
    // dummy register for empty structure
    uint8_t dummy;
    #endif //(BLE_ATTS)
};


#if (BLE_CENTRAL || BLE_PERIPHERAL)
/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
extern struct attm_env_tag attm_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialization of the ATTM module.
 * This function performs all the initializations of the ATTM module.
 *
 * @param[in] reset  true if it's requested by a reset; false if it's boot initialization
 *
 ****************************************************************************************
 */
void attm_init(bool reset);

/**
 ****************************************************************************************
 * @brief Initialize connection Attribute resource.
 *
 * @param[in] conidx    connection record index
 *
 ****************************************************************************************
 */
void attm_create(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Created link connection parameters (from bond data) has been set, connection
 * ready to be used.
 *
 * @param[in] conidx     Connection Index
 *
 ****************************************************************************************
 */
void attm_con_enable(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Cleanup connection
 *
 * @param[in] conidx   connection record index
 *
 ****************************************************************************************
 */
void attm_cleanup(uint8_t conidx);



/**
 ****************************************************************************************
 * @brief Convert 16 bit value to 128 bit UUID
 *
 * @param[out] uuid128   converted 16-bit Bluetooth UUID to 128-bit UUID
 * @param[in]  uuid16    16-bit UUID to convert to 128-bit UUID
 *
 ****************************************************************************************
 */
void attm_convert_16to128(uint8_t *uuid128, uint16_t uuid16);

/**
 ****************************************************************************************
 * @brief Convert 32 bit value to 128 bit UUID
 *
 * @param[out] uuid128   converted 32-bit Bluetooth UUID to 128-bit UUID
 * @param[in]  uuid32    32-bit UUID to convert to 128-bit UUID
 *
 ****************************************************************************************
 */
void attm_convert_32to128(uint8_t *uuid128, uint8_t *uuid32);


/**
 ****************************************************************************************
 * @brief Convert UUID value to 128 bit UUID
 *
 * @param[out] uuid128   converted 32-bit Bluetooth UUID to 128-bit UUID
 * @param[in]  uuid      UUID to convert to 128-bit UUID
 * @param[in]  uuid_len  UUID length
 *
 ****************************************************************************************
 */
void attm_convert_to128(uint8_t *uuid128, uint8_t *uuid, uint8_t uuid_len);


/**
 ****************************************************************************************
 * @brief Check the BT UUID for 128-bit input
 *
 * @param[in]  uuid128  128-bit UUID
 * @param[in]  uuid16   16 bits UUID
 *
 * @return true if uuid matches, false otherwise
 ****************************************************************************************
 */
bool attm_bt_uuid_match(uint8_t *uuid128, uint16_t uuid16);

/**
 ****************************************************************************************
 * @brief Check the BT 32bits UUID for 128-bit input
 *
 * @param[in]  uuid128  128-bit UUID
 * @param[in]  uuid32   32 bits UUID
 *
 * @return true if uuid matches, false otherwise
 ****************************************************************************************
 */
bool attm_bt32_uuid_match(uint8_t *uuid128, uint8_t * uuid32);



/**
 ****************************************************************************************
 * @brief Check if two UUIDs matches
 *
 * @param[in]  uuid_a      UUID A value
 * @param[in]  uuid_a_len  UUID A length
 * @param[in]  uuid_b      UUID B value
 * @param[in]  uuid_b_len  UUID B length
 *
 * @return true if UUIDs matches, false otherwise
 ****************************************************************************************
 */
bool attm_uuid_match(uint8_t *uuid_a, uint8_t uuid_a_len,
                      uint8_t *uuid_b, uint8_t uuid_b_len);


/**
 ****************************************************************************************
 * @brief Check if two UUIDs matches
 *
 * @param[in]  uuid_a      UUID A value
 * @param[in]  uuid_a_len  UUID A length
 * @param[in]  uuid_b      UUID B 16 bit value
 *
 * @return true if UUIDs matches, false otherwise
 ****************************************************************************************
 */
bool attm_uuid_match_16(uint8_t *uuid_a, uint8_t uuid_a_len, uint16_t uuid_b);

/**
 ****************************************************************************************
 * @brief Check if it's a Bluetooth UUID for 128-bit input
 *
 * @param[in]  uuid      128-bit UUID
 *
 * @return true if uuid  is a Bluetooth 128-bit UUID, false else.
 ****************************************************************************************
 */
bool attm_is_bt_uuid128(uint8_t *uuid);

/**
 ****************************************************************************************
 * @brief Check if it's a Bluetooth 32 bits UUID for 128-bit input
 *
 * @param[in]  uuid      128-bit UUID
 *
 * @return true if uuid  is a Bluetooth 128-bit UUID, false else.
 ****************************************************************************************
 */
bool attm_is_bt32_uuid128(uint8_t *uuid);

#endif // #if (BLE_CENTRAL || BLE_PERIPHERAL)
/// @} ATTM
#endif // ATTM_H_
