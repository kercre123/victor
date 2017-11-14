/**
 ****************************************************************************************
 *
 * @file attm_db.h
 *
 * @brief Header file - ATTM.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef ATTM_DB_H_
#define ATTM_DB_H_

/**
 ****************************************************************************************
 * @addtogroup ATTDB Database
 * @ingroup ATTM
 * @brief Attribute Protocol Database
 *
 * The ATTDB module is responsible for providing different sets of attribute databases
 * for Attribute Profile server.
 *
 * This module can be tailored by client, to match the requirement of the desired database.
 *
 * @{
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include "rwip_config.h"
#include "ke_task.h"
#include "attm_cfg.h"
#include "attm.h"
#include "attm_task.h"
#include "gattm_task.h"

#if (BLE_ATTS)
/*
 * DEFINES
 ****************************************************************************************
 */

/// Retrieve 128 bits UUID from attribute pointer
/// (note: it doesn't check if uuid type is 128 bytes)
#define ATTM_GET_UUID_128(att) \
    ((struct att_uuid_128 *) (((uint8_t*)(att)) - (ATT_UUID_128_LEN - ATT_UUID_16_LEN)))

/// Retrieve 32 bits UUID from attribute pointer
/// (note: it doesn't check if uuid type is 32 bytes)
#define ATTM_GET_UUID_32(att) \
    ((struct att_uuid_32 *) (((uint8_t*)(att)) - (ATT_UUID_32_LEN - ATT_UUID_16_LEN)))

/// update attribute permission on specific handle
#define ATTMDB_UPDATE_ATT_PERM(handle, access, right)\
    attmdb_att_update_perm(handle, (PERM_ACCESS_MASK_ ## access), PERM(access, right))


/// retrieve Max length
#define ATTM_GET_MAX_LENGTH(att) \
    ((att)->max_length & ATT_MAX_LENGTH_MASK)

/// retrieve attribute type
#define ATTM_GET_UUID_TYPE(att) \
    (((att)->max_length & ATT_UUID_TYPE_MASK) >> ATT_UUID_TYPE_SHIFT)

/// retrieve attribute type
#define ATTM_SET_UUID_TYPE(att, type) \
    ((att)->max_length = (((type) << ATT_UUID_TYPE_SHIFT) & ATT_UUID_TYPE_MASK)\
                         | (((att)->max_length) & ATT_MAX_LENGTH_MASK))


/*
 * TYPE DEF
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Add a service in database.
 *
 * According to service start handle and number of attribute, ATTM DB allocate a set of
 * attribute handles, then using other parameters it allocate a buffer used to describe
 * service, and allocate attributes + their values.
 *
 * If start_hdl = 0, it allocated service using first available handle (start_hdl is
 * modified); else it will allocate service according to given start handle.
 *
 * @param[in|out] start_hdl Service start handle.
 * @param[in]     task_id Task that manages service.
 * @param[in]     nb_att_uuid_16 Total Number of attributes using 16 bits UUID.
 * @param[in]     nb_att_uuid_32 Total Number of attributes using 32 bits UUID.
 * @param[in]     nb_att_uuid_128 Number of attributes using 128 bits UUID.
 * @param[in]     total_size Total payload size expected for attribute.
 *
 * @return Command status code:
 *  - @ref ATT_ERR_NO_ERROR: If service allocation succeeds.
 *  - @ref ATT_ERR_INVALID_HANDLE: If start_hdl given in parameter + nb of attribute override
 *                            some existing services handles.
 *  - @ref ATT_ERR_INSUFF_RESOURCE: There is not enough memory to allocate service buffer.
 ****************************************************************************************
 */
uint8_t attmdb_add_service(uint16_t *start_hdl, uint16_t task_id,
                           uint8_t nb_att_uuid_16, uint8_t nb_att_uuid_32,
                           uint8_t nb_att_uuid_128, uint16_t total_size);


/**
 ****************************************************************************************
 * @brief Allocate attribute in service.
 *
 * Called once for each attributes in service database, after allocating service database
 * using @see attmdb_add_service. This function allocates attribute description structure
 * and attribute value into service buffer and update table of handle pointers service
 * description structure. Finally it returns handle of allocated attribute.
 *
 * @param[in] start_hdl Service start handle.
 * @param[in] max_length Maximum attribute value.
 * @param[in] uuid_len  UUID length (2bytes - 16bits, or 4bytes - 32bits,
 *                                   or 16bytes - 128bits UUID).
 * @param[in] uuid      UUID value (as an array of bytes).
 * @param[in] perm      Attribute permission
 *
 * @param[out] handle Allocated attribute handle.
 *
 * @return Command status code:
 *  - @ref ATT_ERR_NO_ERROR: If attribute allocation succeeds.
 *  - @ref ATT_ERR_INVALID_HANDLE: If start handle doesn't match any service
 *  - @ref ATT_ERR_INSUFF_RESOURCE: If new attribute cannot be added because all expected
 *                         attributes already added or buffer overflow detected during
 *                         allocation
 ****************************************************************************************
 */
uint8_t attmdb_add_attribute(uint16_t start_hdl, att_size_t max_length,
                             uint8_t uuid_len, uint8_t* uuid, uint16_t perm,
                             uint16_t *handle);


/**
 ****************************************************************************************
 * @brief Clear database
 *
 * For debug purpose only, this function clear the database and unalloc all services
 * within database.
 *
 * This function shall be used only for qualification and tests in order to manually
 * change database without modifying software.
 ****************************************************************************************
 */
void attmdb_destroy(void);

/**
 ****************************************************************************************
 *  @brief Search in database from which service attribute handle comes from.
 *
 * @param[in] handle Attribute handle.
 *
 * @return Services that contains attribute handle; NULL if handle not available in
 *         database.
 ****************************************************************************************
 */
struct attm_svc_db * attmdb_get_service(uint16_t handle);

/**
 ****************************************************************************************
 *  @brief Search in database Attribute pointer using attribute handle.
 *
 * @param[in] handle Attribute handle.
 *
 * @return Attribute element structure pointer; NULL if handle not available in
 *         database.
 ****************************************************************************************
 */
struct attm_elmt * attmdb_get_attribute(uint16_t handle);

/**
 ****************************************************************************************
 * @brief Retrieve attribute at or after specified handle
 *
 * Retrieve first attribute with handle >= parameter handle.
 * Parameter handle is updated according found attribute.
 *
 * Note: Handles in hidden services or hidden attributes are not returned.
 *
 * @param[in|out] handle Attribute handle.
 *
 * @return Attribute element structure pointer; NULL no more attribute available in
 *         database.
 ****************************************************************************************
 */
struct attm_elmt * attmdb_get_next_att(uint16_t * handle);

/**
 ****************************************************************************************
 * @brief Update attribute value
 *
 * Updating attribute value do not trigger any notification or indication, this shall be
 * handled by GATT task.
 *
 * @param[in] handle Attribute handle.
 * @param[in] length Size of new attribute value
 * @param[in] value  Attribute value payload
 *
 * @return Command status code:
 *  - @ref ATT_ERR_NO_ERROR: If attribute value update succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 *  - @ref ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN: If new value length exceeds maximum attribute
 *                              value length.
 ****************************************************************************************
 */
uint8_t attmdb_att_set_value(uint16_t handle, att_size_t length, uint8_t* value);


/**
 ****************************************************************************************
 * @brief Update attribute with value to append using offset
 *
 * Updating attribute value do not trigger any notification or indication, this shall be
 * handled by GATT task.
 *
 * @param[in] handle Attribute handle.
 * @param[in] length Size of new attribute to append
 * @param[in] offset Data offset from where data shall be applied
 * @param[in] value  Attribute value payload to append
 *
 * @return Command status code:
 *  - @ref ATT_ERR_NO_ERROR: If attribute value update succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 *  - @ref ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN: If new value length exceeds maximum attribute
 *                              value length.
 ****************************************************************************************
 */
uint8_t attmdb_att_update_value(uint16_t handle, att_size_t length, att_size_t offset,
                                uint8_t* value);



/**
 ****************************************************************************************
 * @brief Partially update attribute value with a block value
 *
 * Updating attribute value do not trigger any notification or indication, this shall be
 * handled by GATT task.
 *
 * Block value length + offset shall not exceed attribute length.
 * This function doesn't modify attribute length
 *
 * @param[in] handle Attribute handle.
 * @param[in] offset Data offset from where data shall be apped
 * @param[in] size Size of block value to modify
 * @param[in] blk_value  Block value payload to set in attribute value
 *
 * @return Command status code:
 *  - @ref ATT_ERR_NO_ERROR: If attribute value update succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 *  - @ref ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN: If block value length + offset exceeds
 *                                            attribute value length.
 ****************************************************************************************
 */
uint8_t attmdb_att_partial_value_update(uint16_t handle, att_size_t offset, att_size_t size,
                                        uint8_t* blk_value);

/**
 ****************************************************************************************
 * @brief Retrieve attribute value
 *
 * @param[in]  handle Attribute handle.
 * @param[out] length Size of attribute value
 * @param[out] value  Pointer to attribute value payload
 *
 * @return Command status code:
 *  - @ref ATT_ERR_NO_ERROR: If request succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 ****************************************************************************************
 */
uint8_t attmdb_att_get_value(uint16_t handle, att_size_t* length, uint8_t** value);


/**
 ****************************************************************************************
 * @brief Retrieve attribute UUID
 *
 * @param[in]  handle   Attribute handle.
 * @param[out] uuid_len Size of attribute UUID
 * @param[out] uuid     UUID value to update
 *
 * @return Command status code:
 *  - @ref ATT_ERR_NO_ERROR: If request succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 ****************************************************************************************
 */
uint8_t attmdb_att_get_uuid(uint16_t handle, uint8_t* uuid_len, uint8_t* uuid);

/**
 ****************************************************************************************
 * @brief Update attribute permission
 *
 * @param[in] handle Attribute handle.
 * @param[in] perm   New attribute permission
 *
 * @return Command status code:
 *  - @ref ATT_ERR_NO_ERROR: If request succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 ****************************************************************************************
 */
uint8_t attmdb_att_set_permission(uint16_t handle, uint16_t perm);

/**
 ****************************************************************************************
 * @brief Retrieve attribute permission
 *
 * @param[in]  handle Attribute handle.
 * @param[out] perm   Permission value to return
 *
 * @return Command status code:
 *  - @ref ATT_ERR_NO_ERROR: If request succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 ****************************************************************************************
 */
uint8_t attmdb_att_get_permission(uint16_t handle, uint16_t* perm);

/**
 ****************************************************************************************
 * @brief Reset some permissions bit in the Handle passed as parameter.
 *
 * @param[in] attm_elmt Attribute element for which permission bits have to be reset
 * @param[in] access_mask Access mask of permission to update
 * @param[in] perm  New value of the permission to update
 *
 *
 * @return Command status code:
 *  - @ref ATT_ERR_NO_ERROR: If request succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 ****************************************************************************************
 */
uint8_t attmdb_att_update_perm(uint16_t handle, uint16_t access_mask, uint16_t perm);

/**
 ****************************************************************************************
 * @brief Update attribute service permission
 *
 * @param[in] handle Attribute handle.
 * @param[in] perm   New attribute permission
 *
 * @return Command status code:
 *  - @ref ATT_ERR_NO_ERROR: If request succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 ****************************************************************************************
 */
uint8_t attmdb_svc_set_permission(uint16_t handle, uint8_t perm);

/**
 ****************************************************************************************
 * @brief Retrieve attribute service permission
 *
 * @param[in]  handle Attribute handle.
 * @param[out] perm   Permission value to return
 *
 * @return Command status code:
 *  - @ref ATT_ERR_NO_ERROR: If request succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 ****************************************************************************************
 */
uint8_t attmdb_svc_get_permission(uint16_t handle, uint8_t* perm);

/**
 ****************************************************************************************
 * @brief Initialize Attribute Database (clear it)
 *
 * @param[in] reset  true if it's requested by a reset; false if it's boot initialization
 ****************************************************************************************
 */
void attmdb_init(bool reset);

#if (BLE_DEBUG)

/**
 ****************************************************************************************
 * @brief Retrieve number of services.
 *
 * @return number of services
 ****************************************************************************************
 */
uint8_t attmdb_get_nb_svc(void);

/**
 ****************************************************************************************
 * @brief Retrieve services informations
 *
 * @param[in] svc_info Services information array to update
 ****************************************************************************************
 */
void attmdb_get_svc_info(struct attm_svc_info* svc_info);
#endif /* (BLE_DEBUG) */
#endif // #if (BLE_ATTS)

/// @} ATTDB
#endif // ATTM_DB_H_
