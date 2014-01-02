///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for MV0172 Daughtercard when run from MV0153
/// 
/// 
/// 
/// 
#include "brdMv0172DCardDefines.h"


#ifndef BRD_MV0172_DCARD_H
#define BRD_MV0172_DCARD_H 

// 1: Includes
// ----------------------------------------------------------------------------

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
int brdMv0172DcStartupVoltages(void);
int brdMv0153ForMv0172Initialise(void);
         
#endif // BRD_MV0172_DCARD_H  
