/**
 ****************************************************************************************
 *
 * @file gattm.h
 *
 * @brief Header file - GATT Manager.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef GATTM_H_
#define GATTM_H_



/**
 ****************************************************************************************
 * @addtogroup GATTM Generic Attribute Profile Manager
 * @ingroup GATT
 * @brief Generic Attribute Profile.
 *
 * The GATT manager module is responsible for providing an API for all action operations
 * not related to a connection. It's responsible to managing internal database.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
/* kernel task */
#include "rwip_config.h"
#include "ke_task.h"

#if (BLE_CENTRAL || BLE_PERIPHERAL)
/*
 * DEFINES
 ****************************************************************************************
 */

#if (BLE_ATTS)
/// retrieve gatt attribute handle from attribute index.
#define GATT_GET_ATT_HANDLE(idx)\
    ((gattm_env.svc_start_hdl == 0)? (0) :(gattm_env.svc_start_hdl + (idx)))
#endif // (BLE_ATTS)

/// GATT General Information Manager
struct gattm_env_tag
{
    /// Task ID used to request database operations.
    ke_task_id_t requester;

    #if (BLE_ATTS)
    /// GATT service start handle
    uint16_t svc_start_hdl;
    #endif // (BLE_ATTS)

    /// Maximum device MTU size
    uint16_t max_mtu;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
extern struct gattm_env_tag gattm_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the GATT manager module.
 * This function performs all the initialization steps of the GATT module.
 *
 * @param[in] reset  true if it's requested by a reset; false if it's boot initialization
 *
 ****************************************************************************************
 */
void gattm_init(bool reset);


/**
 ****************************************************************************************
 * @brief Initialize GATT resources for connection.
 *
 * @param[in] conidx connection record index
 * @param[in] role   device role after connection establishment
 *
 ****************************************************************************************
 */
void gattm_create(uint8_t conidx);
/**
 ****************************************************************************************
 * @brief Cleanup GATT resources for connection
 *
 * @param[in] conidx   connection record index
 *
 ****************************************************************************************
 */
void gattm_cleanup(uint8_t conidx);

#if (BLE_ATTS)
/**
 ****************************************************************************************
 * @brief Return the start handle of the GATT service in the database *
 ****************************************************************************************
 */
uint16_t gattm_svc_get_start_hdl(void);
#endif //(BLE_ATTS)

/**
 ****************************************************************************************
 * @brief Return the maximal MTU value
 *
 * @param[out] Maximal MTU value
 ****************************************************************************************
 */
uint16_t gattm_get_max_mtu(void);

/**
 ****************************************************************************************
 * @brief Set the maximal MTU value
 *
 * @param[in] mtu   Max MTU value (Minimum is 23)
 ****************************************************************************************
 */
void gattm_set_max_mtu(uint16_t mtu);

#endif /* (BLE_CENTRAL || BLE_PERIPHERAL) */

/// @} GATTM
#endif // GATTM_H_
