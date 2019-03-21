/****************************************************************************\
* Copyright (C) 2016 Infineon Technologies
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
\****************************************************************************/

/**
* \addtogroup royaleCAPI
* @{
*/

#pragma once

#include <DefinitionsCAPI.h>
#include <CameraDeviceCAPI.h>
#include <TriggerModeCAPI.h>
#include <stdint.h>

ROYALE_CAPI_LINKAGE_TOP

typedef uint64_t royale_cam_manager_hnd;

#define ROYALE_NO_INSTANCE_CREATED 0
#define ROYALE_NO_CAMERA_DEVICES_FOUND NULL

/**
 * create a new CameraManager instance.
 *
 * \return handle if successful, ROYALE_NO_INSTANCE_CREATED otherwise.
 */
ROYALE_CAPI royale_cam_manager_hnd royale_camera_manager_create();

/**
 * create a new CameraManager instance with activation code.
 * see royale::CameraManager::CameraManager()
 *
 * \return handle if successful, ROYALE_NO_INSTANCE_CREATED otherwise.
 */
ROYALE_CAPI royale_cam_manager_hnd royale_camera_manager_create_with_code (const char *activation_code);

/**
 * destroy a CameraManager instance and clean up memory
 */
ROYALE_CAPI void royale_camera_manager_destroy (royale_cam_manager_hnd handle);

/**
 * see royale::CameraManager::getAccessLevel()
 */
ROYALE_CAPI royale_camera_access_level royale_camera_manager_get_access_level (royale_cam_manager_hnd handle, const char *activation_code);

/**
 * see royale::CameraManager::getConnectedCameraList()
 * if nr_cameras > 0 use royale_free_string_array() to free memory of the returned array.
 *
 * \return C string array of connected cameras if at least 1 camera was found, ROYALE_NO_CAMERA_DEVICES_FOUND otherwise
 */
ROYALE_CAPI char **royale_camera_manager_get_connected_cameras (royale_cam_manager_hnd handle, uint32_t *nr_cameras);

/**
 * ! ONLY FOR ANDROID !
 * see royale::CameraManager::getConnectedCameraList()
 * if nr_cameras > 0 use royale_free_string_array() to free memory of the returned array.
 *
 * \return C string array of connected cameras if at least 1 camera was found, ROYALE_NO_CAMERA_DEVICES_FOUND otherwise
 */
ROYALE_CAPI char **royale_camera_manager_get_connected_cameras_android (royale_cam_manager_hnd handle, uint32_t *nr_cameras, uint32_t android_usb_device_fd,
        uint32_t android_usb_device_vid, uint32_t android_usb_device_pid);

/**
 * see royale::CameraManager::createCamera()
 *
 * \return handle if successful, ROYALE_NO_INSTANCE_CREATED otherwise.
 */
ROYALE_CAPI royale_camera_handle royale_camera_manager_create_camera (royale_cam_manager_hnd handle, const char *camera_id);

/**
 * create a camera with a specific trigger
 * see royale::CameraManager::createCamera()
 *
 * \return handle if successful, ROYALE_NO_INSTANCE_CREATED otherwise.
 */
ROYALE_CAPI royale_camera_handle royale_camera_manager_create_camera_with_trigger (royale_cam_manager_hnd handle, const char *camera_id, royale_trigger_mode trigger_mode);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
