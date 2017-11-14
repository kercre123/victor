/**
 ****************************************************************************************
 *
 * @file gapm.h
 *
 * @brief Generic Access Profile Manager Header.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */


#ifndef _GAPM_H_
#define _GAPM_H_

/**
 ****************************************************************************************
 * @addtogroup GAPM Generic Access Profile Manager
 * @ingroup GAP
 * @brief Generic Access Profile Manager.
 *
 * The GAP Manager module is responsible for providing an API to the application in order
 * to manage all non connected stuff such as configuring device to go in desired mode
 * (discoverable, connectable, etc.) and perform required actions (scanning, connection,
 * etc.). GAP Manager is also responsible of managing GAP Controller state according to
 * corresponding BLE connection states.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#include "ke_task.h"
#include "llc_task.h"
#include "gap.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Scan filter size
#define GAPM_SCAN_FILTER_SIZE   10


/// Operation type
enum gapm_op_type
{
    /// Configuration operation
    GAPM_CFG_OP = 0x00,
    /// Air mode operation (scanning, advertising, connection establishment)
    GAPM_AIR_OP,
    /// Max number of operations
    GAPM_MAX_OP,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// Air operation default parameters
struct gapm_air_operation
{
    /// Operation code.
    uint8_t  code;


    /** Own BD address source of the device:
     * - GAPM_PUBLIC_ADDR: Public Address
     * - GAPM_PROVIDED_RND_ADDR: Provided random address
     * - GAPM_GEN_STATIC_RND_ADDR: Generated static random address
     * - GAPM_GEN_RSLV_ADDR: Generated resolvable private random address
     * - GAPM_GEN_NON_RSLV_ADDR: Generated non-resolvable private random address
     * - GAPM_PROVIDED_RECON_ADDR: Provided Reconnection address (only for GAPM_ADV_DIRECT)
     */
    uint8_t addr_src;

    /// Dummy data use to retrieve internal operation state (should be set to 0).
    uint16_t state;

    /// Duration of resolvable address before regenerate it.
    uint16_t renew_dur;

    /// Provided own static private random address (addr_src = 1 or 5)
    struct bd_addr addr;
};


/// GAP Manager environment structure
struct gapm_env_tag
{
    /// Request operation Kernel message
    void* operation[GAPM_MAX_OP];

    #if (BLE_CENTRAL || BLE_OBSERVER)
    /// Scan filtering Array
    struct gap_bdaddr* scan_filter;
    #endif // (BLE_CENTRAL || BLE_OBSERVER)

    /// Task ID used to request database operations.
    ke_task_id_t requester[GAPM_MAX_OP];
    #if (BLE_ATTS)
    /// GAP service start handle
    uint16_t svc_start_hdl;
    #endif // (BLE_ATTS)

    /// Device IRK used for resolvable random BD address generation (MSB -> LSB)
    struct gap_sec_key irk;

    /// Local Public BD Address information
    struct bd_addr addr;
    /// Device Role
    uint8_t role;
    /// Number of BLE connection
    uint8_t connections;
};

/*
 * MACROS
 ****************************************************************************************
 */
#if (BLE_ATTS)
/// retrieve gap attribute handle from attribute index.
#define GAPM_GET_ATT_HANDLE(idx)\
    ((gapm_env.svc_start_hdl == 0)? (0) :(gapm_env.svc_start_hdl + (idx)))

#endif // (BLE_ATTS)


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
/// GAP Manager environment variable.
extern struct gapm_env_tag gapm_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize Generic Access Profile Manager Module.
 *
 * @param[in] reset  true if it's requested by a reset; false if it's boot initialization
 *
 ****************************************************************************************
 */
void gapm_init(bool reset);

/**
 ****************************************************************************************
 * @brief Initialize the host (reset requested)
 *
 ****************************************************************************************
 */
void gapm_host_init(void);

/**
 ****************************************************************************************
 * @brief Get operation on going
 *
 * @param[in] op_type       Operation type.
 *
 * @return operation code on going
 ****************************************************************************************
 */
uint8_t gapm_get_operation(uint8_t op_type);

/**
 ****************************************************************************************
 * @brief Send operation completed message according to operation type.
 * Perform also am operation clean-up
 *
 * @param[in] op_type Operation type
 * @param[in] status  Command status
 *****************************************************************************************
 */
void gapm_send_complete_evt(uint8_t op_type, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send operation completed message with status error code not related to a
 * running operation.
 *
 * @param[in] operation Operation code
 * @param[in] requester requester of operation
 * @param[in] status    Error status code
 ****************************************************************************************
 */
void gapm_send_error_evt(uint8_t operation, const ke_task_id_t requester, uint8_t status);


#if (BLE_CENTRAL || BLE_PERIPHERAL)
/**
 ****************************************************************************************
 * @brief A connection has been created, initialize host stack to be ready for connection.
 *
 * @param[in] con_params Connection parameters from lower layers
 * @param[in] air_op     Air operation informations
 *
 * @return Connection index allocated to the new connection.
 ****************************************************************************************
 */
uint8_t gapm_con_create(struct llc_create_con_cmd_complete const *con_params,
                        struct gapm_air_operation* air_op);


/**
 ****************************************************************************************
 * @brief Created link connection parameters (from bond data) has been set, connection
 * ready to be used.
 *
 * @param[in] conidx     Connection Index
 *
 ****************************************************************************************
 */
void gapm_con_enable(uint8_t conidx);


/**
 ****************************************************************************************
 * @brief A link has been disconnected, clean-up host stack for this connection.
 *
 * @param[in] conidx     Connection Index
 * @param[in] conhdl     Connection Handle
 * @param[in] reason     Reason of the disconnection
 *
 ****************************************************************************************
 */
void gapm_con_cleanup(uint8_t conidx, uint16_t conhdl, uint8_t reason);

#endif // (BLE_CENTRAL || BLE_PERIPHERAL)

/// @} GAPM

#endif /* _GAPM_H_ */
