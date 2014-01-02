///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Fill chromas with 0x80
///

#ifndef __FILLCHROMA_H__
#define __FILLCHROMA_H__

#include <mv_types.h>

/// Function to fill chromas with 0x80, "no value"/"indiferent" chroma
/// @param[in]  in_tile address of the memory where the input tile is stored
/// @param[in]  out_tile_space address of the memory where the output tile is stored
/// @param[in]  width width of the tile
/// @param[in]  height height of the tile
/// @return u8* pointer to the memory area where the tile was filled
u8* fill_no_chroma(u8* in_tile,u8* out_tile_space, int width, int height);


#endif //__FILLCHROMA_H__
