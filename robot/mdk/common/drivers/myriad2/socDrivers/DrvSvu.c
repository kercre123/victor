///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Subroutines dealing with shave low level functionality
///

#include <mv_types.h>
#include <registersMyriad.h>
//#include "DrvL2Cache.h"
#include <DrvSvu.h>
#include <DrvSvuDefines.h>
#include <assert.h>
#include "swcShaveLoader.h"
#include <swcLeonUtils.h>


// Do this so we can write to SVUs using for loops
u32 SVU_BASE_ADDR[TOTAL_NUM_SHAVES] = { CMX_SLICE_0_BASE_ADR,
                                        CMX_SLICE_1_BASE_ADR,
                                        CMX_SLICE_2_BASE_ADR,
                                        CMX_SLICE_3_BASE_ADR,
                                        CMX_SLICE_4_BASE_ADR,
                                        CMX_SLICE_5_BASE_ADR,
                                        CMX_SLICE_6_BASE_ADR,
                                        CMX_SLICE_7_BASE_ADR,
                                        CMX_SLICE_8_BASE_ADR,
                                        CMX_SLICE_9_BASE_ADR,
                                        CMX_SLICE_10_BASE_ADR,
                                        CMX_SLICE_11_BASE_ADR,
                                         };

u32 SVU_CTRL_ADDR[TOTAL_NUM_SHAVES] = { SHAVE_0_BASE_ADR,
                                        SHAVE_1_BASE_ADR,
                                        SHAVE_2_BASE_ADR,
                                        SHAVE_3_BASE_ADR,
                                        SHAVE_4_BASE_ADR,
                                        SHAVE_5_BASE_ADR,
                                        SHAVE_6_BASE_ADR,
                                        SHAVE_7_BASE_ADR,
                                        SHAVE_8_BASE_ADR,
                                        SHAVE_9_BASE_ADR,
                                        SHAVE_10_BASE_ADR,
                                        SHAVE_11_BASE_ADR,
                                        };

u32 SVU_DATA_ADDR[TOTAL_NUM_SHAVES] = { CMX_SLICE_0_BASE_ADR,
                                        CMX_SLICE_1_BASE_ADR,
                                        CMX_SLICE_2_BASE_ADR,
                                        CMX_SLICE_3_BASE_ADR,
                                        CMX_SLICE_4_BASE_ADR,
                                        CMX_SLICE_5_BASE_ADR,
                                        CMX_SLICE_6_BASE_ADR,
                                        CMX_SLICE_7_BASE_ADR,
                                        CMX_SLICE_8_BASE_ADR,
                                        CMX_SLICE_9_BASE_ADR,
                                        CMX_SLICE_10_BASE_ADR,
                                        CMX_SLICE_11_BASE_ADR,
                                        };


u32 SVU_INST_BASE[TOTAL_NUM_SHAVES] = { CMX_SLICE_0_BASE_ADR,
                                        CMX_SLICE_1_BASE_ADR,
                                        CMX_SLICE_2_BASE_ADR,
                                        CMX_SLICE_3_BASE_ADR,
                                        CMX_SLICE_4_BASE_ADR,
                                        CMX_SLICE_5_BASE_ADR,
                                        CMX_SLICE_6_BASE_ADR,
                                        CMX_SLICE_7_BASE_ADR,
                                        CMX_SLICE_8_BASE_ADR,
                                        CMX_SLICE_9_BASE_ADR,
                                        CMX_SLICE_10_BASE_ADR,
                                        CMX_SLICE_11_BASE_ADR,
                                        };

//void svuSliceReset(u32 shaveNumber,u32 mask)
//{
//	SET_REG_WORD(DCU_SVU_SLICE_RST(shaveNumber), ~mask); // Clear all of the slice bits
//		return;
//}

void DrvSvuSliceResetAll(u32 shaveNumber)
{
	SET_REG_WORD(DCU_SVU_SLICE_RST(shaveNumber), ~(SVU_RESET | DCU_RESET | SLICE_RESET | /*TLB_RESET |*/
			SLICE_FIFO_RESET | /*L1_CACHE_RESET |*/ DMA_RESET | TMU_RESET)); // Clear all of the slice bits
	return;
}

void DrvSvuSliceResetOnlySvu(u32 shaveNumber)
{
	SET_REG_WORD(DCU_SVU_SLICE_RST(shaveNumber), ~(SLICE_RESET | SVU_RESET | DCU_RESET)); // Clear only the bits we want to reset
	return;
}

//Set a TRF register to a value
void DrvSvutTrfWrite(u32 shaveNumber, u32 registerOffset, u32 value)
{
	u32 TRFAddress = DCU_SVU_TRF(shaveNumber, registerOffset);
	SET_REG_WORD(TRFAddress, value);
}
//Get the value from a TRF register
u32 DrvSvutTrfRead(u32 shaveNumber, u32 registerOffset)
{
	u32 retValue;
	u32 TRFAddress = DCU_SVU_TRF(shaveNumber, registerOffset);
	retValue = GET_REG_WORD_VAL(TRFAddress);
	return retValue;
}

//Enable Performance counter
void DrvSvuEnablePerformanceCounter(u32 shaveNumber, u32 pcNb, u32 countType)
{
    u32 bpCtrl;
	if (countType == PC_IBP0_EN)
	{
	    bpCtrl = GET_REG_WORD_VAL(DCU_IBC0(shaveNumber));
		SET_REG_WORD(DCU_IBC0(shaveNumber), (bpCtrl | 0x00002000));
	}
	else if (countType == PC_IBP1_EN)
	{
	    bpCtrl = GET_REG_WORD_VAL(DCU_IBC1(shaveNumber));
		SET_REG_WORD(DCU_IBC1(shaveNumber), (bpCtrl | 0x00002000));
	}
	else if (countType == PC_DBP0_EN)
	{
	    bpCtrl = GET_REG_WORD_VAL(DCU_DBC0(shaveNumber));
		SET_REG_WORD(DCU_DBC0(shaveNumber), (bpCtrl | 0x20000000));
	}
	else if (countType == PC_DBP1_EN)
	{
	    bpCtrl = GET_REG_WORD_VAL(DCU_DBC1(shaveNumber));
		SET_REG_WORD(DCU_DBC1(shaveNumber), (bpCtrl | 0x20000000));
	}
	else
	{
		// enable history
	    bpCtrl = GET_REG_WORD_VAL(DCU_OCR(shaveNumber));
		SET_REG_WORD(DCU_OCR(shaveNumber), (bpCtrl | 0x00000800));

		//enable debug
		u32 dcrValue = GET_REG_WORD_VAL(DCU_DCR(shaveNumber));
		SET_BITS(DCU_DCR(shaveNumber), dcrValue, DCR_DBGE);
	}

    if (pcNb)
	{
	    SET_REG_WORD(DCU_PCC1(shaveNumber), countType);
	}
	else
	{
	    SET_REG_WORD(DCU_PCC0(shaveNumber), countType);
	}
}

/// Read executed instructions
/// @param[in] shaveNumber - shave number
/// @return    - u32 get executed Instructions count
///
u32 DrvSvuGetPerformanceCounter0(u32 shaveNumber)
{
	return GET_REG_WORD_VAL(DCU_PC0(shaveNumber));
}

/// Read stall cycles
/// @param[in] shaveNumber - shave number
/// @return    - u32 get stall cycles
///
u32 DrvSvuGetPerformanceCounter1(u32 shaveNumber)
{
	return GET_REG_WORD_VAL(DCU_PC1(shaveNumber));
}

//  Use this for cache ctrl transactions that do not need an address
//  These are ctrlType=:
//       L1C_INVAL_ALL
//       L1C_FLUSH_ALL
//       L1C_FLUSHINV_ALL
//
// L1 cache utils
//
//  ctrlType is one of L1C_* defined above
//

void DrvSvuL1CacheCtrl(u32 shvNumber,     u32 ctrlType)
{
    u32 lVal;
    int stopBit = 0x8;

	// Set Stop
	lVal = GET_REG_WORD_VAL(SHAVE_CACHE_CTRL(shvNumber));
//	DEBUG_INFO ("SHAVE_CACHE_CTRL = ");
//	ACTUAL_DATA (lVal);
	SET_REG_WORD(SHAVE_CACHE_CTRL(shvNumber), (lVal | stopBit));

	asm("nop");
	asm("nop");
	asm("nop");

	while(GET_REG_WORD_VAL(SHAVE_CACHE_CTRL(shvNumber) + 0x18) & 0x01);

    // Select Invalidate Operation
	SET_REG_WORD(SHAVE_CACHE_DBG_TXN_TYPE(shvNumber),ctrlType);
	asm("nop");
	asm("nop");
	asm("nop");

	while(GET_REG_WORD_VAL(SHAVE_CACHE_CTRL(shvNumber) + 0x18) & 0x03);

    // Clear Stop
	lVal = GET_REG_WORD_VAL(SHAVE_CACHE_CTRL(shvNumber));

//	DEBUG_INFO ("SHAVE_CACHE_CTRL = ");
//	ACTUAL_DATA (lVal);
	SET_REG_WORD(SHAVE_CACHE_CTRL(shvNumber), (lVal & ~stopBit));
}

void DrvSvuEnableL1Cache(u32 slice)
{
  // Set non-windowed (L1) cache policy configuration for LSU DDRL2Cache/Bypass accesses
  volatile unsigned int *pu32NWCPC = (volatile unsigned int *)(0x80140024+(slice<<16));
  (*pu32NWCPC) = 0x180;                // LSU ENABLE DDRL2C/B L1 CACHING

  // Invalidate and enable L1Cache
  volatile unsigned int *pu32L1CacheCnfg = (volatile unsigned int *)(0x80147000+(slice<<16));
  volatile unsigned int *pu32L1CacheCtrl = (volatile unsigned int *)(0x80147014+(slice<<16));
  volatile unsigned int *pu32L1CacheStat = (volatile unsigned int *)(0x80147018+(slice<<16));

  (*pu32L1CacheCnfg) = 0x9;            // ENABLE (STOPPED)
  (*pu32L1CacheCtrl) = 0x5;            // INVALIDATE
  do {
      NOP;
  } while ((*pu32L1CacheStat) != 0x0); // BUSY
  (*pu32L1CacheCnfg) = 0x1;            // ENABLE

  return;
}

//  Use this for cache ctrl transactions that need an address
//  These are ctrlType=:
//        L1C_PFL1
//        L1C_PFL1_LOCK
//        L1C_PFL2
//        L1C_FLUSH_LINE
//        L1C_INVAL_LINE
//
void DrvSvuL1CacheCtrlLine(u32 shvNumber, u32 ctrlType, u32 address)
{
    u32 lVal;
    int stopBit = 0x8;

	// Set Stop
	lVal = GET_REG_WORD_VAL(SHAVE_CACHE_CTRL(shvNumber));
//	DEBUG_INFO ("SHAVE_CACHE_CTRL = ");
//	ACTUAL_DATA (lVal);
	SET_REG_WORD(SHAVE_CACHE_CTRL(shvNumber), (lVal | stopBit));

    // Set address and then operations
    SET_REG_WORD(SHAVE_CACHE_DBG_TXN_ADDR(shvNumber),address);
	SET_REG_WORD(SHAVE_CACHE_DBG_TXN_TYPE(shvNumber),ctrlType);

    // Clear Stop
	lVal = GET_REG_WORD_VAL(SHAVE_CACHE_CTRL(shvNumber));
//	DEBUG_INFO ("SHAVE_CACHE_CTRL = ");
//	ACTUAL_DATA (lVal);
	SET_REG_WORD(SHAVE_CACHE_CTRL(shvNumber), (lVal & ~stopBit));

	// Wait for flush/invalidate busy to clear
    do
	{
	    NOP;
		NOP;
		NOP;
		NOP;
		NOP;
		NOP;
		NOP;
		NOP;
		NOP;
		NOP;
		lVal = GET_REG_WORD_VAL (SHAVE_CACHE_STATUS(shvNumber));
    }
	while ((lVal & 0x2) != 0);
}

// Invalidate L1 Cache
void DrvSvuInvalidateL1Cache(u32 shvNumber)
{
    DrvSvuL1CacheCtrl(shvNumber, L1C_INVAL_ALL);
}

void DrvSvutIrfWrite(u32 svu, u32 irfLoc, u32 irfVal) {
    SET_REG_WORD(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+IRF_BASE+(irfLoc<<2), irfVal);
}

u32 DrvSvutIrfRead(u32 svu, u32 irfLoc) {
   u32 retValue;
   do
	   {
	   retValue =    GET_REG_WORD_VAL(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+IRF_BASE+(irfLoc<<2));
	   }
   while (retValue == 0xDEAD1501  ); // This special value indicates that a port clash occured and we could not read the register
return retValue;

  /*  SET_REG_WORD(   SVU_CTRL_ADDR[svu] + SVU_RFC_ADDR, 0x00000140 + irfLoc);
    u32 irfVal = GET_REG_WORD_VAL(SVU_CTRL_ADDR[svu]+SVU_RF0_ADDR);
    return irfVal;*/

}

void DrvSvutVrfWrite(u32 svu, u32 vrfLoc, u32 vrfEle, u32 vrfVal) {
    SET_REG_WORD(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+VRF_BASE+(vrfLoc<<4)+(vrfEle<<2), vrfVal);
}

u32 DrvSvutVrfRead(u32 svu, u32 vrfLoc, u32 vrfEle ) {
    return GET_REG_WORD_VAL(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+VRF_BASE+(vrfLoc<<4)+(vrfEle<<2));
}

void DrvSvutZeroRfs(u32 svu) {
  u32 i, k;

  svu = svu; // reserve for future use
  for (i=0; i<32; i++) {
    DrvSvutIrfWrite(0, i, 0x00000000);
      for (k=0; k<4; k++) {
        DrvSvutVrfWrite(0, i, k, 0x00000000);
      }
  }
}

//void svut_pc_write(u32 svu, u32 pc_val) {
//	SET_REG_WORD(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+SVU_PTR, pc_val);
//}
//
//void svut_start(u32 svu) {
//    //without affecting other bits in OCR
//    u32 OCR_val = GET_REG_WORD_VAL(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+SVU_OCR);
//    SET_REG_WORD(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+SVU_OCR, (OCR_val&0xFFFFFFFB));
//}

void DrvSvuStart(u32 svu, u32 pcVal)
{
    u32 OCR_val;
    if ((pcVal & 0xFF000000) == 0x1D000000) // Optimised away when assert() not present
    {   // If we are attempting to start a shave in windowed space, make sure window pointer is set
        assert(GET_REG_WORD_VAL((SHAVE_0_BASE_ADR + (SVU_SLICE_OFFSET * svu) + SLC_TOP_OFFSET_WIN_B )) != 0); 
    }

    OCR_val = GET_REG_WORD_VAL(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+SVU_OCR);
	SET_REG_WORD(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+SVU_PTR, pcVal);
    //without affecting other bits in OCR
    SET_REG_WORD(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+SVU_OCR, (OCR_val&0xFFFFFFFB));
	return;
}

void DrvSvutStop(u32 svu) {
    //without affecting other bits in OCR
    u32 OCR_val = GET_REG_WORD_VAL(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+SVU_OCR);
    SET_REG_WORD(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+SVU_OCR, (OCR_val|0x4));
}

u32 DrvSvuIsStopped(u32 svu) {
    u32 status = GET_REG_WORD_VAL(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+SVU_OSR);
    return (status & 0x00000008);
}

u32 DrvSvutSwiTag(u32 svu) {
	u32 status = GET_REG_WORD_VAL(SVU_CTRL_ADDR[svu]+SLC_OFFSET_SVU+SVU_ISR);
    return ((status>>13) & 0x0000001F);
}




