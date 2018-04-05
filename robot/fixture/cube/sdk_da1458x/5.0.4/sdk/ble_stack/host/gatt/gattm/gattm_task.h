/**
 ****************************************************************************************
 *
 * @file gattm_task.h
 *
 * @brief Header file - GATTMTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef GATTM_TASK_H_
#define GATTM_TASK_H_

#if (BLE_CENTRAL || BLE_PERIPHERAL)
/**
 ****************************************************************************************
 * @addtogroup GATTMTASK Task
 * @ingroup GATTM
 * @brief Handles ALL GATT block operations not related to a connection.
 *
 * The GATTMTASK is responsible for managing internal attribute database and state of
 * GATT controller which manage GATT block operations related to a connection.
 *
 * Messages may originate from @ref ATTM "ATTM", @ref GAP "GAP" and Application.
 *
 * @{
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#include "gatt.h"
#include "attm.h"
#include "attm_db.h"
#include "gattm.h"
#include "co_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// number of GATT Process
#define GATTM_IDX_MAX                                 0x01


/// states of GATT task
enum gattm_state_id
{
    /// idle state
    GATTM_IDLE,
    /// busy state
    GATTM_BUSY,
    /// Number of defined states.
    GATTM_STATE_MAX
};

/// GATT Task messages
enum gattm_msg_id
{
    /* Database Management */
    /// Add service in database request
    GATTM_ADD_SVC_REQ = KE_FIRST_MSG(TASK_GATTM),
    /// Add service in database response
    GATTM_ADD_SVC_RSP,
    /// Add attribute in service database request
    GATTM_ADD_ATTRIBUTE_REQ,
    /// Add attribute in service database response
    GATTM_ADD_ATTRIBUTE_RSP,

    /* Service management */
    /// Get permission settings of service request
    GATTM_SVC_GET_PERMISSION_REQ,
    /// Get permission settings of service response
    GATTM_SVC_GET_PERMISSION_RSP,
    /// Set permission settings of service request
    GATTM_SVC_SET_PERMISSION_REQ,
    /// Set permission settings of service response
    GATTM_SVC_SET_PERMISSION_RSP,

    /* Attribute Manipulation */
    /// Get permission settings of attribute request
    GATTM_ATT_GET_PERMISSION_REQ,
    /// Get permission settings of attribute response
    GATTM_ATT_GET_PERMISSION_RSP,
    /// Set permission settings of attribute request
    GATTM_ATT_SET_PERMISSION_REQ,
    /// Set permission settings of attribute response
    GATTM_ATT_SET_PERMISSION_RSP,

    /// Get attribute value request
    GATTM_ATT_GET_VALUE_REQ,
    /// Get attribute value response
    GATTM_ATT_GET_VALUE_RSP,
    /// Set attribute value request
    GATTM_ATT_SET_VALUE_REQ,
    /// Set attribute value response
    GATTM_ATT_SET_VALUE_RSP,

    /* Debug messages */
    /// DEBUG ONLY: Destroy Attribute database request
    GATTM_DESTROY_DB_REQ,
    /// DEBUG ONLY: Destroy Attribute database response
    GATTM_DESTROY_DB_RSP,
    /// DEBUG ONLY: Retrieve list of services request
    GATTM_SVC_GET_LIST_REQ,
    /// DEBUG ONLY: Retrieve list of services response
    GATTM_SVC_GET_LIST_RSP,
    /// DEBUG ONLY: Retrieve information of attribute request
    GATTM_ATT_GET_INFO_REQ,
    /// DEBUG ONLY: Retrieve information of attribute response
    GATTM_ATT_GET_INFO_RSP,
};

/// Add service in database request
struct gattm_add_svc_req
{
    /// Set attribute service start handle
    /// (0 = automatically use first available start handle)
    uint16_t start_hdl;
    /// Task identifier that will manage attribute
    uint16_t task_id;
    /// Total attribute data size
    uint16_t total_size;
    /// Number of attribute with BT 16 UUID
    uint8_t nb_att_uuid_16;
    /// Number of attributes with BT 32bits UUID
    uint8_t nb_att_uuid_32;
    /// Number of attributes with 128bits UUID (proprietary only)
    uint8_t nb_att_uuid_128;
};

/// Add service in database response
struct gattm_add_svc_rsp
{
    /// Start handle of allocated service in attribute database
    uint16_t start_hdl;
    /// Return status of service allocation in attribute database.
    uint8_t status;
};

/// Add attribute in service database request
struct gattm_add_attribute_req
{
    /// Service start handle
    uint16_t start_hdl;
    /// Attribute max data size
    uint16_t max_data_size;
    /// Attribute permissions
    uint16_t perm;
    /// Size of attributes UUID length:
    /// -  2 = 16 bits UUIDs
    /// -  4 = 32 bits UUIDS
    /// - 16 = 128 bits UUIDS
    uint8_t uuid_len;
    /// 2, 4 or 16 bytes array that represents the UUID value
    uint8_t uuid[__ARRAY_EMPTY];
};

/// Add attribute in service database response
struct gattm_add_attribute_rsp
{
    /// Handle of allocated attribute
    uint16_t handle;
    /// Return status of attribute allocation.
    uint8_t status;
};

/* Service management */
/// Get permission settings of service request
struct gattm_svc_get_permission_req
{
    /// Service start attribute handle
    uint16_t start_hdl;
};

/// Get permission settings of service response
struct gattm_svc_get_permission_rsp
{
    /// Service start attribute handle
    uint16_t start_hdl;
    /// service permission
    uint8_t perm;
    /// Return status
    uint8_t status;
};

/// Set permission settings of service request
struct gattm_svc_set_permission_req
{
    /// Service start attribute handle
    uint16_t start_hdl;
    /// service permission
    uint8_t perm;
};

/// Set permission settings of service response
struct gattm_svc_set_permission_rsp
{
    /// Service start attribute handle
    uint16_t start_hdl;
    /// Return status
    uint8_t status;
};


/* Attribute management */
/// Get permission settings of attribute request
struct gattm_att_get_permission_req
{
    /// Handle of the attribute
    uint16_t handle;
};

/// Get permission settings of attribute response
struct gattm_att_get_permission_rsp
{
    /// Handle of the attribute
    uint16_t handle;
    /// Attribute permission
    uint16_t perm;
    /// Return status
    uint8_t status;
};

/// Set permission settings of attribute request
struct gattm_att_set_permission_req
{
    /// Handle of the attribute
    uint16_t handle;
    /// Attribute permission
    uint16_t perm;
};

/// Set permission settings of attribute response
struct gattm_att_set_permission_rsp
{
    /// Handle of the attribute
    uint16_t handle;
    /// Return status
    uint8_t status;
};


/// Get attribute value request
struct gattm_att_get_value_req
{
    /// Handle of the attribute
    uint16_t handle;
};

/// Get attribute value response
struct gattm_att_get_value_rsp
{
    /// Handle of the attribute
    uint16_t handle;
    /// Attribute value length
    uint16_t length;
    /// Return status
    uint8_t status;
    /// Attribute value
    uint8_t value[__ARRAY_EMPTY];
};

/// Set attribute value request
struct gattm_att_set_value_req
{
    /// Handle of the attribute
    uint16_t handle;
    /// Attribute value length
    uint16_t length;
    /// Attribute value
    uint8_t value[__ARRAY_EMPTY];
};

/// Set attribute value response
struct gattm_att_set_value_rsp
{
    /// Handle of the attribute
    uint16_t handle;
    /// Return status
    uint8_t status;
};

/// DEBUG ONLY: Destroy Attribute database request
struct gattm_destroy_db_req
{
    /// New Gap Start Handle
    uint16_t gap_hdl;
    /// New Gatt Start Handle
    uint16_t gatt_hdl;
};

/// DEBUG ONLY: Destroy Attribute database Response
struct gattm_destroy_db_rsp
{
    /// Return status
    uint8_t status;
};

/// DEBUG ONLY: Retrieve list of services response
struct gattm_svc_get_list_rsp
{
    /// Return status
    uint8_t status;
    /// Number of services
    uint8_t nb_svc;
    /// Array of information about services
    struct attm_svc_info svc[__ARRAY_EMPTY];
};

/// DEBUG ONLY: Retrieve information of attribute request
struct  gattm_att_get_info_req
{
    /// Attribute Handle
    uint16_t handle;
};

/// DEBUG ONLY: Retrieve information of attribute response
struct  gattm_att_get_info_rsp
{
    /// Return status
    uint8_t status;
    /// UUID Length
    uint8_t uuid_len;
    /// Attribute Handle
    uint16_t handle;
    /// Attribute Permissions
    uint16_t perm;
    /// UUID value
    uint8_t uuid[GATT_UUID_128_LEN];
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler gattm_state_handler[GATTM_STATE_MAX];
extern const struct ke_state_handler gattm_default_handler;
extern ke_state_t gattm_state[GATTM_IDX_MAX];

#endif /* (BLE_CENTRAL || BLE_PERIPHERAL) */
/// @} GATTMTASK
#endif // GATTM_TASK_H_
