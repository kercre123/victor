///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Application configuration Leon file
///


// 1: Includes
// ----------------------------------------------------------------------------
#include "app_config.h"
#include "DrvCpr.h"
#include <DrvDdr.h>
#include <DrvL2Cache.h>
#include <isaac_registers.h>
#include <DrvL2Cache.h>

#include "sysClkCfg/sysClk180_180.h"
             
// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#define CMX_CONFIG      (0x66666666)
#define L2CACHE_CFG     (L2CACHE_NORMAL_MODE)
                         
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// Sections decoration is require here for downstream tools
u32 __cmx_config  __attribute__((section(".cmx.ctrl"))) = CMX_CONFIG;  
u32 __l2_config   __attribute__((section(".l2.mode")))  = L2CACHE_CFG;

// 4: Static Local Data 
// ----------------------------------------------------------------------------

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

    retVal = DrvCprSetupClocks(&appClockConfig180_180);
    if (retVal)
        return retVal;

    // Configure all CMX RAM layouts
    SET_REG_WORD(LHB_CMX_RAMLAYOUT_CFG, CMX_CONFIG);

    DrvDdrInitialise(DrvCprGetClockFreqKhz(AUX_CLK_DDR,NULL));

    SET_REG_WORD( L2C_MODE_ADR, __l2_config );  // TODO:  Check if needed
    DrvL2CachePartitionInvalidate(0);

    return 0;
}






