///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by Application Board Interface Module
/// 
#ifndef _BOARD_DEF_H
#define _BOARD_DEF_H 

#include "mv_types.h"
#include "DrvI2cMasterDefines.h"
// 1: Defines
// ----------------------------------------------------------------------------

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

typedef struct 
{
    I2CM_Device * i2c1Handle;
    I2CM_Device * i2c2Handle;
} tyAppDeviceHandles;
// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // BOARD_DEF_H

