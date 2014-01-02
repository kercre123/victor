///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @defgroup CifOV5642Api Flip and mirroring sensor config API
/// @{
/// @brief     Flip and mirroring config of the sensors.
///
///
#ifndef CIFOV5642API_H_
#define CIFOV5642API_H_

#include "CIFGenericApi.h"
/// Configure the flip or mirroring of the sensors
// (See OV5_720p.h content for the set of commands sent to the sensor)
/// @param  hndl               Pointer to the sensor handle
/// @param  camSpec            Camera configuration
/// @param  sensorFlipMirror   0 for old cameras (flip image) and 1 for new cameras (mirror image)
void CifOV5642FlipMirror(SensorHandle* hndl, CamSpec* camSpec, int sensorFlipMirror);
/// @}
#endif /* CIFOV5642_H_ */
