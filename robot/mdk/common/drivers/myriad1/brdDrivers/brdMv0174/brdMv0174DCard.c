///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for use on MV0153 to configure MV0174 Daughtercard
/// 
/// 
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
#include "assert.h"
#include <isaac_registers.h>
#include "brdMv0174DCard.h"
#include "brdMv0153.h"
#include "brdGpioCfgs/brdMv0174GpioDefaults.h"
#include "icMipiTC358746.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

TMipiBridge Mv174ToDragonboardBridge =
{
    .i2cDev = NULL,
    .BridgeType = CSI2TXParIn,
    .BridgeI2CSlaveAddr = TC358746XBG_SLAVE_ADDRESS,

    .ChipSelectGPIO = GPIO_MV0174_MIPI_CS,
    .ResetGPIO = GPIO_MV0174_MIPI_RESX,
    .ModeSelectGPIO =  GPIO_MV0174_MIPI_MSEL,
};

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------


int brd174InitialiseGpios(void)
{

    DrvGpioInitialiseRange(brdMv0174GpioCfgDefault);

    return 0;
}
