/**
 ****************************************************************************************
 *
 * @file prf_utils.h
 *
 * @brief  Header file - Profile Utilities
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _PRF_UTILS_H_
#define _PRF_UTILS_H_

/**
 ****************************************************************************************
 * @addtogroup PRF_UTILS
 * @ingroup PROFILE
 *
 * @brief Definitions of shared profiles functions that can be used by several profiles
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_SERVER_PRF || BLE_CLIENT_PRF)
#include "ke_msg.h"
#include "prf_types.h"
#include "gattc_task.h"
#include "gapc.h"
#include "gapc_task.h"
#include "attm_db.h"
#endif /* (BLE_SERVER_PRF || BLE_CLIENT_PRF) */

/*
 * MACROS
 ****************************************************************************************
 */

#if (BLE_CLIENT_PRF || BLE_TIP_SERVER || BLE_AN_SERVER || BLE_PAS_SERVER)
/**
 ****************************************************************************************
 * @brief Macro used to called the prf_client_enable function.
 *
 * @param con_info      Connection Information
 * @param param         [PRF]_ENABLE_REQ message structure
 * @param type          Profile task type (Lowercase, ex: htpc, disc, ...)
 ****************************************************************************************
 */
#define PRF_CLIENT_ENABLE(con_info, param, type) \
        (prf_client_enable((prf_env_struct ***)&type ## _envs, &(con_info), (param), sizeof(struct type ## _env_tag)))

/**
 ****************************************************************************************
 * @brief Macro used to called the prf_client_reset function.
 *
 * @param envs      Pool of environments
 * @param type      Profile task type (Uppercase, ex: HTPC, DISC, ...)
 ****************************************************************************************
 */
#define PRF_CLIENT_RESET(envs, type) \
        (prf_client_reset((prf_env_struct ***)&(envs), TASK_ ## type, type ## _IDLE, &TASK_DESC_ ## type, type ## _CLEANUP_FNCT))

/**
 ****************************************************************************************
 * @brief Macro used to called the prf_client_get_env function.
 *
 * @param prf_id   Profile Task ID
 * @param type      Profile task type (Uppercase, ex: HTPC, DISC, ...)
 ****************************************************************************************
 */
#define PRF_CLIENT_GET_ENV(prf_id, type) \
        ((struct type ## _env_tag *)prf_client_get_env((prf_env_struct **)type ## _envs, prf_id))

/**
 ****************************************************************************************
 * @brief Macro used to called the prf_client_disable_ind_send function.
 *
 * @param envs     Pool of environments
 * @param prf_id   Profile Task ID
 * @param type     Profile task type (Uppercase, ex: HTPC, DISC, ...)
 ****************************************************************************************
 */
#define PRF_CLIENT_DISABLE_IND_SEND(envs, task_id, type, conhdl) \
        (prf_client_disable_ind_send((prf_env_struct ***)&envs, type ## _DISABLE_IND, task_id, type ## _IDLE, (conhdl), type ## _CLEANUP_FNCT))

/**
 ****************************************************************************************
 * @brief Macro used to called the prf_client_disable_ind_send function.
 *
 * @param envs     Pool of environments
 * @param prf_id   Profile Task ID
 * @param type     Profile task type (Uppercase, ex: HTPC, DISC, ...)
 ****************************************************************************************
 */
#define PRF_CLIENT_ENABLE_ERROR(envs, prf_task_id, type) \
        (prf_client_enable_error((prf_env_struct ***)&envs, prf_task_id, type ## _DISCOVERING, type ## _IDLE, type ## _CLEANUP_FNCT))

/**
 ****************************************************************************************
 * @brief Macro used to called the prf_client_disable function.
 *
 * @param envs     Pool of environments
 * @param conidx   connection index
 * @param type     Profile task type (Uppercase, ex: HTPC, DISC, ...)
 ****************************************************************************************
 */
#define PRF_CLIENT_DISABLE(envs, conidx, type) \
        (prf_client_disable((prf_env_struct ***)&envs, (conidx), type ## _CLEANUP_FNCT))

#endif //(BLE_CLIENT_PRF || BLE_TIP_SERVER || BLE_AN_SERVER || BLE_PAS_SERVER)



/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

#if (BLE_BAS_SERVER)
/**
 ****************************************************************************************
 * @brief Pack Characteristic Presentation Format descriptor value
 ****************************************************************************************
 */
void prf_pack_char_pres_fmt(uint8_t *packed_val, const struct prf_char_pres_fmt* char_pres_fmt);
#endif // (BLE_BAS_SERVER)

#if (BLE_BAS_CLIENT)
/**
 ****************************************************************************************
 * @brief Unpack Characteristic Presentation Format descriptor value
 ****************************************************************************************
 */
void prf_unpack_char_pres_fmt(const uint8_t *packed_val, struct prf_char_pres_fmt* char_pres_fmt);
#endif // (BLE_BAS_CLIENT)

#if (BLE_CLIENT_PRF)
/**
 ****************************************************************************************
 * @brief Request  peer device to read an attribute
 *
 * @param con_info Pointer to connection information (connection handle, app task id,
 *                 profile task id)
 *
 * @param shdl     Search Start Handle
 * @param ehdl     Search End Handle
 *
 * @param valhdl   Value Handle
 *
 * @note: if attribute is invalid, nothing is registered
 ****************************************************************************************
 */
void prf_read_char_send(struct prf_con_info* con_info,
                        uint16_t shdl, uint16_t ehdl, uint16_t valhdl);


/**
 ****************************************************************************************
 * @brief register attribute handle in GATT
 *
 * @param con_info Pointer to connection information (connection handle, app task id,
 *                 profile task id)
 *
 * @param svc   Service to register
 *
 * @note: if attribute is invalid, nothing is registered
 ****************************************************************************************
 */
void prf_register_atthdl2gatt(struct prf_con_info* con_info, struct prf_svc *svc);

/**
 ****************************************************************************************
 * @brief Unregister attribute handle in GATT
 *
 * @param con_info Pointer to connection information (connection handle, app task id,
 *                 profile task id)
 *
 * @param svc   Service to register
 *
 * @note: if attribute is invalid, nothing is registered
 ****************************************************************************************
 */
void prf_unregister_atthdl2gatt(struct prf_con_info* con_info, struct prf_svc *svc);

/**
 ****************************************************************************************
 * @brief Request service discovery on peer device.
 *
 * This request will be used to retrieve start and end handles of the service.
 *
 * @param con_info Pointer to connection information (connection handle, app task id,
 *                 profile task id)
 *
 * @param uuid     Service UUID
 ****************************************************************************************
 */
void prf_disc_svc_send(struct prf_con_info* con_info, uint16_t uuid);

/**
 ****************************************************************************************
 * @brief Request included service discovery on peer device.
 *
 * This request will be used to retrieve included services of a primary service
 *
 * @param con_info Pointer to connection information (connection handle, app task id,
 *                 profile task id)
 *
 * @param svc      Service
 ****************************************************************************************
 */
void prf_disc_incl_svc_send(struct prf_con_info* con_info, struct prf_svc *svc);

/**
 ****************************************************************************************
 * @brief Request service characteristic discovery on peer device.
 *
 * This request service to provide all its service characteristic information.
 *
 * @param con_info Pointer to connection information (connection handle, app task id,
 *                 profile task id)
 *
 * @param svc      pointer to service handles (start and end handles)
 ****************************************************************************************
 */
void prf_disc_char_all_send(struct prf_con_info* con_info, struct prf_svc * svc);


/**
 ****************************************************************************************
 * @brief Request characteristic descriptor discovery in a generic manner.
 *
 * It will request discovery of all characteristic descriptors for the giben service
 *
 * @param con_info pointer to connection information (connection handle, app task id,
 *                 profile task id)
 *
 * @param svc      pointer to service handles (start and end handles)
 ****************************************************************************************
 */
void prf_disc_char_desc_send(struct prf_con_info* con_info, struct prf_char_inf * charact);



/**
 ****************************************************************************************
 * @brief Write peer characteristic using GATT.
 *
 * It will request write modification of peer handle
 *
 * @param[in] con_info pointer to connection information (connection handle, app task id,
 *                 profile task id)
 * @param[in] handle Peer handle to modify
 * @param[in] value  New Peer handle value
 * @param[in] length Value length
 ****************************************************************************************
 */
void prf_gatt_write(struct prf_con_info* con_info,
                    uint16_t handle, uint8_t* value, uint16_t length, uint8_t req_type);

/**
 ****************************************************************************************
 * @brief Modify peer client configuration descriptor using GATT
 *
 * It will request write modification of peer client configuration descriptor handle
 *
 * @param[in] con_info pointer to connection information (connection handle, app task id,
 *                 profile task id)
 *
 * @param[in] handle Peer client configuration descriptor handle to modify
 *
 * @param[in] ntf_ind_cfg  Indication/Notification configuration
 ****************************************************************************************
 */
void prf_gatt_write_ntf_ind(struct prf_con_info* con_info, uint16_t handle,
        uint16_t ntf_ind_cfg);

/**
 ****************************************************************************************
 * @brief Check service characteristic validity
 *
 * For each characteristic in service it verifies handles.
 *
 * If some handles are not present, it checks if they shall be present or they are optional.
 *
 * @param nb_chars   Number of Characteristics in the service
 * @param chars      Characteristics values (char handles, val handles, properties)
 * @param chars_req  Characteristics requirements.
 *
 * @return 0x1 if service is valid, 0x00 else.
 ****************************************************************************************
 */
uint8_t prf_check_svc_char_validity(uint8_t nb_chars,
                                    const struct prf_char_inf* chars,
                                    const struct prf_char_def* chars_req);

/**
 ****************************************************************************************
 * @brief Check service characteristic descriptors validity
 *
 * For each characteristic descriptors in service it verifies handles.
 *
 * If some handles are not present, according to characteristic properties it verify if
 * descriptor is optional or not.
 *
 * @param descs_size Number of Characteristic descriptors in the service
 * @param descs      Characteristic descriptors values (handles)
 * @param descs_req  Characteristics descriptors requirements.
 *
 * @return 0x1 if service is valid, 0x00 else.
 ****************************************************************************************
 */
uint8_t prf_check_svc_char_desc_validity(uint8_t descs_size,
                                        const struct prf_char_desc_inf* descs,
                                        const struct prf_char_desc_def* descs_req,
                                        const struct prf_char_inf* chars);

/**
 ****************************************************************************************
 * @brief Look for requested characteristics in the list provided by the
 * gattc_disc_char_ind message.
 *
 * @param svc_ehdl         End handle of the service for which characteristics are discovered
 * @param nb_chars         Length of provided arrays (chars and chars_req)
 * @param chars            Characteristics
 * @param chars_req        Characteristics requirements
 * @param param            List of found characteristics
 * @param last_found_char  Characteristics code of the last characteristic that has been found.
 ****************************************************************************************
 */
void prf_search_chars(uint16_t svc_ehdl, uint8_t nb_chars,
                      struct prf_char_inf* chars, const struct prf_char_def* chars_req,
                      const struct gattc_disc_char_ind* param,
                      uint8_t* last_found_char);

/**
 ****************************************************************************************
 * @brief Look for requested descriptors in the list provided by the
 * gattc_disc_char_ind message.
 *
 * @param nb_descs         Length of provided arrays (descs and descs_req)
 * @param descs            Descriptors
 * @param descs_req        Descriptors requirements
 * @param param            List of found descriptors
 * @param last_char_code   Characteristics code of the characteristic for which the
 *                         gatt_disc_char_desc_req has been sent
 ****************************************************************************************
 */
void prf_search_descs(uint8_t nb_descs,
                      struct prf_char_desc_inf* descs, const struct prf_char_desc_def* descs_req,
                      const struct gattc_disc_char_desc_ind* param,
                      uint8_t last_char_code);

#endif //(BLE_CLIENT_PRF)

#if (BLE_CLIENT_PRF || BLE_TIP_SERVER || BLE_AN_SERVER || BLE_PAS_SERVER)

/**
 ****************************************************************************************
 * @brief This function is called from a profile client role task to allocate an environment
 * for a specified connection (p_con_info). If the prf_id parameter doesn't match the connection
 * index, the [PRF]_ENABLE_REQ message (p_param) is forwarded to the good profile task id.
 *
 * An environment whose size is env_size will be added in the pool of environments (p_envs).
 * This pool is allocated if it didn't exist.
 *
 * @param[in] p_con_info       Pointer to the connection information structure
 * @param[in] p_param          Pointer to task enable request message
 * @param[in/out] p_envs       Pointer to the profile environment variable
 * @param[in] env_size         Profile Client Role Task Environment size
 *
 * @return  PRF_ERR_OK                      If the environment has been successfully allocated.
 *          PRF_ERR_REQ_DISALLOWED          If the allocation has failed because of a lack of memory.
 *          PRF_ERR_INVALID_PARAM           The profile is already enable for this connection.
 *          PRF_ERR_FEATURE_NOT_SUPPORTED   The message has been forwarded to another task instance.
 ****************************************************************************************
 */
uint8_t prf_client_enable(prf_env_struct ***p_envs, struct prf_con_info *p_con_info,
                          void const *p_param, uint16_t env_size);

/**
 ****************************************************************************************
 * @brief This function is called from a profile client role task to free an environment
 * for a specified connection (idx). If the removed environment was the last one for the
 * profile, the pool of environment is free.
 *
 * @param[in/out] p_envs    Pointer to the profile environment variable
 * @param[in] idx           Connection Index
 * @param[in] cleanup       Clean-up function used to clean environment variables
 *                          allocated by profile.
 *
 * @return PRF_ERR_OK               If the operation succeed.
 * @return PRF_ERR_INVALID_PARAM    If the environment for the provided connection index
 *                                  doesn't exist.
 ****************************************************************************************
 */
uint8_t prf_client_disable(prf_env_struct ***p_envs, uint8_t idx,
        prf_cleanup_cb cleanup);

/**
 ****************************************************************************************
 * @brief Reset all instances of a profile client role task.
 * All environments are removed and the pool of environment is removed. The state of all task
 * is set to state.
 *
 * @param[in/out] p_envs        Pointer to the profile environment variable
 * @param[in] task_type         Task type of the profile
 * @param[in] state             State of each task instance after the reset.
 * @param[in] cleanup           Clean-up function used to clean environment variables
 *                              allocated by profile.
 ****************************************************************************************
 */
void prf_client_reset(prf_env_struct ***p_envs, ke_task_id_t task_type,
        ke_state_t state, struct ke_task_desc const * p_task_desc,
        prf_cleanup_cb cleanup);

/**
 ****************************************************************************************
 * @brief Return the address of the profile client role environment linked to the profile
 * task instance whose id is task_id.
 *
 * @param[in/out] p_envs        Pointer to the profile environment variable
 * @param[in] task_id           Profile task id
 *
 * @return the address of the required profile client role environment
 ****************************************************************************************
 */
prf_env_struct *prf_client_get_env(prf_env_struct **p_envs, ke_task_id_t task_id);

/**
 ****************************************************************************************
 * @brief The function is used in a profile client role task to inform the application that
 * the profile has been disabled after a disconnection.
 *
 * @param[in/out] p_envs        Pointer to the profile environment variable
 * @param msg_id                Message ID of the [PRF]_DISABLE_IND message
 * @param task_id               Task ID of the disabled profile client role task
 * @param state                 State of the task after the disabling.
 * @param conhdl                Connection Handle TODO[FBE] remove it as soon as conhdl remove for external API
 * @param[in] cleanup           Clean-up function used to clean environment variables
 *                              allocated by profile.
 ****************************************************************************************
 */
void prf_client_disable_ind_send(prf_env_struct ***p_envs, ke_msg_id_t msg_id,
                                 ke_task_id_t task_id, uint8_t state, uint16_t conhdl,
                                 prf_cleanup_cb cleanup );

/**
 ****************************************************************************************
 * @brief The function is used when an error has been raised during the enabling of a profile
 * client role task. Its purpose is to check the current state of the task in order to come
 * back to the initial configuration if needed
 *
 * @param[in/out] p_envs        Pointer to the profile environment variable
 * @param[in] prf_task_id       Profile Task ID
 * @param[in] disc_state        Task discovering state
 * @param[in] idle_state        Task idle state
 * @param[in] cleanup           Clean-up function used to clean environment variables
 *                              allocated by profile.
 ****************************************************************************************
 */
void prf_client_enable_error(prf_env_struct ***p_envs, ke_task_id_t prf_task_id,
                             ke_state_t disc_state, ke_state_t idle_state,
                             prf_cleanup_cb cleanup);



/**
 ****************************************************************************************
 * @brief The function is used to send information about peer attribute value
 *
 * @param[in] p_envs        Pointer to the profile environment variable
 * @param[in] msg_id        Profile message ID to trigger
 * @param[in] status        Response status code
 * @param[in] read_ind      GATT read message indication
 ****************************************************************************************
 */
void prf_client_att_info_rsp(prf_env_struct *p_env, uint16_t msg_id,
                             uint8_t status, struct gattc_read_ind const* read_ind);

#endif //(BLE_CLIENT_PRF || BLE_TIP_SERVER || BLE_AN_SERVER || BLE_PAS_SERVER)

#if (BLE_SERVER_PRF)

/**
 ****************************************************************************************
 * @brief The function is used in a profile server role task to inform the application that
 * an error has occurred in the msg_id message handler.
 *
 * @param p_env                 Profile server role task environment
 * @param status                Error status
 * @param ind_msg_id            [PRF]_ERROR_IND message id
 * @param msg_id                Message ID of the message for which an error has been raised
 ****************************************************************************************
 */
void prf_server_error_ind_send(prf_env_struct *p_env, uint8_t status,
                               ke_msg_id_t ind_msg_id, ke_msg_id_t msg_id);



/**
 ****************************************************************************************
 * @brief The function is used in a profile server role task to trigger an event to peer
 * device (indication/notification) about new attribute value modification.
 *
 * @param p_env                 Profile server role task environment
 * @param indication            True if it's an indication, False if it's a notification
 * @param handle                Attribute handle that triggers an event
 ****************************************************************************************
 */
void prf_server_send_event(prf_env_struct *p_env, bool indication,
                           uint16_t handle);

#endif //(BLE_SERVER_PRF)

#if (BLE_SERVER_PRF || BLE_CLIENT_PRF)
/**
 ****************************************************************************************
 * @brief Pack date time value
 *
 * @param[out] packed_date packed date time
 * @param[in] date_time structure date time
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t prf_pack_date_time(uint8_t *packed_date, const struct prf_date_time* date_time);

/**
 ****************************************************************************************
 * @brief Unpack date time value
 *
 * @param[in] packed_date packed date time
 * @param[out] date_time structure date time
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t prf_unpack_date_time(uint8_t *packed_date, struct prf_date_time* date_time);

/**
 ****************************************************************************************
 * @brief Perform Profile initialization
 ****************************************************************************************
 */
void prf_init(void);


/**
 ****************************************************************************************
 * @brief Link creation event, update profiles states.
 *
 * @param[in] conidx        connection index
 *
 ****************************************************************************************
 */
void prf_create(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Link disconnection event, clean-up profiles.
 *
 * @param[in] conidx        connection index
 * @param[in] conhdl        connection handle
 * @param[in] reason        detach reason
 *
 ****************************************************************************************
 */
void prf_cleanup(uint8_t conidx, uint16_t conhdl, uint8_t reason);
#endif /* (BLE_SERVER_PRF || BLE_CLIENT_PRF) */

/// @} prf_utils

#endif /* _PRF_UTILS_H_ */
