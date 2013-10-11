///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Application configuration Leon header
///

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "mv_types.h"

extern u32 __cmx_config;   //notify other tools of the cmx configuration
extern u32 __l2_config ;

/// Setup all the clock configurations needed by this application and also the ddr
///
/// @return    0 on success, non-zero otherwise  
int initClocksAndMemory(void);

#endif



