///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for accessing the CMX DMA controller. (Macros, Types)
///

#ifndef DRV_CMX_DMA_DEFINES_H
#define DRV_CMX_DMA_DEFINES_H

#include <mv_types.h>

// TODO: add all register fields here (MASK && SHIFT)

#define DRV_CMX_DMA_MUTEX 31
#define DRV_CMX_DMA_MAX_INTERRUPT_NUMBER 15

/// @brief Interrupt 15 is reserved.
///
/// It is used for transactions, that we don't want to be notified of when they complete
#define DRV_CMX_DMA_IGNORED_INTERRUPT_NUMBER DRV_CMX_DMA_MAX_INTERRUPT_NUMBER
#define DRV_CMX_DMA_MAX_ALLOCATABLE_INTERRUPT_NUMBER (DRV_CMX_DMA_MAX_INTERRUPT_NUMBER - 1)

#define DRV_CMX_DMA_CDMA_CTRL_START_CONTROL_REGISTER_AGENT_MASK (1 << 0)
#define DRV_CMX_DMA_CDMA_CTRL_START_LINK_COMMAND_AGENT_MASK (1 << 1)
#define DRV_CMX_DMA_CDMA_CTRL_ENABLE_DMA_MASK (1 << 2)
#define DRV_CMX_DMA_CDMA_CTRL_SOFTWARE_RESET_MASK (1 << 3)

#define DRV_CMX_DMA_CDMA_STATUS_REGISTER_COMMAND_AGENT_BUSY (1 << 1)

#define DRV_CMX_DMA_CDMA_CFG_LINK_INTERRUPT_SHIFT (43)
#define DRV_CMX_DMA_CDMA_CFG_LINK_INTERRUPT_MASK (((1ULL << 47) - 1) ^ ((1ULL << DRV_CMX_DMA_CDMA_CFG_LINK_INTERRUPT_SHIFT) - 1))
#define DRV_CMX_DMA_CDMA_CFG_LINK_TRANSACTION_TYPE_MASK (1ULL << 32)
#define DRV_CMX_DMA_CDMA_CFG_LINK_TRANSACTION_PRIORITY_SHIFT (33)
#define DRV_CMX_DMA_CDMA_CFG_LINK_BURST_LENGTH_SHIFT (35)

#define DRV_CMX_DMA_KEEP_INTERRUPT 1
#define DRV_CMX_DMA_IGNORE_INTERRUPT 0

#define DRV_CMX_DMA_DEFAULT_TRANSACTION_PRIORITY 1
#define DRV_CMX_DMA_DEFAULT_BURST_LENGTH 8

/// @brief A linked list data structure that contains a Type=0, 1D transaction command
struct CmxDmaTransaction1D
{
    union
    {
        struct
        {
            struct CmxDmaTransaction *LinkAddress; ///< linked list, next element pointer
            u32 Config; ///< the configuration bits only
        } BrokenUpRegister; ///< broken up into the configuration bits, and the linked list pointer
        u64 FullRegister; ///< full 64-bit value. Use the DRV_CMX_DMA_CDMA_CFG_LINK* macros with this.
    } CfgLink; ///< Configuration bitfield, and pointer to the next command in the linked list
    void *Src; ///< Pointer to the source of the data transfer
    void *Dst; ///< Pointer to the destination
    u32 Len;   ///< Length of transaction in bytes
    u32 unused;
};

/******************************************************
Updated 1D transaction structure

struct CmxDmaTransaction1D
{
	void *LinkAddress;
	union
	{
		struct configBits
		{
			u32 type             : 2;
			u32 priority         : 2;
			u32 brstLength       : 4;
			u32 id               : 4;
			u32 interruptTrigger : 4;
			u32 partition        : 4;
			u32 disableInt       : 1;
			u32 unused           : 6;
			u32 skipNr           : 5;
		}
		u32 fullRegister;
	}cfgLink;
    void *src; ///< Pointer to the source of the data transfer
    void *dst; ///< Pointer to the destination
	u32 length;
	u32 no_planes;
	u32 src_plane_stride;
	u32 dst_plane_stride;
}

*********************************************************/

/// @brief A linked list data structure that contains a Type=1, 2D transaction command
struct CmxDmaTransaction2D
{
    struct CmxDmaTransaction1D Transaction1D; ///< fields that are inherited from CmxDmaTransaction1D
    u32 SrcWidth;  ///< Source Width
    u32 SrcStride; ///< Source Stride
    u32 DstWidth;  ///< Destination Width
    u32 DstStride; ///< Destination Stride
};

/// @brief A structure that is used to store persistent data across calls to the DrvCmxDma* functions.
///
/// This structure will contain command descriptors that were either not
/// built initially in CMX, or they were built on the fly.
/// It also contains information about which interrupts are already used, and
/// cannot be auto-allocated.
struct CmxDmaState {
    void *CmxBuffer;    ///< Pointer to a buffer in CMX that will hold linked commands
    u32 SizeInBytes;    ///< Size of CmxBuffer in bytes
    u32 FirstUsedByte;  ///< Variable used for queue management
    u32 FirstFreeByte;  ///< Variable used for queue management
    u32 UsedInterrupts; ///< A bitfield specifying which interrupts are unavailable for automatic allocation
};

#endif
