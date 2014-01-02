///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for use on MV0153 to configure MV0172 Daughtercard
/// 
/// 
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
#include "assert.h"
#include "DrvTimer.h"
#include <isaac_registers.h>
#include "brdMv0177DCard.h"
#include "../../../../components/Board/leon/BoardApi.h"
#include "brdMv0153.h"
#include "brdGpioCfgs/brdMv0177GpioDefaults.h"
#include "icMipiTC358746.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

TMipiBridge mv177MipiBridge =
    {
            .i2cDev = NULL,
            .BridgeType = CSI2RXParOut,
            .BridgeI2CSlaveAddr = TC358746XBG_SLAVE_ADDRESS,

            .ChipSelectGPIO = PIN_NOT_CONNECTED_TO_GPIO,
            .ResetGPIO = GPIO_MIPI_RESX,
            .ModeSelectGPIO = PIN_NOT_CONNECTED_TO_GPIO,

    };


tyDcVoltageCfg mv0177DCVoltageCfg =
{
    .voltCfgU15 =
    {                                 // See comments on definition of tyLT3906Config for description of voltage config
        .switchedVoltage1  =  120,    // This rail is not actually used but set to 1.250V anyway
        .switchedVoltage2  =  33,     // 3.300V -> MV0172 Rail 3.3VS
        .ldoVoltage1       =  12,     // This rail is not actually used but set to 1.100V anyway
        .ldoVoltage2       =  18,     // This rail is not actually used but set to 1.700V anyway
        {0,0,0,0,0}                   // Voltage register cache initilised to zero
    },
    .voltCfgU17 =
    {
        .switchedVoltage1  =  180,    // 1.800V -> MV0172 Rail 1.8VS
        .switchedVoltage2  =  25,     // This rail is not actually used but set to 2.500V anyway
        .ldoVoltage1       =  28,     // 2.800V -> MV0172 Rail 2.8Vin
        .ldoVoltage2       =  180,     // This rail is not actually used
        {0,0,0,0,0}                   // Voltage register cache initilised to zero
    }
};

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------
int brdMv0153ForMv0177Initialise(void)
{

    DrvGpioInitialiseRange(brdMv0177GpioCfgDefault);
    SleepMs(50);
    brdMv0177DcStartupVoltages();
    SleepMs(20);
    int res = icTC358746AssignI2c(&mv177MipiBridge, gAppDevHndls.i2c1Handle);
        if (res) return res;
//   brdMv177InitialiseMipiBridge(&mv177MipiBridge, gAppDevHndls.i2c1Handle);
    icTC358746Reset(&mv177MipiBridge);
    return 0;
}

int brdMv0177DcStartupVoltages(void)
{
    int camVoltStartupFail;
    camVoltStartupFail = brd153CfgDaughterCardVoltages(&mv0177DCVoltageCfg);
    assert(camVoltStartupFail == 0);
    return camVoltStartupFail;
}

