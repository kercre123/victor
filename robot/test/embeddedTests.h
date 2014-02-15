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

//#if defined(ROBOT_HARDWARE)
#include "anki/cozmo/robot/messages.h"
//#else
//#include "anki/cozmo/basestation/messages.h"
//#endif

#define LARGE_BUFFER_SIZE 900000
#define SMALL_BUFFER_SIZE 190000

extern char largeBuffer[LARGE_BUFFER_SIZE];
extern char smallBuffer[SMALL_BUFFER_SIZE];

#endif // _ANKICORETECHEMBEDDED_TESTS_H_
