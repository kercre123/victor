///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for use on MV0153 to configure MV0167 Daughtercard
/// 
// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
#include "assert.h"
#include <isaac_registers.h>
#include "brdMv0167DCard.h"
#include "brdMv0153.h"
#include "brdGpioCfgs/brdMv0167GpioDefaults.h"
#include <DrvSpi.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

int brdMv0153ForMv0167Initialise(void)
{
    u32 rsiSpiBaseAddr = SPI1_BASE_ADR;
    DrvGpioInitialiseRange(brdMv0167GpioCfgDefault);

    return 0;
}



