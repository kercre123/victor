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
#include <isaac_registers.h>
#include "brdMv0172DCard.h"
#include "brdMv0153.h"
#include "brdGpioCfgs/brdMv0172GpioDefaults.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

tyDcVoltageCfg mv0172DCVoltageCfg =
{
    .voltCfgU15 =
    {                                 // See comments on definition of tyLT3906Config for description of voltage config
        .switchedVoltage1  =  125,    // This rail is not actually used but set to 1.250V anyway
        .switchedVoltage2  =  33,     // 3.300V -> MV0172 Rail 3.3VS  
        .ldoVoltage1       =  11,     // This rail is not actually used but set to 1.100V anyway 
        .ldoVoltage2       =  17,     // This rail is not actually used but set to 1.700V anyway  
        {0,0,0,0,0}                   // Voltage register cache initilised to zero
    },
    .voltCfgU17 =
    {
        .switchedVoltage1  =  180,    // 1.800V -> MV0172 Rail 1.8VS  
        .switchedVoltage2  =  25,     // This rail is not actually used but set to 2.500V anyway  
        .ldoVoltage1       =  28,     // 2.800V -> MV0172 Rail 2.8Vin   
        .ldoVoltage2       =  180,     // This rail is not actually used but set to 2.500V anyway  
        {0,0,0,0,0}                   // Voltage register cache initilised to zero
    },
};

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

int brdMv0172DcStartupVoltages(void)
{
    int camVoltStartupFail;
    camVoltStartupFail = brd153CfgDaughterCardVoltages(&mv0172DCVoltageCfg);
    assert(camVoltStartupFail == 0);
    return camVoltStartupFail;
}


int brdMv0153ForMv0172Initialise(void)
{

    DrvGpioInitialiseRange(brdMv0172GpioCfgDefault);

    return 0;
}



