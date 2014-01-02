// Drivers includes
#include "LcdOverlayApi.h"
#include "LcdOverlayApiDefines.h"
#include "DrvLcd.h"
#include "DrvSvu.h"
#include "mv_types.h"
#include "stdio.h"

//buffer for the graphics layer
frameBuffer *Graphics;
unsigned int glSize = 0;


// color LUT for the LCD GL
unsigned int osdColorTable[] = {TRANSPARENT, BLUE, GREEN, RED};

unsigned int DBG = 0;

// Test function for the OSD; Draws some patterns with different sprites on the screen.
void LcdOverlayTest()
{
    SpriteHandle spHandl1;
    SpriteHandle spHandl2;
    SpriteHandle spHandl3;
    unsigned int i;

    // Clearing the LcdOverlay layer
    LcdOverlayClearGraphics((unsigned int*)Graphics->p1, glSize, 0);
    LcdOverlaySpriteInit(&spHandl1, 0x3CC3C33C);
    LcdOverlaySpriteInit(&spHandl2, LcdOverlaySpriteFlip(0x20802000));
    LcdOverlaySpriteInit(&spHandl3, LcdOverlaySpriteFlip(0x41141441));
    // Sprite init must be done before any rendering

    // Drawing a set of sprites in every possible x location to see if it spreads ok over 32bit words
    for (i = 0; i < 70; i++)
    {
        LcdOverlayWrite2BppSprite4x4(&spHandl1,(unsigned int*)Graphics->p1, i, i*8);
        // Sprite rendering for different sprite format; LcdOverlaySpriteFlip used just in the test
        LcdOverlayWrite2BppSprite4x4(&spHandl2,(unsigned int*)Graphics->p1, i + 6, i*8);
        // Sprite rendering for different sprite format; LcdOverlaySpriteFlip used just in the test
        LcdOverlayWrite2BppSprite4x4(&spHandl3,(unsigned int*)Graphics->p1, i + 12, i*8);
    }
    // Sprite rendering for different sprite format; LcdOverlaySpriteFlip used just in the test

}

void LcdOverlayInit( frameBuffer *overlayFrame, unsigned int size)
{
    // Generate sprite either with the excell spreadsheet, either
    // by calling LcdOverlaySpriteFlip(sprite);

    unsigned int sprite = 0x08020800;
    Graphics = overlayFrame;
    glSize = size;
    //    LcdOverlaySpriteInit(spHandl, spHandl->spriteId);
    //    LcdOverlayClear();
}

void LcdOverlaySpriteInit(SpriteHandle *spHandl, unsigned int sprite)
{
    unsigned int i;
    unsigned char sprite_byte0, sprite_byte1, sprite_byte2, sprite_byte3;

    spHandl->spriteId = sprite;
    // Get every byte of the 32bit sprite
    sprite_byte0 = spHandl->spriteId;
    sprite_byte1 = (spHandl->spriteId) >> 8;
    sprite_byte2 = (spHandl->spriteId) >> 16;
    sprite_byte3 = (spHandl->spriteId) >> 24;

    // Create all possible combinations of placement in a 32bit word
    for (i = 0; i < 16; i++)
    {
        spHandl->line0[i] = sprite_byte0 << (2 * i);
        spHandl->line1[i] = sprite_byte1 << (2 * i);
        spHandl->line2[i] = sprite_byte2 << (2 * i);
        spHandl->line3[i] = sprite_byte3 << (2 * i);
    }

    // Create all combinations of placement in the second word if
    // pixel is at the boundary between words
    for (i = 1; i < 4; i++)
    {
        spHandl->line0_rem[i] = sprite_byte0 >> (2 * i);
        spHandl->line1_rem[i] = sprite_byte1 >> (2 * i);
        spHandl->line2_rem[i] = sprite_byte2 >> (2 * i);
        spHandl->line3_rem[i] = sprite_byte3 >> (2 * i);
    }
}

unsigned int LcdOverlaySpriteFlip( unsigned int sprite)    // only if not generating sprite with excell sheet
{
    SpriteHandle spHandl;
    unsigned int tmp_sprite;
    spHandl.spriteId = sprite;

    // Take the 32bit sprite and for each byte reverse the 2bit pixels' order.
    // Namely, for any given byte, [bb3 bb2 bb1 bb0] becomes [bb0 bb1 bb2 bb3]
    // This is due to the way the LCD controller works, considering endianness at a sub-byte level
    tmp_sprite = ((spHandl.spriteId>> 6) & 0x03) | ((spHandl.spriteId >> 2) & 0x0C) | ((spHandl.spriteId << 2) & 0x30) | ((spHandl.spriteId << 6) & 0xC0) |
            ((spHandl.spriteId >> 6) & 0x0300) | ((spHandl.spriteId >> 2) & 0x0C00) | ((spHandl.spriteId << 2) & 0x3000) | ((spHandl.spriteId << 6) & 0xC000) |
            ((spHandl.spriteId >> 6) & 0x030000) | ((spHandl.spriteId >> 2) & 0x0C0000) | ((spHandl.spriteId << 2) & 0x300000) | ((spHandl.spriteId << 6) & 0xC00000) |
            ((spHandl.spriteId >> 6) & 0x03000000) | ((spHandl.spriteId >> 2) & 0x0C000000) | ((spHandl.spriteId << 2) & 0x30000000) | ((spHandl.spriteId << 6) & 0xC0000000);
    return tmp_sprite;

}

void LcdOverlayClearGraphics(unsigned int * overlay_base, unsigned int n_words, unsigned char color)
{
    unsigned int fill_color = 0;
    unsigned int i;

    // generate a 32bit fill color pattern from the color argument
    color = color | (color << 2) | (color << 4) | (color << 6);
    fill_color = fill_color | color | (color << 8) | (color << 16) | (color << 24);

    //u32_memset((u32*)overlay_base, n_bytes, fill_color);
    for (i = 0; i < n_words; i++)
        overlay_base[i] = fill_color;
}

void LcdOverlayClearLines(unsigned int y_loc, unsigned int lines)
{
    unsigned int i;

    unsigned int *overlay_base;

    // Gets address of the graphics buffer starting from the line passed as a parameter.
    overlay_base = &Graphics->p1[(y_loc * (Graphics->spec.width)) >> 4];

    // Fill the desired number of lines with the transparent color
    for (i = 0; i < ((lines * (Graphics->spec.width)) >> 4); i++)
        overlay_base[i] = TRANSPARENT;
}

void LcdOverlayClear()
{
    // Higher level clear function.
    LcdOverlayClearGraphics((unsigned int*)Graphics->p1, glSize, 0);
}

void LcdOverlayWrite2BppSprite4x4(SpriteHandle *spHandl, unsigned int * overlay_base, unsigned short x_loc, unsigned short y_loc)
{
    unsigned int spNum = 32;
    unsigned char fine_x;
    // center x location in sprite's (1,1) pixel (not applied in margin cases)
    if(x_loc > 0 && x_loc < (Graphics->spec.width - 1))
        x_loc -= 1;
    // center y location in sprite's (1,1) pixel (not applied in margin cases)
    if(y_loc > 0 && y_loc < (Graphics->spec.height - 1))
        y_loc -= 1;

    // verify if a sprite byte falls into a single word
    fine_x = x_loc % 16;

    // write the corresponding sprite line (generated, depending on fine_x) to the (x,y) location
    overlay_base[(((y_loc + 0) * Graphics->spec.width+ x_loc) >> 4)] |= spHandl->line3[fine_x];
    overlay_base[(((y_loc + 1) * Graphics->spec.width + x_loc) >> 4)] |= spHandl->line2[fine_x];
    overlay_base[(((y_loc + 2) * Graphics->spec.width + x_loc) >> 4)] |= spHandl->line1[fine_x];
    overlay_base[(((y_loc + 3) * Graphics->spec.width + x_loc) >> 4)] |= spHandl->line0[fine_x];

    // if the sprite falls on a word boundary, write the second word with the corresponding
    // line that is also generated and depending on fine_x
    if (fine_x > 12 && x_loc < (Graphics->spec.width - 1))
    {
        overlay_base[(((y_loc + 0) * Graphics->spec.width + x_loc) >> 4) + 1] |= spHandl->line3_rem[16 - fine_x];
        overlay_base[(((y_loc + 1) * Graphics->spec.width + x_loc) >> 4) + 1] |= spHandl->line2_rem[16 - fine_x];
        overlay_base[(((y_loc + 2) * Graphics->spec.width + x_loc) >> 4) + 1] |= spHandl->line1_rem[16 - fine_x];
        overlay_base[(((y_loc + 3) * Graphics->spec.width + x_loc) >> 4) + 1] |= spHandl->line0_rem[16 - fine_x];
    }

}

void LcdOverlayWriteSpriteList(unsigned int (*point_List), unsigned int pointCount)
{
    unsigned int i;
    SpriteHandle spHandl;

    // Draw each sprite using coordinates list we get from caller
    for (i = 0; i < pointCount; i++)
    {
        LcdOverlayWrite2BppSprite4x4(&spHandl, (unsigned int *)Graphics->p1, (unsigned short)point_List[i] & 0xFFFF, (unsigned short)(point_List[i] >> 16) & 0xFFFF);
    }
}


unsigned int* LcdOverlayGetColorTable()
{
    return osdColorTable;
}
unsigned int LcdOverlayGetLengthColorTable()
{
    return sizeof(osdColorTable);
}

