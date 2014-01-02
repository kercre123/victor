///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for MV0177 Daughtercard when run from MV0153
/// 
/// 
/// 
/// 

#ifndef BRD_MV0177_DCARD_H
#define BRD_MV0177_DCARD_H

// 1: Includes
// ----------------------------------------------------------------------------
#include "brdMv0177DCardDefines.h"
#include "icMipiTC358746.h"
// 2:  Exported Global Data (generally better to avoid)
// -------------------------------------------------------------- --------------
extern TMipiBridge mv177MipiBridge;
// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
int brdMv0177DcStartupVoltages(void);
int brdMv0153ForMv0177Initialise(void);
#endif // BRD_MV0177_DCARD_H
