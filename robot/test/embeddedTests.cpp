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
#define OFFCHIP_BUFFER_LOCATION __attribute__((section(".ram1"), zero_init))
#define ONCHIP_BUFFER_LOCATION  __attribute__((section(".ARM.__at_0x20004100"), zero_init))
#define CCM_BUFFER_LOCATION     __attribute__((section(".ARM.__at_0x10000000"), zero_init))
#else
#define OFFCHIP_BUFFER_LOCATION
#define ONCHIP_BUFFER_LOCATION
#define CCM_BUFFER_LOCATION
#endif

OFFCHIP_BUFFER_LOCATION char offchipBuffer[OFFCHIP_BUFFER_SIZE];
ONCHIP_BUFFER_LOCATION char onchipBuffer[ONCHIP_BUFFER_SIZE];
CCM_BUFFER_LOCATION char ccmBuffer[CCM_BUFFER_SIZE];
