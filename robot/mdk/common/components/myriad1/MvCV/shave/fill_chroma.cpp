///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Shave kernel example
///

#include <mv_types.h>
#include <fill_chroma.h>
#include <string.h>
#include "stdio.h"


u8* fill_no_chroma(u8* in_tile, u8* out_tile_space, int width, int height)
{
    int j;
    //for U & V -> fills them with pixels of 0x8080
    for(j = 0; j < width * height; j++)
        out_tile_space[j] = 0x80;
    return &out_tile_space[0];
}

