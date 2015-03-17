/**
File: embeddedTests.cpp
Author: Peter Barnum
Created: 2014-02-14

Shared info for embedded unit tests

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "embeddedTests.h"

OFFCHIP char offchipBuffer[OFFCHIP_BUFFER_SIZE];
ONCHIP char onchipBuffer[ONCHIP_BUFFER_SIZE];
CCM char ccmBuffer[CCM_BUFFER_SIZE];
