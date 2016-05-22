/**
File: embeddedTests.h
Author: Peter Barnum
Created: 2014-02-14

Shared info for embedded unit tests

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_TESTS_H_
#define _ANKICORETECHEMBEDDED_TESTS_H_

#ifndef COZMO_ROBOT
#define COZMO_ROBOT
#endif

#include "anki/common/robot/config.h"

#if ANKICORETECH_EMBEDDED_USE_MATLAB
#include "anki/common/robot/matlabInterface.h"
extern Anki::Embedded::Matlab matlab;
#endif

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#endif

#if ANKICORETECH_EMBEDDED_USE_GTEST
#include "gtest/gtest.h"
#endif

#define OFFCHIP_BUFFER_SIZE 2000000
#define ONCHIP_BUFFER_SIZE 170000 // The max here is somewhere between 175000 and 180000 bytes
#define CCM_BUFFER_SIZE 50000 // The max here is probably 65536 (0x10000) bytes

extern char offchipBuffer[OFFCHIP_BUFFER_SIZE];
extern char onchipBuffer[ONCHIP_BUFFER_SIZE];
extern char ccmBuffer[CCM_BUFFER_SIZE];

#endif // _ANKICORETECHEMBEDDED_TESTS_H_
