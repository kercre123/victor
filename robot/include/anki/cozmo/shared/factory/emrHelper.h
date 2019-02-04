/**
* File: emrHelper.h
*
* Author: Al Chaussee
* Date:   1/30/2018
*
* Description: Electronic medical(manufacturing) record written to the robot at the factory
*              DO NOT MODIFY
*
* Copyright: Anki, Inc. 2018
**/

#ifdef SIMULATOR
#include "anki/cozmo/shared/factory/emrHelper_mac.h"
#else
#include "anki/cozmo/shared/factory/emrHelper_vicos.h"
#endif
