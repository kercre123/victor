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

#include <CAPIVersion.h>
#include <DefinitionsCAPI.h>
#include <DepthDataCAPI.h>
#include <DepthImageCAPI.h>
#include <ExtendedDataCAPI.h>
#include <IRImageCAPI.h>
#include <SparsePointCloudCAPI.h>
#include <ExposureModeCAPI.h>
#include <RecordCAPI.h>
#include <ExposureCAPI.h>
#include <StatusCAPI.h>
#include <EventCAPI.h>
#include <ProcessingParametersCAPI.h>
#include <DataStructuresCAPI.h>
#include <LensParametersCAPI.h>
#include <stdint.h>
#include <stdbool.h>

ROYALE_CAPI_LINKAGE_TOP

typedef uint64_t royale_camera_handle;

typedef enum royale_camera_access_level
{
    ROYALE_ACCESS_LEVEL1 = 1,               //!< Level 1 access provides the depth data using standard, known-working configurations
    ROYALE_ACCESS_LEVEL2 = 2,               //!< Level 2 access provides the raw data, for example for developing custom processing pipelines
    ROYALE_ACCESS_LEVEL3 = 3,               //!< Level 3 access enables you to overwrite exposure limits
    ROYALE_ACCESS_LEVEL4 = 4                //!< Level 4 access is for bringing up new camera modules
} royale_camera_access_level;

typedef royale_pair_uint32_uint32 royale_exposure_limits;

// ----------------------------------------------------------------------------------------------
// Level 1: Standard users (Laser Class 1 guaranteed)
// ----------------------------------------------------------------------------------------------

/**
* Destroy a camera device instance and clean up memory
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI void royale_camera_device_destroy_v210 (royale_camera_handle handle);

/**
* see royale::ICameraDevice::initialize()
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_initialize_v210 (royale_camera_handle handle);

/**
* see royale::ICameraDevice::getId()
* @param[in] handle camera device instance handle.
* @param[out] camera_id pointer where the C string should be written to.
* royale_free_string() has to be used on camera_id when it is not needed anymore.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_id_v220 (royale_camera_handle handle, char **camera_id);

/**
* see royale::ICameraDevice::getCameraName()
* @param[in] handle camera device instance handle.
* @param[out] camera_name pointer where the C string should be written to.
* royale_free_string() has to be used on the resulting string.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_camera_name_v220 (royale_camera_handle handle, char **camera_name);

/**
* see royale::ICameraDevice::getCameraInfo()
* @param[in] handle camera device instance handle.
* @param[out] info pointer where the resulting structure array should be written to.
* @param[out] nr_info_entries pointer where the number of entries in the resulting array should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_camera_info_v220 (royale_camera_handle handle, royale_pair_string_string **info, uint32_t *nr_info_entries);

/**
* see royale::ICameraDevice::setUseCase()
* @param[in] handle camera device instance handle.
* @param[in] use_case_name C string containing the use case name.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_use_case_v210 (royale_camera_handle handle, const char *use_case_name);

/**
* see royale::ICameraDevice::getUseCases()
* in case of ROYALE_STATUS_SUCCESS use ::royale_free_string_array() to free memory of the returned array.
* @param[in] handle camera device instance handle.
* @param[out] use_cases pointer where the resulting C String array should be written to.
* @param[out] nr_use_cases pointer where the number of entries in the resulting array should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_use_cases_v220 (royale_camera_handle handle, char ***use_cases, uint32_t *nr_use_cases);

/**
* see royale::ICameraDevice::getCurrentUseCase()
* royale_free_string() has to be used on the resulting string.
* @param[in] handle camera device instance handle.
* @param[out] use_case_name pointer where the resulting C string should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_current_use_case_v220 (royale_camera_handle handle, char **use_case_name);

/**
* [deprecated]
* see royale::ICameraDevice::setExposureTime()
* @param[in] handle camera device instance handle.
* @param[in] exposure_time the desired exposure time.
*/
ROYALE_CAPI_DEPRECATED royale_camera_status royale_camera_device_set_exposure_time_v210 (royale_camera_handle handle, uint32_t exposure_time);

/**
* see royale::ICameraDevice::setExposureTime()
* @param[in] handle camera device instance handle.
* @param[in] stream_id stream id (may be 0 if there is only one stream).
* @param[in] exposure_time the desired exposure time.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_exposure_time_v300 (royale_camera_handle handle, royale_stream_id stream_id, uint32_t exposure_time);

/**
* [deprecated]
* see royale::ICameraDevice::setExposureMode()
* @param[in] handle camera device instance handle.
* @param[in] exposure_mode the desired exposure mode.
*/
ROYALE_CAPI_DEPRECATED royale_camera_status royale_camera_device_set_exposure_mode_v210 (royale_camera_handle handle, royale_exposure_mode exposure_mode);

/**
* see royale::ICameraDevice::setExposureMode()
* @param[in] handle camera device instance handle.
* @param[in] stream_id stream id (may be 0 if there is only one stream).
* @param[in] exposure_mode the desired exposure mode.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_exposure_mode_v300 (royale_camera_handle handle, royale_stream_id stream_id, royale_exposure_mode exposure_mode);

/**
* [deprecated]
* see royale::ICameraDevice::getExposureMode()
* @param[in] handle camera device instance handle.
* @param[out] exposure_mode pointer where the resulting royale_exposure_mode should be written to.
*/
ROYALE_CAPI_DEPRECATED royale_camera_status royale_camera_device_get_exposure_mode_v220 (royale_camera_handle handle, royale_exposure_mode *exposure_mode);

/**
* see royale::ICameraDevice::getExposureMode()
* @param[in] handle camera device instance handle.
* @param[in] stream_id stream id (may be 0 if there is only one stream).
* @param[out] exposure_mode pointer where the resulting royale_exposure_mode should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_exposure_mode_v300 (royale_camera_handle handle, royale_stream_id stream_id, royale_exposure_mode *exposure_mode);

/**
* [deprecated]
* see royale::ICameraDevice::getExposureLimits()
* @param[in] handle camera device instance handle.
* @param[out] lower_limit pointer where the lower exposure limit should be written to.
* @param[out] upper_limit pointer where the upper exposure limit should be written to.
*/
ROYALE_CAPI_DEPRECATED royale_camera_status royale_camera_device_get_exposure_limits_v220 (royale_camera_handle handle, uint32_t *lower_limit, uint32_t *upper_limit);

/**
* see royale::ICameraDevice::getExposureLimits()
* @param[in] handle camera device instance handle.
* @param[in] stream_id stream id (may be 0 if there is only one stream).
* @param[out] lower_limit pointer where the lower exposure limit should be written to.
* @param[out] upper_limit pointer where the upper exposure limit should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_exposure_limits_v300 (royale_camera_handle handle, royale_stream_id stream_id, uint32_t *lower_limit, uint32_t *upper_limit);

/**
* see royale::ICameraDevice::startCapture()
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_start_capture_v210 (royale_camera_handle handle);

/**
* see royale::ICameraDevice::stopCapture()
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_stop_capture_v210 (royale_camera_handle handle);

/**
* see royale::ICameraDevice::getMaxSensorWidth()
* @param[in] handle camera device instance handle.
* @param[out] max_sensor_width pointer where the max sensor width should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_max_sensor_width_v220 (royale_camera_handle handle, uint16_t *max_sensor_width);

/**
* see royale::ICameraDevice::getMaxSensorHeight()
* @param[in] handle camera device instance handle.
* @param[out] max_sensor_height pointer where the max sensor height should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_max_sensor_height_v220 (royale_camera_handle handle, uint16_t *max_sensor_height);

/**
* see royale::ICameraDevice::getLensParameters()
* use ::royale_free_lens_parameters() to free allocated memory within the structure.
* @param[in] handle camera device instance handle.
* @param[out] params pointer where the lens parameters should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_lens_parameters_v210 (royale_camera_handle handle, royale_lens_parameters *params);

/**
* see royale::ICameraDevice::isConnected()
* @param[in] handle camera device instance handle.
* @param[out] is_connected pointer where the resulting boolean should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_is_connected_v220 (royale_camera_handle handle, bool *is_connected);

/**
* see royale::ICameraDevice::isCalibrated()
* @param[in] handle camera device instance handle.
* @param[out] is_calibrated pointer where the resulting boolean should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_is_calibrated_v220 (royale_camera_handle handle, bool *is_calibrated);

/**
* see royale::ICameraDevice::isCapturing()
* @param[in] handle camera device instance handle.
* @param[out] is_capturing pointer where the resulting boolean should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_is_capturing_v220 (royale_camera_handle handle, bool *is_capturing);

/**
* see royale::ICameraDevice::getAccessLevel()
* @param[in] handle camera device instance handle.
* @param[out] access_level pointer where the access level should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_access_level_v220 (royale_camera_handle handle, royale_camera_access_level *access_level);

/**
* see royale::ICameraDevice::startRecording()
* @param[in] handle camera device instance handle.
* @param[in] filename target file for recording.
* @param[in] nr_of_frames number of frames to record. 0 (default) records until royale_camera_device_stop_recording_v210() is called.
* @param[in] frame_skip number of frames to skip after each recorded frame. 0 (default) is none.
* @param[in] ms_skip time in milliseconds to skip after each recorded frame. 0 (default) is none.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_start_recording_v210 (royale_camera_handle handle, const char *filename,
        uint32_t nr_of_frames, uint32_t frame_skip, uint32_t ms_skip);

/**
* see royale::ICameraDevice::stopRecording()
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_stop_recording_v210 (royale_camera_handle handle);

/**
* see royale::ICameraDevice::registerRecordListener()
* @param[in] handle camera device instance handle.
* @param[in] callback the callback to register.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_register_record_stop_listener_v210 (royale_camera_handle handle, ROYALE_RECORD_STOP_CALLBACK callback);

/**
* see royale::ICameraDevice::registerExposureListener(original IExposureListener)
* [deprecated]
* @param[in] handle camera device instance handle.
* @param[in] callback the callback to register.
*/
ROYALE_CAPI_DEPRECATED royale_camera_status royale_camera_device_register_exposure_listener_v210 (royale_camera_handle handle, ROYALE_EXPOSURE_CALLBACK_v210 callback);

/**
* see royale::ICameraDevice::registerExposureListener (IExposureListener2)
* @warning The callback type has changed from the v2.3.0 version
*
* @param[in] handle camera device instance handle.
* @param[in] callback the callback to register.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_register_exposure_listener_stream_v300 (royale_camera_handle handle, ROYALE_EXPOSURE_CALLBACK_v300 callback);

/**
* see royale::ICameraDevice::registerDataListener()
*
* !PLEASE NOTE: data returned through the callback is only available within the callback method's scope. Deep copy the data if you need it afterwards.
* @param[in] handle camera device instance handle.
* @param[in] callback the callback to register.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_register_data_listener_v210 (royale_camera_handle handle, ROYALE_DEPTH_DATA_CALLBACK callback);

/**
* see royale::ICameraDevice::registerDepthImageListener()
*
* !PLEASE NOTE: data returned through the callback is only available within the callback method's scope. Deep copy the data if you need it afterwards.
* @param[in] handle camera device instance handle.
* @param[in] callback the callback to register.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_register_depth_image_listener_v210 (royale_camera_handle handle, ROYALE_DEPTH_IMAGE_CALLBACK callback);

/**
* see royale::ICameraDevice::registerIRImageListener()
*
* !PLEASE NOTE: data returned through the callback is only available within the callback method's scope. Deep copy the data if you need it afterwards.
* @param[in] handle camera device instance handle.
* @param[in] callback the callback to register.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_register_ir_image_listener_v210 (royale_camera_handle handle, ROYALE_IR_IMAGE_CALLBACK callback);

/**
* see royale::ICameraDevice::registerSparsePointCloudListener()
*
* !PLEASE NOTE: data returned through the callback is only available within the callback method's scope. Deep copy the data if you need it afterwards.
* @param[in] handle camera device instance handle.
* @param[in] callback the callback to register.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_register_spc_listener_v210 (royale_camera_handle handle, ROYALE_SPC_DATA_CALLBACK callback);

/**
* see royale::ICameraDevice::registerEventListener()
*
* !PLEASE NOTE: data returned through the callback is only available within the callback method's scope. Deep copy the data if you need it afterwards.
* @param[in] handle camera device instance handle.
* @param[in] callback the callback to register.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_register_event_listener_v210 (royale_camera_handle handle, ROYALE_EVENT_CALLBACK callback);

/**
* see royale::ICameraDevice::unregisterRecordListener()
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_unregister_record_stop_listener_v210 (royale_camera_handle handle);

/**
* see royale::ICameraDevice::unregisterExposureListener()
* This _v210 unregister function works for both the _v210 and _v300 register functions.
*
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_unregister_exposure_listener_v210 (royale_camera_handle handle);

/**
* see royale::ICameraDevice::unregisterDataListener()
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_unregister_data_listener_v210 (royale_camera_handle handle);

/**
* see royale::ICameraDevice::unregisterDepthImageListener()
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_unregister_depth_image_listener_v210 (royale_camera_handle handle);

/**
* see royale::ICameraDevice::unregisterIRImageListener()
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_unregister_ir_image_listener_v210 (royale_camera_handle handle);

/**
* see royale::ICameraDevice::unregisterSparsePointCloudListener()
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_unregister_spc_listener_v210 (royale_camera_handle handle);

/**
* see royale::ICameraDevice::unregisterEventListener()
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_unregister_event_listener_v210 (royale_camera_handle handle);

/**
* see royale::ICameraDevice::setFrameRate()
* @param[in] handle camera device instance handle.
* @param[in] frame_rate the desired frame rate.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_frame_rate_v210 (royale_camera_handle handle, uint16_t frame_rate);

/**
* see royale::ICameraDevice::getFrameRate()
* @param[in] handle camera device instance handle.
* @param[out] frame_rate pointer where the current frame rate should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_frame_rate_v220 (royale_camera_handle handle, uint16_t *frame_rate);

/**
* see royale::ICameraDevice::getMaxFrameRate()
* @param[in] handle camera device instance handle.
* @param[out] max_frame_rate pointer where the max frame rate should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_max_frame_rate_v220 (royale_camera_handle handle, uint16_t *max_frame_rate);

/**
* see royale::ICameraDevice::getStreams()
* in case of ROYALE_STATUS_SUCCESS use ::royale_free_vector_stream_id() to free memory of the returned array.
* @param[in] handle camera device instance handle.
* @param[out] streams pointer where the resulting vector should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_streams_v300 (royale_camera_handle handle, royale_vector_stream_id *streams);

/**
* see royale::ICameraDevice::getNumberOfStreams()
* @param[in] handle camera device instance handle.
* @param[in] use_case_name use case name.
* @param[out] nr_streams number of streams for the specified use case.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_number_of_streams_v330 (royale_camera_handle handle, const char *use_case_name, uint32_t *nr_streams);

/**
* see royale::ICameraDevice::setExternalTrigger()
* @param[in] handle camera device instance handle.
* @param[in] use_external_trigger set to true to synchronize with another camera
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_external_trigger_v330 (royale_camera_handle handle, bool use_external_trigger);

// ----------------------------------------------------------------------------------------------
// Level 2: Experienced users (Laser Class 1 guaranteed) - activation key required
// ----------------------------------------------------------------------------------------------

/**
* [deprecated]
* see royale::ICameraDevice::setProcessingParameters()
* @param[in] handle camera device instance handle.
* @param[in] parameters array of processing parameters to set.
* @param[in] nr_params number of array entries.
*/
ROYALE_CAPI_DEPRECATED royale_camera_status royale_camera_device_set_processing_parameters_v210 (royale_camera_handle handle, royale_processing_parameter **parameters, uint32_t nr_params);

/**
* see royale::ICameraDevice::setProcessingParameters()
* @param[in] handle camera device instance handle.
* @param[in] stream_id stream id (may be 0 if there is only one stream).
* @param[in] parameters array of processing parameters to set.
* @param[in] nr_params number of array entries.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_processing_parameters_v300 (royale_camera_handle handle, royale_stream_id stream_id, royale_processing_parameter **parameters, uint32_t nr_params);

/**
* [deprecated]
* see royale::ICameraDevice::getProcessingParameters()
* @param[in] handle camera device instance handle.
* @param[out] parameters pointer where the resulting processing parameter array should be written to.
* @param[out] nr_params pointer where the number of entries in the resulting array should be written to.
*/
ROYALE_CAPI_DEPRECATED royale_camera_status royale_camera_device_get_processing_parameters_v210 (royale_camera_handle handle, royale_processing_parameter **parameters, uint32_t *nr_params);

/**
* see royale::ICameraDevice::getProcessingParameters()
* @param[in] handle camera device instance handle.
* @param[in] stream_id stream id (may be 0 if there is only one stream).
* @param[out] parameters pointer where the resulting processing parameter array should be written to.
* @param[out] nr_params pointer where the number of entries in the resulting array should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_processing_parameters_v300 (royale_camera_handle handle, royale_stream_id stream_id, royale_processing_parameter **parameters, uint32_t *nr_params);

/**
* [deprecated]
* see royale::ICameraDevice::setExposureTimes()
* @param[in] handle camera device instance handle.
* @param[in] exposure_times array of exposure times to set.
* @param[in] nr_exposure_times number of array entries.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_exposure_times_v210 (royale_camera_handle handle, uint32_t *exposure_times, uint32_t nr_exposure_times);

/**
* see royale::ICameraDevice::setExposureTimes()
* @param[in] handle camera device instance handle.
* @param[in] stream_id stream id (may be 0 if there is only one stream).
* @param[in] exposure_times array of exposure times to set.
* @param[in] nr_exposure_times number of array entries.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_exposure_times_v300 (royale_camera_handle handle, royale_stream_id stream_id, uint32_t *exposure_times, uint32_t nr_exposure_times);

/**
* see royale::ICameraDevice::setExposureForGroups()
* @param[in] handle camera device instance handle.
* @param[in] exposure_times array of exposure times to set.
* @param[in] nr_exposure_times number of array entries.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_exposure_for_groups_v300 (royale_camera_handle handle, uint32_t *exposure_times, uint32_t nr_exposure_times);

/**
* see royale::ICameraDevice::getExposureGroups()
* @param[in] handle camera device instance handle.
* @param[out] exposure_groups pointer where the resulting C String array should be written to.
* @param[out] nr_exposure_groups pointer where the number of entries in the resulting array should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_exposure_groups_v300 (royale_camera_handle handle, char ***exposure_groups, uint32_t *nr_exposure_groups);

/**
* see royale::ICameraDevice::setExposureTime()
* @param[in] handle camera device instance handle.
* @param[in] exposure_group exposure group for which the exposure time should be set.
* @param[in] exposure_time new exposure time for the group.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_group_exposure_time_v300 (royale_camera_handle handle, const char *exposure_group, uint32_t exposure_time);

/**
* see royale::ICameraDevice::getExposureLimits()
* @param[in] handle camera device instance handle.
* @param[in] exposure_group exposure group for which to query.
* @param[out] lower_limit pointer where the lower exposure limit should be written to.
* @param[out] upper_limit pointer where the upper exposure limit should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_group_exposure_limits_v300 (royale_camera_handle handle, const char *exposure_group, uint32_t *lower_limit, uint32_t *upper_limit);

/**
* see royale::ICameraDevice::registerDataListenerExtended()
*
* !PLEASE NOTE: data returned through the callback is only available within the callback method's scope. Deep copy the data if you need it afterwards.
* @param[in] handle camera device instance handle.
* @param[in] callback the callback to register.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_register_extended_data_listener_v210 (royale_camera_handle handle, ROYALE_EXTENDED_DATA_CALLBACK callback);

/**
* see royale::ICameraDevice::unregisterDataListenerExtended()
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_unregister_extended_data_listener_v210 (royale_camera_handle handle);

/**
* see royale::ICameraDevice::setCallbackData(royale::CallbackData)
* @param[in] handle camera device instance handle.
* @param[in] cb_data the desired callback data.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_callback_data_v210 (royale_camera_handle handle, royale_callback_data cb_data);

/**
* see royale::ICameraDevice::setCallbackData(uint16_t)
* @param[in] handle camera device instance handle.
* @param[in] cb_data the desired callback data in uint16_t format.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_callback_dataU16_v210 (royale_camera_handle handle, uint16_t cb_data);

/**
* see royale::ICameraDevice::setCalibrationData()
* @param[in] handle camera device instance handle.
* @param[in] calibration_data pointer to the calibration data array.
* @param[in] nr_of_data_entries number of entries in the array.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_calibration_data_v210 (royale_camera_handle handle, uint8_t **calibration_data, uint32_t nr_of_data_entries);

/**
* see royale::ICameraDevice::getCalibrationData()
* @param[in] handle camera device instance handle.
* @param[out] calibration_data pointer where the calibration data array should be written to.
* @param[out] nr_of_data_entries pointer where the number of entries in the resulting should be written to.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_calibration_data_v210 (royale_camera_handle handle, uint8_t **calibration_data, uint32_t *nr_of_data_entries);

/**
* see royale::ICameraDevice::writeCalibrationToFlash()
* @param[in] handle camera device instance handle.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_write_calibration_to_flash_v210 (royale_camera_handle handle);


// ----------------------------------------------------------------------------------------------
// Level 3: Advanced users (Laser Class 1 not (!) guaranteed) - activation key required
// ----------------------------------------------------------------------------------------------

/**
* see royale::ICameraDevice::writeDataToFlash()
* @param[in] handle camera device instance handle.
* @param[in] flash_data pointer to the data array that should be written.
* @param[in] nr_of_data_entries number of entries in the array.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_write_data_to_flash_v31000 (royale_camera_handle handle, uint8_t **flash_data, uint32_t nr_of_data_entries);

/**
* see royale::ICameraDevice::writeDataToFlash()
* @param[in] handle camera device instance handle.
* @param[in] file_name name of the file that should be used.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_write_data_to_flash_file_v31000 (royale_camera_handle handle, char *file_name);

/**
* see royale::ICameraDevice::setDutyCycle()
* @param[in] handle camera device instance handle.
* @param[in] duty_cycle the desired duty cycle.
* @param[in] index index of the sequence to change.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_set_duty_cycle_v210 (royale_camera_handle handle, double duty_cycle, uint16_t index);

/**
* see royale::ICameraDevice::writeRegisters()
* @param[in] handle camera device instance handle.
* @param[in] registers pointer to the array of registers to write.
* @param[in] nr_registers number of registers in the array.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_write_registers_v210 (royale_camera_handle handle, royale_pair_string_uint64 **registers, uint32_t nr_registers);

/**
* see royale::ICameraDevice::readRegisters()
* @param[in] handle camera device instance handle.
* @param[in,out] registers pointer to the array of registers to read, their values will be replaced after read by this function.
* @param[in] nr_registers number of registers in the array.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_read_registers_v210 (royale_camera_handle handle, royale_pair_string_uint64 **registers, uint32_t nr_registers);

/**
* see royale::ICameraDevice::shiftLensCenter()
* @param[in] handle camera device instance handle.
* @param[in] tx translation in x direction.
* @param[in] ty translation in y direction.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_shift_lens_center_v320 (royale_camera_handle handle, int16_t tx, int16_t ty);

/**
* see royale::ICameraDevice::getLensCenter()
* @param[in] handle camera device instance handle.
* @param[out] x current x center.
* @param[out] y current y center.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_get_lens_center_v320 (royale_camera_handle handle, uint16_t *x, uint16_t *y);


// ----------------------------------------------------------------------------------------------
// Level 4: Direct imager access (Laser Class 1 not (!) guaranteed) - activation key required
// ----------------------------------------------------------------------------------------------

/**
* see royale::ICameraDevice::initialize(const royale::String &)
* @param[in] handle camera device instance handle.
* @param[in] init_use_case the desired initial use case name.
*/
ROYALE_CAPI royale_camera_status royale_camera_device_initialize_with_use_case_v210 (royale_camera_handle handle, const char *init_use_case);


ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/