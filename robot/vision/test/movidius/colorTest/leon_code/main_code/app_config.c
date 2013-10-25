///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Key application system settings
/// 
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include "app_config.h"
#include "DrvCpr.h"
#include "DrvDdr.h"
#include "assert.h"
#include <isaac_registers.h>
#include <DrvL2Cache.h>
             
// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#define CMX_CONFIG      (0x66666666)
#define L2CACHE_CFG     (L2CACHE_NORMAL_MODE)
                         
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// Sections decoration is require here for downstream tools
#ifndef __APPLE__
u32 __cmx_config  __attribute__((section(".cmx.ctrl"))) = CMX_CONFIG;  
u32 __l2_config   __attribute__((section(".l2.mode")))  = L2CACHE_CFG;
#endif //#ifndef __APPLE__

// 4: Static Local Data 
// ----------------------------------------------------------------------------

static const tyAuxClkDividerCfg simpleCamHdmiAuxClkCfg[] =
{
    {
        .auxClockEnableMask     =(AUX_CLK_MASK_DDR    |         // Ensure DDR always has a clock 
                                  AUX_CLK_MASK_LCD1   |
                                  AUX_CLK_MASK_LCD1X2 |
                                  AUX_CLK_MASK_IO),  
        .auxClockDividerValue   = GEN_CLK_DIVIDER(1,1),     // Run DDR at system frequency also (Bypass divider)
    },
    {
        .auxClockEnableMask     = AUX_CLK_MASK_SAHB,           // Clock Slow AHB Bus by default as needed for SDHOST,SDIO,USB,NAND 
        .auxClockDividerValue   = GEN_CLK_DIVIDER(1,2),     // Slow AHB must run @ less than 100Mhz so we divide by 2 to give 90
    },
    {0,0}, // Null Terminated List
};

static const tySocClockConfig simpleCamHdmiClkCfg =
{
    .targetOscInputClock        = 12000,                // Default to 12Mhz input Osc
    .targetPllFreqKhz           = 180000,
    .systemClockConfig          = 
        {
        .sysClockEnableMask     = DEFAULT_CORE_BUS_CLOCKS,
        .sysClockDividerValue   = GEN_CLK_DIVIDER(1,1),
        },                      
    .pAuxClkCfg                 = simpleCamHdmiAuxClkCfg,
};
         

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

int initClocksAndMemory(void)
{
    int retVal;

    // Initialise the Clock Power reset module
    retVal = DrvCprInit(NULL,NULL);
    if (retVal)
        return retVal;

    retVal = DrvCprSetupClocks(&simpleCamHdmiClkCfg);
    if (retVal)
        return retVal;

    // Configure all CMX RAM layouts
    SET_REG_WORD(LHB_CMX_RAMLAYOUT_CFG, CMX_CONFIG);

    DrvDdrInitialise(DrvCprGetClockFreqKhz(AUX_CLK_DDR,NULL));

    return 0;
}
               







