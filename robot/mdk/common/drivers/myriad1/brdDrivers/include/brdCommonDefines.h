///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by the more than one Board Driver
/// 
/// 
/// 
/// 

#ifndef BRD_COMMON_DEF_H
#define BRD_COMMON_DEF_H 

#include "DrvGpioDefines.h"
#include "DrvI2cMasterDefines.h"
#include "icRegulatorLT3906.h"
#include "icPllCDCE913.h"

// 1: Defines
#define ACTIVE_LOW               (0)
#define ACTIVE_HIGH              (1)


// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
typedef enum
{
    RST_NORMAL,
    RST_AND_HOLD
}tyResetAction;

typedef struct
{
    u32 deviceFlag;
    u8  gpioPin;
    u8  activeLevel;
    u32 activeMs;
    u32 holdMs;
} tyResetPinCfg;

typedef tyResetPinCfg *tyResetTable;

typedef struct
{
    tyResetTable    mbRstTable;
    tyResetTable    daughterCard1RstTable;
    tyResetTable    daughterCard2RstTable;
    tyResetTable    daughterCard3RstTable;
    tyResetTable    daughterCard4RstTable;
} tyBoardResetConfiguration;

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // BRD_COMMON_DEF_H

