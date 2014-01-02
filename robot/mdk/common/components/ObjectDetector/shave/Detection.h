///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     
///

#ifndef __DETECTOR_H__
#define __DETECTOR_H__

#include "../include/ObjectDetection.h"

extern "C" void Run(ObjectDetectionConfig* const cfg);

#endif