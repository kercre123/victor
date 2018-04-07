/**
 ****************************************************************************************
 *
 * @file gapc.h
 *
 * @brief Generic Access Profile Controller Header.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */


#ifndef _GAPC_H_
#define _GAPC_H_

/**
 ****************************************************************************************
 * @addtogroup GAPC Generic Access Profile Controller
 * @ingroup GAP
 * @brief  Generic Access Profile Controller.
 *
 * The GAP Controller module is responsible for providing an API to the application in
 * to perform GAP action related to a BLE connection (pairing, update parameters,
 * disconnect ...). GAP controller is multi-instantiated, one task instance per BLE
 * connection.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_CENTRAL || BLE_PERIPHERAL)

#include "ke_task.h"
#include "gap.h"
#include "gapc_task.h"
#include "llc_task.h"



/*
 * DEFINES
 ****************************************************************************************
 */

/// Information source.
enum gapc_addr_src
{
    /// Local info.
    GAPC_INFO_LOCAL,
    /// Peer info.
    GAPC_INFO_PEER,
    /// Maximum info source.
    GAPC_INFO_MAX,
};



/// Link security status. This status represents the authentication/authorization/bonding levels of the connection
enum gapc_lk_sec_req
{
    /// No security requirements on current link
    GAPC_LK_SEC_NONE,
    /// Link is unauthenticated
    GAPC_LK_UNAUTHENTICATED,
    /// Link is authenticated
    GAPC_LK_AUTHENTICATED,
    /// Link is authorized
    GAPC_LK_AUTHORIZED ,
    /// Link is bonded
    GAPC_LK_BONDED,
    /// Link is Encrypted
    GAPC_LK_ENCRYPTED,
};


/// fields definitions.
enum gapc_fields
{
    /// Local connection role
    GAPC_ROLE      = 0,
    /// Encrypted connection or not
    GAPC_ENCRYPTED = 1,
    /// Authentication informations
    GAPC_AUTH      = 2,
    /// Authorization informations
    GAPC_AUTHZ     = 5,
};


/// fields mask definitions.
enum gapc_fields_mask
{
    /// Bit[0]
    GAPC_ROLE_MASK      = 0x01,
    /// Bit[1]
    GAPC_ENCRYPTED_MASK = 0x02,
    /// Bit[4-2]
    GAPC_AUTH_MASK      = 0x1C,
    /// Bit[6-5]
    GAPC_AUTHZ_MASK     = 0x60,
};


/*
 * MACRO DEFINITIONS
 *********************************GET*******************************************************
 */
/// Set link configuration field
#define GAPC_SET_FIELD(conidx, field, value)\
    (gapc_env[conidx]->fields) = ((gapc_env[conidx]->fields) & (~GAPC_##field##_MASK)) \
                                     | (((value) << GAPC_##field) & (GAPC_##field##_MASK))


/// Get link configuration field
#define GAPC_GET_FIELD(conidx, field)\
    (((gapc_env[conidx]->fields) & (GAPC_##field##_MASK)) >> GAPC_##field)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// GAP controller environment variable structure.
struct gapc_env_tag
{
    /// Request operation Kernel message
    void* operation;
    /// Operation requester task id
    ke_task_id_t requester;

    /* Connection parameters to keep */
    /// connection handle
    uint16_t conhdl;

    /// CSRK values (Local and remote)
    struct gap_sec_key csrk[GAPC_INFO_MAX];
    /// signature counter values (Local and remote)
    uint32_t sign_counter[GAPC_INFO_MAX];

    /// Encryption key size
    uint8_t key_size;

    /// Configuration fields:
    ///   7   6   5   4   3   2   1   0
    /// +---+---+---+---+---+---+---+---+
    /// | - | AUTHZ |    AUTH   | E | R |
    /// +---+---+---+---+---+---+---+---+
    uint8_t fields;

    /// Task id requested disconnection
    ke_task_id_t disc_requester;

    // BD Address used for the link that should be kept
    struct gap_bdaddr src[GAPC_INFO_MAX];
};

/*
 * MACROS
 ****************************************************************************************
 */


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
extern struct gapc_env_tag* gapc_env[GAPC_IDX_MAX];

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize Generic Access Profile Controller Module.
 *
 * @param[in] reset  true if it's requested by a reset; false if it's boot initialization
 *
 ****************************************************************************************
 */
void gapc_init(bool reset);


/**
 ****************************************************************************************
 * @brief A connection has been created, initialize Controller task.
 *
 * This function find first available task index available for new connection.
 * It triggers also connection event to task that has requested the connection.
 *
 * @param[in] con_params Connection parameters from lower layers
 * @param[in] requester  Task that request the connection to send indication(s)
 * @param[in] laddr      Local BD Address
 * @param[in] laddr_type Local BD Address Type (PUBLIC or RAND)
 *
 * @return Connection index allocated to the new connection.
 ****************************************************************************************
 */
uint8_t gapc_con_create(struct llc_create_con_cmd_complete const *con_params,
                        ke_task_id_t requester, struct bd_addr* laddr, uint8_t laddr_type);

/**
 ****************************************************************************************
 * @brief A connection has been disconnected, uninitialized Controller task.
 *
 * unregister connection, and destroy environment variable allocated for current connection.
 *
 * @param[in] conidx  Connection index
 *
 * @return Connection index of the connection.
 ****************************************************************************************
 */
uint8_t gapc_con_cleanup(uint8_t conidx);


/**
 ****************************************************************************************
 * @brief Send Disconnection indication to specific task
 *
 * @param[in] conidx  Connection index
 * @param[in] reason  Disconnection reason
 * @param[in] conhdl  Connection handle
 * @param[in] dest_id Message destination ID
 *
 ****************************************************************************************
 */
void gapc_send_disconect_ind(uint8_t conidx,  uint8_t reason, uint8_t conhdl,
                              ke_task_id_t dest_id);


/**
 ****************************************************************************************
 * @brief Retrieve connection index from connection handle.
 *
 * @param[in] conhdl Connection handle
 *
 * @return Return found connection index, GAP_INVALID_CONIDX if not found.
 ****************************************************************************************
 */
uint8_t gapc_get_conidx(uint16_t conhdl);

/**
 ****************************************************************************************
 * @brief Retrieve connection handle from connection index.
 *
 * @param[in] conidx Connection index
 *
 * @return Return found connection handle, GAP_INVALID_CONHDL if not found.
 ****************************************************************************************
 */
uint16_t gapc_get_conhdl(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Retrieve connection role from connection index.
 *
 * @param[in] conidx Connection index
 *
 * @return Return found connection role
 ****************************************************************************************
 */
uint8_t gapc_get_role(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Retrieve connection address information on current link.
 *
 * @param[in] conidx Connection index
 * @param[in] src    Connection information source
 *
 * @return Return found connection address
 ****************************************************************************************
 */
struct gap_bdaddr* gapc_get_bdaddr(uint8_t conidx, uint8_t src);

/**
 ****************************************************************************************
 * @brief Retrieve connection CSRK information on current link.
 *
 * @param[in] conidx Connection index
 * @param[in] src    Connection information source
 *
 * @return Return found connection CSRK
 ****************************************************************************************
 */
struct gap_sec_key* gapc_get_csrk(uint8_t conidx, uint8_t src);

/**
 ****************************************************************************************
 * @brief Return the sign counter value for the specified connection index.
 *
 * @param[in] conidx Connection index
 * @param[in] src    Connection information source
 *
 * @return the requested signCounter value
 ****************************************************************************************
 */
uint32_t gapc_get_sign_counter(uint8_t conidx, uint8_t src);

/**
 * @brief Send a complete event of ongoing executed operation to requester.
 * It also clean-up variable used for ongoing operation.
 *
 * @param[in] conidx Connection index
 * @param[in] status Status of completed operation
 */
void gapc_send_complete_evt(uint8_t conidx, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send operation completed message with status error code not related to a
 * running operation.
 *
 * @param[in] conidx    Connection index
 * @param[in] operation Operation code
 * @param[in] requester requester of operation
 * @param[in] status    Error status code
 ****************************************************************************************
 */
void gapc_send_error_evt(uint8_t conidx, uint8_t operation, const ke_task_id_t requester, uint8_t status);



/**
 ****************************************************************************************
 * @brief Check if current link support security requirements.
 *
 * @param[in] conidx  Connection index
 * @param[in] sec_req Link security requirement to test
 *
 * @return True if link requirement is supported, False else.
 ****************************************************************************************
 */
bool gapc_is_sec_set(uint8_t conidx, uint8_t sec_req);

/**
 ****************************************************************************************
 * @brief Retrieve the encryption key size of the connection
 *
 * @param[in] conidx Connection index
 *
 * @return encryption key size (size is 7 - 16 byte range)
 *
 ****************************************************************************************
 */
uint8_t gapc_get_enc_keysize(uint8_t conidx);



/**
 ****************************************************************************************
 * @brief Set the encryption key size of the connection
 *
 * @param[in] conidx Connection index
 * @param[in] key_size encryption key size (size is 7 - 16 byte range)
 *
 ****************************************************************************************
 */
void gapc_set_enc_keysize(uint8_t conidx, uint8_t key_size);


/**
 ****************************************************************************************
 * @brief Update link status, current link is now encrypted
 *
 * @param[in] conidx Connection index
 *
 ****************************************************************************************
 */
void gapc_link_encrypted(uint8_t conidx);


/**
 ****************************************************************************************
 * @brief Update link authentication level
 *
 * @param[in] conidx Connection index
 * @param[in] auth   Link authentication level
 *
 ****************************************************************************************
 */
void gapc_auth_set(uint8_t conidx, uint8_t auth);

#endif // (BLE_CENTRAL || BLE_PERIPHERAL)
/// @} GAPC

#endif /* _GAPC_H_ */
