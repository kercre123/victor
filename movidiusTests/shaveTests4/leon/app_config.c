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
#include <registersMyriad.h>
#include <DrvCpr.h>

#ifdef MYRIAD1
    #include <DrvDdr.h>
    #include <DrvL2Cache.h>
    #include "sysClkCfg/sysClk180_180.h"
#else //MYRIAD2
    #include <DrvRegUtils.h>
    #include <DrvShaveL2Cache.h>
#endif

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#define CMX_CONFIG      (0x66666666)
#define L2CACHE_CFG     (SHAVE_L2CACHE_NORMAL_MODE)

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

#ifdef MYRIAD1

    // Initialise the Clock Power reset module
    retVal = DrvCprInit(NULL, NULL);
    if (retVal)
        return retVal;

    retVal = DrvCprSetupClocks(&appClockConfig180_180);
    if (retVal)
        return retVal;

    // Initialise DDR
    DrvDdrInitialise(DrvCprGetClockFreqKhz(AUX_CLK_DDR, NULL));

    // Invalidate L2 Cache
    DrvL2CachePartitionInvalidate(0);
    
#else //MYRIAD2

    int i;

    //For myriad2 just turn all clocks on for now
    SET_REG_WORD(CPR_CLK_EN0_ADR, 0xFFFFFFFF);
    SET_REG_WORD(CPR_CLK_EN1_ADR, 0xFFFFFFFF);
    SET_REG_WORD(CPR_AUX_CLK_EN_ADR, 0xFFFFFFFF);

    //Set Shave L2 cache partitions
    DrvShaveL2CacheSetupPartition(SHAVEPART256KB);

    //Allocate Shave L2 cache set partitions
    DrvShaveL2CacheAllocateSetPartitions();

    //Assign the one partition defined to all shaves
    for (i = 0; i < 12; i++)
    {
        DrvShaveL2CacheSetLSUPartId(i, 0);
    }

#endif

    return 0;
}

