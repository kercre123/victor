///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver to control the startup of LeonRT processor
/// 
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include <DrvCpr.h>
#include <registersMyriad2.h>
#include <assert.h>
#include <DrvRegUtils.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#define LEON_STARTUP_ALIGNMENT_MASK   ( 0xFFF)  // bottom 12 bits of target address must be 0 for 4KB alignment
#define LEON_IRQI_RUN                 (1 << 0) 
#define LEON_IRQI_RESET               (1 << 1)

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

void DrvLeonRTStartup(u32 leonStartupAddress)
{
    assert((leonStartupAddress & LEON_STARTUP_ALIGNMENT_MASK) == 0);

    leonStartupAddress &= ~LEON_STARTUP_ALIGNMENT_MASK; // Force alignment  

    // Setup the new target address and the RUN bit to request start of execution
    SET_REG_WORD(CPR_LEON_RT_VECTOR_ADR,((u32)leonStartupAddress) | LEON_IRQI_RUN ); 

    // Then toggle the system reset to the LEON_RT to get it to latch the new address
    // TODO: The following lines chould be replaced with the approriate DrvCpr function when available
    SET_REG_WORD(MSS_RSTN_CTRL_ADR, ~( 1 << MSS_LRT)); // Place LeonRT in reset by driving its rst input low
    SET_REG_WORD(MSS_RSTN_CTRL_ADR, 0xFFFFFFFF      ); // Release LeonRT reset

    // Finally pulse the IRQI.RST input to the LeonRT to actually start it running
    SET_REG_WORD(CPR_LEON_RT_VECTOR_ADR,((u32)leonStartupAddress) | LEON_IRQI_RUN | LEON_IRQI_RESET ); 
    SET_REG_WORD(CPR_LEON_RT_VECTOR_ADR,((u32)leonStartupAddress) | LEON_IRQI_RUN ); 

    return;
}
