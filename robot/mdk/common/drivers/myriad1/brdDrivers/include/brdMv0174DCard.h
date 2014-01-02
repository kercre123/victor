///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for MV0174 Daughtercard when run from MV0153
/// 
/// 
/// 
/// 
#include "brdMv0174DCardDefines.h"


#ifndef BRD_MV0174_DCARD_H
#define BRD_MV0174_DCARD_H

// 1: Includes
// ----------------------------------------------------------------------------

#include "icMipiTC358746.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

extern TMipiBridge Mv174ToDragonboardBridge;

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
int brd174InitialiseGpios(void);

#endif // BRD_MV0172_DCARD_H  
