///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     API manipulating memory transfers
///
///


#ifndef _SWC_MEMORY_TRANSFER_H_
#define _SWC_MEMORY_TRANSFER_H_

/// Function that copies from source to destination
/// @param[in] dst - Destination address
/// @param[in] src - Source address
/// @param[in] len - Length to copy
/// @return - void
void swcU32memcpy(u32* dst, u32* src, u32 len);

/// Function that sets memory with a given value
/// @param[in] addr - Destination address
/// @param[in] lenB - Length to copy
/// @param[in] val - Value to set
/// @return - void
void swcU32memsetU32(u32 *addr, u32 lenB, u32 val);

#endif /* _SWC_MEMORY_TRANSFER_H_ */
