/**
File: embeddedTests.cpp
Author: Peter Barnum
Created: 2014-02-14

Shared info for embedded unit tests

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "embeddedTests.h"

#if defined(__EDG__)  // ARM-MDK
#define LARGE_BUFFER_LOCATION __attribute__((section(".ram1"), zero_init))
//#define SMALL_BUFFER_LOCATION __attribute__((section(".ARM.__at_0x20004100"), zero_init))
#define SMALL_BUFFER_LOCATION __attribute__((section(".ram1"), zero_init))
#else
#define LARGE_BUFFER_LOCATION
#define SMALL_BUFFER_LOCATION
#endif

LARGE_BUFFER_LOCATION char largeBuffer[LARGE_BUFFER_SIZE];
SMALL_BUFFER_LOCATION char smallBuffer[SMALL_BUFFER_SIZE];
