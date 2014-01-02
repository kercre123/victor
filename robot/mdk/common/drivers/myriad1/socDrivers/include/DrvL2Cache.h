///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     L2 cache functionality.
///
/// This implements L2 cache functionality
///
///
///

#ifndef _BRINGUP_SABRE_L2C_H_
#define _BRINGUP_SABRE_L2C_H_
#include "mv_types.h"

// 1: Defines
// ----------------------------------------------------------------------------
/// In this mode the L2Cache acts as a 128KB SRAM at address 0x40000000
/// This mode is typically used for unstacked Myriad parts that do not have DDR
#define L2CACHE_DIRECT_MODE         (0x7)  
/// In this mode the L2Cache acts as a cache for the DRAM
/// If in doubt the cache should typically be configured as L2CACHE_NORMAL_MODE
#define L2CACHE_NORMAL_MODE         (0x6)  

/// Myriad2 forward compatibility
#define SHAVE_L2CACHE_DIRECT_MODE    L2CACHE_DIRECT_MODE
#define SHAVE_L2CACHE_NORMAL_MODE    L2CACHE_NORMAL_MODE


// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
typedef enum{
	PART128KB=0x0,
	PART64KB=0x1,
	PART32KB=0x2,
	PART16KB=0x3
}drvL2CachePartitionSizes_enum_t;

typedef enum{
	L2CACHEWIN_C=0,
	L2CACHEWIN_D=4,
	L2CACHEWIN_E=8,
	L2CACHEWIN_F=12
}drvL2CacheWindowType_t;

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Set slice NW_CPC DMA L2Cache partition ID
/// @param[in]  sliceNum  Slice DMA to allocate partition to
/// @param[in]  partNum  partition number to allocate to the said slice
/// @return     void
///
void DrvL2CacheSetDMAPartId(u32 sliceNum, u32 partNum);

/// Set slice NW_CPC LSU L2Cache partition ID
/// @param[in]  sliceNum  Slice LSU to allocate partition to
/// @param[in]  partNum  partition number to allocate to the said slice
/// @return     void
///
void DrvL2CacheSetLSUPartId(u32 sliceNum, u32 partNum);

/// Set slice Windowed LSU L2Cache partition ID
/// @param[in]  sliceNum  Slice LSU to allocate partition to
/// @param[in]  win  Window to set partition ID to
/// @param[in]  partNum  partition number to allocate to the said slice
/// @return     void
///
void DrvL2CacheSetWinPartId(u32 sliceNum, drvL2CacheWindowType_t win, u32 partNum);

/// Perform flush on L2Cache partition
/// @param[in] partNo Partition to flush
/// @return    void
///
void DrvL2CachePartitionFlush (u32 partNo);

/// Perform invalidate on L2Cache partition
/// @param[in]  partNo  Partition to invalidate
/// @return     void
///
void DrvL2CachePartitionInvalidate (u32 partNo);

/// Perform flush and invalidate on L2Cache partition
/// @param[in]  partNo  Partition to flush and invalidate
/// @return     void
///
void DrvL2CachePartitionFlushAndInvalidate (u32 partNo);

/// Reinitialize partitions allocation
/// @return     void
///
void DrvL2CacheClearPartitions(void);

/// Sets up a new partition
/// @param[in]  partitionSize  Partition size requested for this particular partition
/// @return     void
///
void DrvL2CacheSetupPartition(drvL2CachePartitionSizes_enum_t partitionSize);

/// Allocates all requested partitions
///
/// @return     1/0 - ERROR/SUCCESS
///
unsigned int DrvL2CacheAllocateSetPartitions(void);

/// Partition L2Cache for slice DMAs - numParts partitions are created. - ONLY HERE FOR CONVENIENCE (DEPRECATED)
/// @param[in]  numParts  Number of equally sized partitions to create. My only be: 1, 2, 4 or 8
/// @return     void
///
void DrvL2CacheSetupDMAPartitionsNumber (u32 numParts);

#endif
