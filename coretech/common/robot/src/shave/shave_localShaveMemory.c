/**
File: shave_localShaveMemory.c
Author: Peter Barnum
Created: 2013

Declaration of local memory buffers for each shave. When this file is compiled, it will add the prefix shave#_ to buffer. To use these buffers, include the header shaveKernels_c.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/shaveKernels_c.h"

char localBuffer[LOCAL_SHAVE_BUFFER_SIZE];