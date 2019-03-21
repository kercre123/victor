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

/* New functions in the latest API release (currently 300) that weren't in previous versions */
#if ROYALE_C_API_VERSION == 220
#define royale_camera_device_register_exposure_listener_stream  royale_camera_device_register_exposure_listener_stream_v300
#endif

/*
 * Intentional breakages.  Mismatched callbacks are likely to cause compiler warnings rather than
 * errors, but they can also cause hard-to-debug memory corruption, so the names have been changed
 * to make it more likely that a mismatch causes a compiler error
 */

#ifdef royale_camera_device_register_exposure_listener
#undef royale_camera_device_register_exposure_listener
#endif
#define royale_camera_device_register_exposure_listener register_exposure_listener is deprecated and replaced by register_exposure_listener_stream with a changed callback type

#ifdef ROYALE_EXPOSURE_CALLBACK
#undef ROYALE_EXPOSURE_CALLBACK
#endif
#define ROYALE_EXPOSURE_CALLBACK register_exposure_listener has changed its callback type to ROYALE_EXPOSURE_STREAM_CALLBACK


/* C API functions of the latest API release (currently 300), with "_new_api" suffix added. */
/* Intended to be used during migration to the latest API */

#define royale_camera_device_destroy_new_api                           royale_camera_device_destroy_v210
#define royale_camera_device_initialize_new_api                        royale_camera_device_initialize_v210
#define royale_camera_device_get_id_new_api                            royale_camera_device_get_id_v220
#define royale_camera_device_get_camera_name_new_api                   royale_camera_device_get_camera_name_v220
#define royale_camera_device_get_camera_info_new_api                   royale_camera_device_get_camera_info_v220
#define royale_camera_device_set_use_case_new_api                      royale_camera_device_set_use_case_v210
#define royale_camera_device_get_use_cases_new_api                     royale_camera_device_get_use_cases_v220
#define royale_camera_device_get_current_use_case_new_api              royale_camera_device_get_current_use_case_v220
#define royale_camera_device_set_exposure_time_new_api                 royale_camera_device_set_exposure_time_v300
#define royale_camera_device_set_exposure_mode_new_api                 royale_camera_device_set_exposure_mode_v300
#define royale_camera_device_get_exposure_mode_new_api                 royale_camera_device_get_exposure_mode_v300
#define royale_camera_device_get_exposure_limits_new_api               royale_camera_device_get_exposure_limits_v300
#define royale_camera_device_start_capture_new_api                     royale_camera_device_start_capture_v210
#define royale_camera_device_stop_capture_new_api                      royale_camera_device_stop_capture_v210
#define royale_camera_device_get_max_sensor_width_new_api              royale_camera_device_get_max_sensor_width_v220
#define royale_camera_device_get_max_sensor_height_new_api             royale_camera_device_get_max_sensor_height_v220
#define royale_camera_device_get_lens_parameters_new_api               royale_camera_device_get_lens_parameters_v210
#define royale_camera_device_is_connected_new_api                      royale_camera_device_is_connected_v220
#define royale_camera_device_is_calibrated_new_api                     royale_camera_device_is_calibrated_v220
#define royale_camera_device_is_capturing_new_api                      royale_camera_device_is_capturing_v220
#define royale_camera_device_get_access_level_new_api                  royale_camera_device_get_access_level_v220
#define royale_camera_device_start_recording_new_api                   royale_camera_device_start_recording_v210
#define royale_camera_device_stop_recording_new_api                    royale_camera_device_stop_recording_v210
#define royale_camera_device_register_record_stop_listener_new_api     royale_camera_device_register_record_stop_listener_v210
#define royale_camera_device_register_exposure_listener_stream_new_api royale_camera_device_register_exposure_listener_stream_v300
#define royale_camera_device_register_data_listener_new_api            royale_camera_device_register_data_listener_v210
#define royale_camera_device_register_depth_image_listener_new_api     royale_camera_device_register_depth_image_listener_v210
#define royale_camera_device_register_ir_image_listener_new_api        royale_camera_device_register_ir_image_listener_v210
#define royale_camera_device_register_spc_listener_new_api             royale_camera_device_register_spc_listener_v210
#define royale_camera_device_register_event_listener_new_api           royale_camera_device_register_event_listener_v210
#define royale_camera_device_unregister_record_stop_listener_new_api   royale_camera_device_unregister_record_stop_listener_v210
#define royale_camera_device_unregister_exposure_listener_new_api      royale_camera_device_unregister_exposure_listener_v210
#define royale_camera_device_unregister_data_listener_new_api          royale_camera_device_unregister_data_listener_v210
#define royale_camera_device_unregister_depth_image_listener_new_api   royale_camera_device_unregister_depth_image_listener_v210
#define royale_camera_device_unregister_ir_image_listener_new_api      royale_camera_device_unregister_ir_image_listener_v210
#define royale_camera_device_unregister_spc_listener_new_api           royale_camera_device_unregister_spc_listener_v210
#define royale_camera_device_unregister_event_listener_new_api         royale_camera_device_unregister_event_listener_v210
#define royale_camera_device_set_frame_rate_new_api                    royale_camera_device_set_frame_rate_v210
#define royale_camera_device_get_frame_rate_new_api                    royale_camera_device_get_frame_rate_v220
#define royale_camera_device_get_max_frame_rate_new_api                royale_camera_device_get_max_frame_rate_v220
#define royale_camera_device_get_streams_new_api                       royale_camera_device_get_streams_v300
#define royale_camera_device_set_processing_parameters_new_api         royale_camera_device_set_processing_parameters_v300
#define royale_camera_device_get_processing_parameters_new_api         royale_camera_device_get_processing_parameters_v300
#define royale_camera_device_set_exposure_times_new_api                royale_camera_device_set_exposure_times_v300
#define royale_camera_device_set_exposure_for_groups_new_api           royale_camera_device_set_exposure_for_groups_v300
#define royale_camera_device_get_exposure_groups_new_api               royale_camera_device_get_exposure_groups_v300
#define royale_camera_device_set_group_exposure_time_new_api           royale_camera_device_set_group_exposure_time_v300
#define royale_camera_device_get_group_exposure_limits_new_api         royale_camera_device_get_group_exposure_limits_v300
#define royale_camera_device_register_extended_data_listener_new_api   royale_camera_device_register_extended_data_listener_v210
#define royale_camera_device_unregister_extended_data_listener_new_api royale_camera_device_unregister_extended_data_listener_v210
#define royale_camera_device_set_callback_data_new_api                 royale_camera_device_set_callback_data_v210
#define royale_camera_device_set_callback_dataU16_new_api              royale_camera_device_set_callback_dataU16_v210
#define royale_camera_device_set_calibration_data_new_api              royale_camera_device_set_calibration_data_v210
#define royale_camera_device_get_calibration_data_new_api              royale_camera_device_get_calibration_data_v210
#define royale_camera_device_write_calibration_to_flash_new_api        royale_camera_device_write_calibration_to_flash_v210
#define royale_camera_device_set_duty_cycle_new_api                    royale_camera_device_set_duty_cycle_v210
#define royale_camera_device_write_registers_new_api                   royale_camera_device_write_registers_v210
#define royale_camera_device_read_registers_new_api                    royale_camera_device_read_registers_v210
#define royale_camera_device_initialize_with_use_case_new_api          royale_camera_device_initialize_with_use_case_v210

/*
 * Defines for structures / typedefs for the latest API release (currently 300)
 */

#ifndef ROYALE_EXPOSURE_STREAM_CALLBACK
#define ROYALE_EXPOSURE_STREAM_CALLBACK ROYALE_EXPOSURE_CALLBACK_v300
#endif

/** @}*/
