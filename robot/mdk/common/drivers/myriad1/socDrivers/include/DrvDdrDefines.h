///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by DRAM driver
/// 
#ifndef DRV_DDR_DEF_H
#define DRV_DDR_DEF_H 

#include "mv_types.h"
                                                                       
// 1: Defines
// ----------------------------------------------------------------------------

#define DRAM_IO_OFFSET          (0)
#define DRAM_IO_MASK            (0x007FFFFF) // CPR_RETENTION_REG6  [22:0]

// Bit 0 is SR 
#define DRAM_PADS_SR_SLOW       (0 << 0)
#define DRAM_PADS_SR_FAST       (1 << 0)

// Bit 1 is P1, Bit 2 is P2
#define DRAM_IO_PULL_NORM    	(0 <<  1)   // P1 =0  P2 = 0
#define DRAM_IO_PULL_UP     	(1 <<  1)   // P1 =1  P2 = 0 
#define DRAM_IO_PULL_DOWN      	(2 <<  1)   // P1 =0  P2 = 1
#define DRAM_IO_PULL_RPT    	(3 <<  1)   // P1 =1  P2 = 1 

// Bit 3 is E1 Bit 4 is E2
#define DRAM_IO_DRV_2MA     	(0 <<  3)   // E1 =0  E2 = 0
#define DRAM_IO_DRV_4MA     	(1 <<  3)   // E1 =1  E2 = 0 
#define DRAM_IO_DRV_6MA      	(2 <<  3)   // E1 =0  E2 = 1
#define DRAM_IO_DRV_8MA     	(3 <<  3)   // E1 =1  E2 = 1 

// Bit 5 is DVVD SEL
#define DRAM_IO_VOLT_25			(0 <<  5)
#define DRAM_IO_VOLT_18			(1 <<  5)

// Bit 6 is OPBIAS_SEL
#define DRAM_IO_SW_25    		(0 <<  6)
#define DRAM_IO_SW_33    		(1 <<  6)

// Bits [10:7] are the Schmitt Trigger control for DQ Lanes [3:0]
#define DRAM_IO_DQ0_SMT_ON      (1  <<  7)
#define DRAM_IO_DQ1_SMT_ON      (1  <<  8)
#define DRAM_IO_DQ2_SMT_ON      (1  <<  9)
#define DRAM_IO_DQ3_SMT_ON      (1  << 10)
		 
// Bits [14:11] are the Schmitt Trigger control for the DQS Lines [3:0]
#define DRAM_IO_DQS0_SMT_ON      (1  << 11)
#define DRAM_IO_DQS1_SMT_ON      (1  << 12)
#define DRAM_IO_DQS2_SMT_ON      (1  << 13)
#define DRAM_IO_DQS3_SMT_ON      (1  << 14)

// Bits [19, 22] are the SMT Enables for OUT_PADS, FIFO_WE_IN
#define DRAM_IO_OUT_PADS_SMT_ON   (1  << 19) 
#define DRAM_IO_FIFO_WE_IN_SMT_ON (1  << 22) 

// Bits [15,16,18,21] are the REN for DQ,DQS,OUT_PADS,FIFO_WE_IN
#define DRAM_IO_DQ_REN          (1  << 15)
#define DRAM_IO_DQS_REN         (1  << 16)
#define DRAM_IO_OUT_PADS_REN    (1  << 18)
#define DRAM_IO_FIFO_WE_IN_REN  (1  << 21)
						   
// Bits [17, 20] are the OEN for OUT_PADS, FIFO_WE_IN
#define DRAM_IO_OUT_PADS_OEN_DIS    (1  << 17) 
#define DRAM_IO_FIFO_WE_IN_OEN_DIS  (1  << 20) 

//=========================================================================================================
// DRAM mode register settings
#ifndef CAS_LATENCY
    #define CAS_LATENCY    (3)           // CAS Latency
#endif
#ifndef BURST_LEN
    #define BURST_LEN      (4)           // Burst length
#endif
#ifndef BURST_TYPE
    #define BURST_TYPE     (1)           // Burst Type, 0 = Sequential, 1 = interleaved
#endif
#ifndef OP_MODE
    #define OP_MODE        (0x16)        // Operating mode
#endif

#define WRITE_LATENCY  CAS_LATENCY   // Write Latency equal to CAS latency
#define READ_LATENCY   CAS_LATENCY   // Read Latency equal to CAS latency

#define BL4_ENC        (0x2)
#define BL8_ENC        (0x3)
               
// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------


// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // DRV_DDR_DEF_H
