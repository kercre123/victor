///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup camMjpgApi Camera JPEG stream component
/// @{
/// @brief    Camera JPEG stream component
///
/// @details This is the API that contains function headers for JPEG streaming from the OV sensor
///

#ifndef _CAM_M_JPG_H_
#define _CAM_M_JPG_H_

// Includes
#include "app_types.h"

///A simple component testing function(dummy)
void JPG_component_test();

///Realize JPEG streaming from the OV sensor
///@param[in] sensor_in pointer to the sensor input buffer
///@param[out] jpeg_out JPEG output buffer
void process_jpeg(unsigned int sensor_in, unsigned int jpeg_out);
/// @}
#endif