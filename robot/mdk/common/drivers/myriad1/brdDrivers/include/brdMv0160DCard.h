///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for MV0160 Daughtercard when run from MV0153
/// 
/// @note This is distinct from the driver for MV0160 which runs on MV0160 processor
/// 
/// 
/// 

//#include "brdMv0160DCardDefines.h"


#ifndef BRD_MV0160_DCARD_H
#define BRD_MV0160_DCARD_H

// 1: Includes
// ----------------------------------------------------------------------------

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
int brdMv0160DcStartupVoltages(void);
         
#endif // BRD_MV0160-_DCARD_H
