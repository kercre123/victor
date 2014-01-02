///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     System Clock configuration for default operation at 180Mhz
///         
/// Configures System clock for 180Mhz and DDR clock for 90Mhz
/// Enables the primary core system clocks and all busses
///

#ifndef SYS_CLK_180_90_H
#define SYS_CLK_180_90_H 

#include "DrvCpr.h"

tyAuxClkDividerCfg auxClkCfg180_180[] =
{
    {
        .auxClockEnableMask     = AUX_CLK_MASK_DDR,            // Ensure DDR always has a clock 
        .auxClockDividerValue   = GEN_CLK_DIVIDER(1,2),     // Run DDR at 90Mhz
    },
    {
        .auxClockEnableMask     = AUX_CLK_MASK_SAHB,           // Clock Slow AHB Bus by default as needed for SDHOST,SDIO,USB,NAND 
        .auxClockDividerValue   = GEN_CLK_DIVIDER(1,2),     // Slow AHB must run @ less than 100Mhz so we divide by 2 to give 90
    },
    {0,0}, // Null Terminated List
};

tySocClockConfig appClockConfig180_180 =
{
    .targetOscInputClock        = 12000,                // Default to 12Mhz input Osc
    .targetPllFreqKhz           = 180000,
    .systemClockConfig          = 
        {
        .sysClockEnableMask     = DEFAULT_CORE_BUS_CLOCKS |
          DEV_SVU0 				  |
		  DEV_SVU1 				  |
		  DEV_SVU2 				  |
		  DEV_SVU3 				  |
		  DEV_SVU4 				  |
		  DEV_SVU5 				  |
		  DEV_SVU6 				  |
		  DEV_SVU7 				  ,
        .sysClockDividerValue   = GEN_CLK_DIVIDER(1,1),
        },                      
    .pAuxClkCfg                 = auxClkCfg180_180,
};
         

#endif // SYS_CLK_180_90_H

