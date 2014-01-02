///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     
///

#ifndef __PREPROCESSING_H__
#define __PREPROCESSING_H__

#include <swcFrameTypes.h>
#include "../include/ObjectDetection.h"

extern "C" void Preprocessing(frameBuffer* const sensorImg, ObjectDetectionConfig* const cfg);

#endif