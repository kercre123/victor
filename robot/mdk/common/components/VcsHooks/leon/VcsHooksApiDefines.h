///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by the vcsHooks component
/// 
#ifndef _VCS_HOOKS_DEF_H
#define _VCS_HOOKS_DEF_H 

#include "mv_types.h"
#include "registersMyriad.h"

// 1: Defines
// ----------------------------------------------------------------------------
#define PLAT_SILICON                    (0)
#define PLAT_VCS_SIM                    (1)
#define PLAT_MOVI_SIM                   (2)
#define PLAT_FPGA                       (3)

#define PLATFORM_DETECT_REG_ADR         (CPR_GEN_CTRL_ADR)
#define GET_CURRENT_PLATFORM(null)      ((GET_REG_WORD_VAL(PLATFORM_DETECT_REG_ADR)>>29)&0x7)
// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // _VCS_HOOKS_DEF_H

