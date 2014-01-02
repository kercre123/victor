/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Definitions and types needed by the Lcd operations library
///

#ifndef _LCD_OVERLAY_API_DEFINES_H_
#define _LCD_OVERLAY_API_DEFINES_H_

// 1: Defines
// ----------------------------------------------------------------------------
#define BLUE                0xFFFF0000
#define GREEN               0xFF00FF00
#define RED                 0xFF0000FF
#define BLACK               0x00000000
#define WHITE               0xFFFFFFFF
#define TRANSPARENT         BLACK

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
struct _spriteHandle
{
    unsigned int spriteId;//position & value of the sprite
    // the 4 lines of the sprite spread over all combinations of a 32bit word
    unsigned int line0[16];
    unsigned int line1[16];
    unsigned int line2[16];
    unsigned int line3[16];
    // the 4 lines of the sprite remaining pixels that have to pass to the next
    // address, if the pixel is located between two 32bit words
    unsigned int line0_rem[16];
    unsigned int line1_rem[16];
    unsigned int line2_rem[16];
    unsigned int line3_rem[16];
};

typedef struct _spriteHandle SpriteHandle;


// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------
#endif
