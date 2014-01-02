///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Shave kernel example
///

#ifndef __KERNEL_UNIT_TEST_SAMPLE_H__
#define __KERNEL_UNIT_TEST_SAMPLE_H__

// 1: Includes
// ----------------------------------------------------------------------------
#include <mv_types.h>

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

#ifdef __cplusplus
namespace mvcv{
#endif

/// Dummy kernel that only copies data around
/// @param[in] in_tile address of the memory where the input tile is stored
/// @param[in] out_tile_space address of the memory where the output tile is stored
/// @param[in] width width of the tile
/// @param[in] height height of the tile
/// @return u8* pointer to the memory area where the tile was copied
u8* dummy_kernel_result(u8* in_tile, u8* out_tile_space, int width, int height);

/// Kernel that bit or blends two planes
/// @param[in] in1 address to plane 1 to blend
/// @param[in] in2 address to plane 2 to blend
/// @param[out] out pointer to output memory with blended result
/// @param[in] width width of the planes
/// @param[in] height height of the planes
/// @return none
void blendkernel(u8* in1, u8* in2, u8* out, u32 width, u32 height);

/// Kernel that blurs with 3x5 radius (5 lines, 3 columns)
/// @param[in]  in_lines pointer to the array of input line pointers
/// @param[out] out_lines pointer to the array of output line pointers
/// @param[in]  width width of the planes
/// @return none
void avg3x5kernel_newint(u8** in_lines, u8** out_lines, u32 width);

/// Kernel that blurs with 1x5 radius (5 lines, 1 column)
/// @param[in]  in address to plane 1 to blend
/// @param[out] out pointer to output memory with blended result
/// @param[in]  width width of the planes
/// @return none
void avg1x5kernel(u8** in, u8** out, u32 width);

#ifdef __cplusplus
}
#endif

// 4: inline function implementations
// ----------------------------------------------------------------------------

#endif //__KERNEL_UNIT_TEST_SAMPLE_H__
