///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     API manipulating memory trabnsfers
///
///


#ifndef _SWC_MEMORY_TRANSFER_H_
#define _SWC_MEMORY_TRANSFER_H_

/// Function that copies from source to destination
/// @param[in] - Destination address
/// @param[in] - Source address
/// @param[in] - Length to copy
/// @param[out] - void
void swcU32memcpy(u32* dst, u32* src, u32 len);

/// Function that sets memory with a givven value
/// @param[in] - Destination address
/// @param[in] - Length to copy
/// @param[in] - Value to set
/// @param[out] - void
void swcU32memsetU32(u32 *addr, u32 lenB, u32 val);

#endif /* _SWC_MEMORY_TRANSFER_H_ */
