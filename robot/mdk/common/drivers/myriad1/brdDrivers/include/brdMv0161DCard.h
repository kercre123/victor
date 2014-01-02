///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for MV0161 Daughtercard when run from MV0153
/// 
/// @note This is distinct from the driver for MV0161 which runs on MV0161 processor
/// 
/// 
/// 
#include "brdMv0161DCardDefines.h"


#ifndef BRD_MV0161_DCARD_H
#define BRD_MV0161_DCARD_H 

// 1: Includes
// ----------------------------------------------------------------------------

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
int brdMv0161DcStartupVoltages(void);
         
#endif // BRD_MV0161_DCARD_H  
