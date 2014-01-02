///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for use on MV0153 to configure MV0155 Daughtercard
/// 
/// 
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
#include "assert.h"
#include <isaac_registers.h>
#include "brdMv0163DCard.h"
#include "brdMv0153.h"


// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

tyDcVoltageCfg mv0163DCVoltageCfg =
{
    .voltCfgU15 =
    {                                 // See comments on definition of tyLT3906Config for description of voltage config
        .switchedVoltage1  =  125,    // 1.250V -> MV0155 Rail 1.2VS  [Myriad Core + Analog          ] TP13
        .switchedVoltage2  =  33,     // 3.300V -> MV0155 Rail 3.3VS  [Myriad 3.3; LEDS; SPI         ] TP11
        .ldoVoltage1       =  13,     // 1.300V -> MV0155 Rail LDOA   [Bridge Core Voltage + Analog  ] TP15
        .ldoVoltage2       =  17,     // 1.700V -> MV0155 Rail LDOB   [Bridge IO Voltage, Cam I/O    ] C91 
        {0,0,0,0,0}                   // Voltage register cache initilised to zero
    },
    .voltCfgU17 =
    {
        .switchedVoltage1  =  180,    // 1.800V -> MV0155 Rail 1.8VS  [Myriad IO/DDR; Mipi Bridge IO; Sensor IO] TP12
        .switchedVoltage2  =  25,     // 2.500V -> MV0155 Rail 2.5VS  [Myriad IO + Analog                      ] TP10
        .ldoVoltage1       =  28,     // 2.800V -> MV0155 Rail LDOC   [Cam Analog Core                         ] TP16
        .ldoVoltage2       =  25,     // 2.500V -> MV0155 Rail LDOD   [Not Used on MV0155                      ]
        {0,0,0,0,0}                   // Voltage register cache initilised to zero
    },
};

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

int brdMv0163DcStartupVoltages(void)
{
    int camVoltStartupFail;
    camVoltStartupFail = brd153CfgDaughterCardVoltages(&mv0163DCVoltageCfg);
    assert(camVoltStartupFail == 0);
    return camVoltStartupFail;
}

