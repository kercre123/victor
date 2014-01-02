///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     L2 cache functionality.
///
/// This implements L2 cache functinality
///
///
///

// 1: Includes
// ----------------------------------------------------------------------------

#include <mv_types.h>
#include <isaac_registers.h>
#include "DrvL2Cache.h"
#include <assert.h>
#include <DrvSvu.h>
#include <swcLeonUtils.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
/*Flush and/or Invalidate operations on a certain L2 partition*/
#define L2_OP_FLUSH_INVLD 0x1
#define L2_OP_INVLD       0x2
#define L2_OP_FLUSH       0x4

//One partition for each slice is the maximum we can get
#define MAX_PARTITIONS 8

//Structure to be initialized and used to generate each buffer address for each user
typedef struct frame_users{
	//Number of partitions asked so far
	unsigned int partitions_asked;
	//Size for each of the asked partitions
	drvL2CachePartitionSizes_enum_t size[MAX_PARTITIONS];
}partitions_cfg_t;

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data
// ----------------------------------------------------------------------------
//Variable to hold the configuration information
static partitions_cfg_t partitions_configuration={
		0,
		{PART16KB,PART16KB,PART16KB,PART16KB,PART16KB,PART16KB,PART16KB,PART16KB}
};

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------


/* ***********************************************************************

@{
----------------------------------------------------------------------------
@}
@param
     sliceNum - which slice to allocate to partition number partNum
            sliceNum can have following values:
            SLC_0 = 0x0
            SLC_1 = 0x1
            SLC_2 = 0x2
            SLC_3 = 0x3
            SLC_4 = 0x4
            SLC_5 = 0x5
            SLC_6 = 0x6
            SLC_7 = 0x7
     partNum - L2 Cache Partion ID, valid values are 0 - 7
@return
@{
info: Set slice NW_CPC DMA L2Cache partition ID
@}
 ************************************************************************* */
void DrvL2CacheSetDMAPartId(u32 sliceNum, u32 partNum)
{
    u32 writeAddr;

    writeAddr = LPB2_SLICE_0_BASE_ADR + (sliceNum * 0x10000) + SLC_TOP_NW_CPC;

    SET_REG_WORD(writeAddr, (GET_REG_WORD_VAL(writeAddr) | ((partNum & 0x7) << 24)));
}


/* ***********************************************************************

@{
----------------------------------------------------------------------------
@}
@param
     sliceNum - which slice to allocate to partition number partNum
            sliceNum can have following values:
            SLC_0 = 0x0
            SLC_1 = 0x1
            SLC_2 = 0x2
            SLC_3 = 0x3
            SLC_4 = 0x4
            SLC_5 = 0x5
            SLC_6 = 0x6
            SLC_7 = 0x7
     partNum - L2 Cache Partion ID, valid values are 0 - 7
@return
@{
info: Set slice NW_CPC LSU L2Cache partition ID
@}
 ************************************************************************* */
void DrvL2CacheSetLSUPartId(u32 sliceNum, u32 partNum)
{
    u32 writeAddr;

    writeAddr = LPB2_SLICE_0_BASE_ADR + (sliceNum * 0x10000) + SLC_TOP_NW_CPC;

    SET_REG_WORD(writeAddr, (GET_REG_WORD_VAL(writeAddr) | (partNum & 0x7)));
}

/// Set slice Windowed LSU L2Cache partition ID
/// @param[in] - sliceNum -> Slice LSU to allocate partition to
/// @param[in] - win -> Window to set partition ID to
/// @param[in] - partNum -> partition number to allocate to the said slice
/// @return    - void
///
void DrvL2CacheSetWinPartId(u32 sliceNum, drvL2CacheWindowType_t win, u32 partNum)
{
	u32 partitionWord;
	//Associate partition to the requested slice for DMA and LSU
	partitionWord=GET_REG_WORD_VAL(SLICE_WIN_CPC(sliceNum));
	//Associate all 4 windows
	partitionWord=partitionWord | (partNum<<win);
	SET_REG_WORD(  SLICE_WIN_CPC(sliceNum) , partitionWord);
	return;
}

/* ***********************************************************************

@{
----------------------------------------------------------------------------
@}
@param
     numParts - number of L2Cache partitions (1,2,4 or 8)
@return
@{
info: Partition L2Cache for slice DMAs and LSUs - numParts partitions are created.
      The size of each partition is 128kB/numParts. The L2Cache partitions
      used are 0 - (numParts-1); these are allocated to slice DMAs and LSUs 0 -
      (numParts-1) by setting the DMA and LSU L2Cache partition bits of the NW_CPC
      register for the corresponding slice.
@}
 ************************************************************************* */
void DrvL2CacheSetupDMAPartitionsNumber (u32 numParts)
{
    u32 i, partSize, startOffset;

    partSize = (numParts == 1) ? 0 :
                (numParts == 2) ? 1 :
                (numParts == 4) ? 2 : 3; /* Assuming only 1,2,4 or 8 for numParts */

    startOffset = (numParts == 2) ? 4 :
                   (numParts == 4) ? 2 : 1; /* Assuming only 1,2,4 or 8 for numParts */
                
    /* Setup numParts cache partitions for slice DMAs */
    SET_REG_WORD(L2C_STOP_ADR, 0x1);                         /* Stop */

    while ((GET_REG_WORD_VAL(L2C_BUSY_ADR) & 0x1)) NOP;      /* Wait for idle */

    /* Set numParts partitions, flush and invalidate */
    for(i = 0; i < numParts; i++)
    {
        SET_REG_WORD(L2C_PTRANS0_ADR + (i * 0x4), ((i * startOffset) << 2) | partSize);
        SET_REG_WORD(L2C_FLUSHINV_ADR,            (i << 3) | 0x2); // see Bug #14636 on l2 init content should just be invalidated( flushing will corrupt existing ddr content ) 

        /* Set the slice DMA L2 Cache partition IDs */
        DrvL2CacheSetDMAPartId(i, i);

		/* Set the slice LSU L2 Cache partition IDs */
        DrvL2CacheSetLSUPartId(i, i);

        /* Wait done flush/invalidate */
        while ((GET_REG_WORD_VAL(L2C_BUSY_ADR) & 0x2)) NOP;
    }
    SET_REG_WORD(L2C_STOP_ADR, 0x0);                         /* Go */
}


/* ***********************************************************************
@{
----------------------------------------------------------------------------
@}
@param
     partNo - partition number to be flushed/invalidated
@return
@{
info: Perform selected flush, invalidate or flush and invalidate operation
      on selected L2 Cache partition
@}
 ************************************************************************* */
void PartitionUtil (u32 partNo, u32 operation)
{
   SET_REG_WORD(L2C_STOP_ADR, 0x1);                            // Stop L2
   
   while ((GET_REG_WORD_VAL(L2C_BUSY_ADR) & 0x1)) NOP;         // Wait for idle
   
   SET_REG_WORD(L2C_FLUSHINV_ADR, (partNo << 3) | operation); // Flush & | Invalidate partition
   
   while ((GET_REG_WORD_VAL(L2C_BUSY_ADR) & 0x2)) NOP;         //Wait for DONE
   
   SET_REG_WORD(L2C_STOP_ADR, 0x0);                            // Go
}



/* ***********************************************************************
@{
----------------------------------------------------------------------------
@}
@param
     partNo - partition number to be flushed
@return
@{
info: Flush selected partition in L2 Cache.

@}
 ************************************************************************* */
void DrvL2CachePartitionFlush (u32 partNo)
{
   PartitionUtil (partNo, L2_OP_FLUSH);
}



/* ***********************************************************************
@{
----------------------------------------------------------------------------
@}
@param
     partNo - partition number to be invalidated
@return
@{
info: Invalidate selected partition in L2 Cache.
@}
 ************************************************************************* */
void DrvL2CachePartitionInvalidate (u32 partNo)
{
   PartitionUtil (partNo, L2_OP_INVLD);
}



/* ***********************************************************************
@{
----------------------------------------------------------------------------
@}
@param
     partNo - partition number to be flushed and invalidated
@return
@{
info: Flush and invalidte selected partition in L2 Cache.
@}
 ************************************************************************* */
void DrvL2CachePartitionFlushAndInvalidate (u32 partNo)
{
   PartitionUtil (partNo, L2_OP_FLUSH_INVLD);
}

void DrvL2CacheClearPartitions(void)
{
	partitions_configuration.partitions_asked=0;
	return;
}

/// Sets up a new partition
/// @param[in] - partitionSize -> Partition size requested for this particular partition
/// @return    - void
///
void DrvL2CacheSetupPartition(drvL2CachePartitionSizes_enum_t partitionSize)
{
	partitions_configuration.size[partitions_configuration.partitions_asked]=partitionSize;
	partitions_configuration.partitions_asked++;
	return;
}

/// Allocates all requested partitions
/// @param[in] - partitionSize -> Partition size requested for this particular partition
/// @param[in] - sliceNo -> slice to allocate the newly created partition to
/// @return    - 1/0 - ERROR/SUCCESS
///
unsigned int DrvL2CacheAllocateSetPartitions(void)
{
	unsigned int result=0,i;
	unsigned int size16KBsectionsLeft;
	unsigned int partitionStartPos;
	//We have 8 16KB sections assignable as partitions
	size16KBsectionsLeft=8;
	//We ned to keep track of the partition start address inside L2cache
	partitionStartPos=0x0;
	i=0;
	while ( (i<partitions_configuration.partitions_asked) && (result==0) )
	{
		if (size16KBsectionsLeft<=0){
			//Fail: we are being requested to allocate a partition
			//but cannot do so anymore because of lack of 16 KB sections
			result=1;
			assert("Cannot allocate partitions\n");
		}else{
			unsigned int partitionWord;
			//Decrease the number of sections left with the number of sections requested
			size16KBsectionsLeft-=partitions_configuration.size[i];
			//Build up the L2Cache partition word requested for the current partition
			partitionWord=(partitionStartPos<<2) | partitions_configuration.size[i];
			//Fill in the partition word for the requested partition
			SET_REG_WORD(L2C_PTRANS0_ADR + (i * 0x4), partitionWord );
//			//Associate partition to the requested slice for DMA and LSU
//			partitionWord=GET_REG_WORD_VAL(SLICE_WIN_CPC(partitions_configuration.slices[i]));
//			//Associate all 4 windows
//			partitionWord=partitionWord | i | (i<<4) | (i<<8) | (i<<12);
//			SET_REG_WORD(  SLICE_WIN_CPC(partitions_configuration.slices[i]) , partitionWord);
//			//Associate non windowed accesses to
//			partitionWord=GET_REG_WORD_VAL(SLICE_NWN_CPC(partitions_configuration.slices[i]));
//			//0x40000018 -> enable TMU L2 cacheable, DMA L2 cacheable, LSU L2 cacheable
//			//(i&7) | ((i&7)<<24) | ((i&7)<<27) -> Set L2 cache partitions for :
//			// LSU, DMA and TMU
//			partitionWord=partitionWord | 0x40000018 | (i&7) | ((i&7)<<24) | ((i&7)<<27);
//			SET_REG_WORD(  SLICE_NWN_CPC(partitions_configuration.slices[i]) , partitionWord);
		}
		i++;
	}
	//And just to make sure everything is allright
	// setup master IDs
	SET_REG_WORD(L2C_MXITID_ADR, 0x76543210);
	// invalidate the cache
	for (i=0;i<partitions_configuration.partitions_asked;i++){
		DrvL2CachePartitionInvalidate(i);
	}
	return result;
}
