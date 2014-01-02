///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for MV0155 Daughtercard when run from MV0155
/// 
/// @note This is distinct from the driver for MV0155 which runs on MV0155 processor
/// 
/// 
/// 
#include "brdMv0163DCardDefines.h"


#ifndef BRD_MV0163_DCARD_H
#define BRD_MV0163_DCARD_H 

// 1: Includes
// ----------------------------------------------------------------------------

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
int brdMv0163DcStartupVoltages(void);
         
#endif // BRD_MV0163_DCARD_H  
