/**
 ****************************************************************************************
 *
 * @file gattc.h
 *
 * @brief Header file - GATT Controller.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef GATTC_H_
#define GATTC_H_

/**
 ****************************************************************************************
 * @addtogroup GATTC Generic Attribute Profile Controller
 * @ingroup GATT
 * @brief Generic Attribute Profile Controller.
 *
 * This GATT module is responsible for providing an API for all attribute related operations
 * related to a BLE connection.
 * It is responsible for all the service framework activities using the Attribute protocol
 * for discovering services and for reading and writing characteristic values on a peer device.
 * To achieve this, the GATT interfaces with @ref ATTC "ATTC" and the @ref ATTS "ATTS".
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
#include "gattc_task.h"
#include "gatt.h"
#include "attc_task.h"
/*
 * DEFINES
 ****************************************************************************************
 */
///Maximum profile registrations for receiving ind/ntf for handlers
#define GATTC_MAX_PRF_REGISTR                        4


/*
 * MACRO DEFINITIONS
 ****************************************************************************************
 */


/// Allocate a Attribute PDU packet for a specific attribute request.
#define GATTC_ALLOCATE_ATT_REQ(conidx, opcode, pdu_type, value_len)\
    (struct pdu_type*) gattc_allocate_att_req(conidx, opcode, value_len)


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Peer device event registration
struct gattc_register_evt
{
    /// Attribute start handle
    uint16_t start_hdl;
    /// Attribute end handle
    uint16_t end_hdl;
    /// Task to be notified
    ke_task_id_t task;
};

/// GATT controller environment variable structure.
struct gattc_env_tag
{
    /// Request operation Kernel message
    void* operation;
    #if (BLE_ATTC)
    /// store response until operation is completed
    void* response;
    /// List of ATT READ RSP used to aggregate read long in a single buffer.
    struct co_list read_rsp_list;
    /// Array that contains peer device event registration
    struct gattc_register_evt reg_evt[GATTC_MAX_PRF_REGISTR];
    #endif // (BLE_ATTC)
    /// Operation requester task id
    ke_task_id_t requester;
    /// Current MTU Size
    uint16_t mtu_size;
#ifdef __DA14581__
    /// check if operation is present in heap.
    bool op_in_heap;
#endif
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
extern struct gattc_env_tag* gattc_env[GATTC_IDX_MAX];

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @brief Initialization of the GATT controller module.
 * This function performs all the initialization steps of the GATT module.
 *
 * @param[in] reset  true if it's requested by a reset; false if it's boot initialization
 *
 ****************************************************************************************
 */
void gattc_init(bool reset);


/**
 ****************************************************************************************
 * @brief Initialize GATT controller for connection.
 *
 * @param[in] conidx    connection record index
 * @param[in] role   device role after connection establishment
 *
 ****************************************************************************************
 */
void gattc_create(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Cleanup GATT controller resources for connection
 *
 * @param[in] conidx   connection record index
 *
 ****************************************************************************************
 */
void gattc_cleanup(uint8_t conidx);


#if ((BLE_CENTRAL || BLE_PERIPHERAL) && BLE_ATTC)
/**
 ****************************************************************************************
 * @brief Sends write execute.
 *
 * @param[in] conidx        connection index
 * @param[in] flag          write execute flag (write or discard)
 *
 ****************************************************************************************
 */
void gattc_send_execute(uint8_t conidx, uint8_t flag);
#endif // ((BLE_CENTRAL || BLE_PERIPHERAL) && BLE_ATTC)

/**
 ****************************************************************************************
 * @brief Get the event message destination
 *
 * @param[in] conidx         Connection index
 * @param[in] handle         Attribute handle
 *
 * @return task identifier
 *
 ****************************************************************************************
 */
ke_task_id_t gattc_get_event_dest(uint8_t conidx, uint16_t handle);

/**
 ****************************************************************************************
 * @brief Initialize an operation request.
 *
 * @param[in] conidx         Connection index
 * @param[in] requester      Task requesting operation
 * @param[in] operation      Requested operation
 *
 ****************************************************************************************
 */
void gattc_init_operation(uint8_t conidx, ke_task_id_t requester, void* operation);

/**
 ****************************************************************************************
 * @brief Get operation on going
 *
 * @param[in] conidx         connection index
 *
 * @return operation code on going
 *
 ****************************************************************************************
 */
uint8_t gattc_get_operation(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Sends the command complete event to upper layer.
 *
 * @param[in] conidx        connection index
 * @param[in] status        status of the complete event
 *
 ****************************************************************************************
 */
void gattc_send_complete_evt(uint8_t conidx, uint8_t status);


#if (BLE_ATTC)
/**
 ****************************************************************************************
 * @brief Allocate Attribute request PDU packet to sent
 *
 * @param[in] conidx  Index of the connection with the peer device
 * @param[in] opcode  Operation code of the PDU packet to send.
 ****************************************************************************************
 */
void* gattc_allocate_att_req(uint8_t conidx, uint8_t opcode, uint16_t value_len);

/**
 ****************************************************************************************
 * @brief Send a PDU Attribute request packet
 *
 * @param[in] pdu        PDU Packet
 ****************************************************************************************
 */
void gattc_send_att_req(void *pdu);
#endif // (BLE_ATTC)

/**
 ****************************************************************************************
 * @brief Gets the negotiated MTU. This function gets the negotiated MTU.
 *
 * @param[in] idx   connection record index
 *
 * @return MTU negotiated
 *
 ****************************************************************************************
 */
uint16_t gattc_get_mtu(uint8_t idx);

/**
 ****************************************************************************************
 * @brief Sets the negotiated MTU This function stores the negotiated MTU.
 *
 * @param[in] idx   connection record index
 * @param[in] mtu   negotiated MTU
 *
 * @return status   indicates if the MTU setting operation is successful
 *
 ****************************************************************************************
 */
void gattc_set_mtu(uint8_t idx, uint16_t mtu);
/// @} GATTC
#endif // GATTC_H_
