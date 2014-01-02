///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup LcdOverlay LCD Overlay component
/// @{
/// @brief    LCD Overlay API.
///
/// This is the API for LCD overlay layer
///

#ifndef _LCD_OVERLAY_API_H_
#define _LCD_OVERLAY_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "LcdOverlayApiDefines.h"
#include "swcFrameTypes.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
/// Clear the overlay buffer
/// @param  overlay_base  pointer to overlay base frame top left corner (must be aligned to 4 bytes).
/// @param  n_bytes       number of bytes (not pixels!) to set.
/// @param  color         color to use for text valid values are 0,1,2,3 as we only have 2 bits/pixel
/// @return    void
///
void LcdOverlayClearGraphics(unsigned int * overlay_base, unsigned int n_bytes, unsigned char color);

/// write a sprite to any location in the frame buffer
/// @param  spHandl  pointer to sprite`s Handler
/// @param  overlay_base  pointer to overlay base frame top left corner
/// @param  x_loc         x_location in pixels (** no multile-of-anything restriction here **)
/// @param  y_loc         x_location in pixels (** no multile-of-anything restriction here **)
/// @return    void
///
void LcdOverlayWrite2BppSprite4x4(SpriteHandle *spHandl, unsigned int * overlay_base, unsigned short x_loc, unsigned short y_loc);

/// Write a sprite array to to coordinates in the frame buffer
/// @param point_List points array  pointer to the points coordinates array
/// @param pointCount points count  number of points
/// @return    void
///
void LcdOverlayWriteSpriteList(unsigned int (*point_List), unsigned int pointCount);

/// Higher level clear function
///
void LcdOverlayClear();

/// Higher level clear function for a slice of the layer
/// @param  y_loc  pointer to line from where we want to start clearing
/// @param  lines  number of lines to clear
/// @return    void
///
void LcdOverlayClearLines(unsigned int y_loc, unsigned int lines);

/// LcdOverlay sprite flip function - won't be needed when we implement the Excell sprite generator
/// @param  sprite  sprite on which we do the sub byte endianness inversion
/// @return    void
///
unsigned int LcdOverlaySpriteFlip( unsigned int sprite);

/// LcdOverlay sprite init needed to generate 32bit sprite lines
/// @param  spHandl  pointer to sprite`s Handler
/// @param  sprite   sprite we use to generate the possible combinations
/// @return    void
///
void LcdOverlaySpriteInit(SpriteHandle *spHandl, unsigned int sprite);

/// Init the sprite display LcdOverlay by doing sprite initialization and clearing the buffer
/// @param  overlayFrame  pointer to frameBuffer
/// @param  size size of framebuffer
/// @return    void
///
void LcdOverlayInit(frameBuffer *overlayFrame, unsigned int size);


/// Dummy function to test stuff
///
void LcdOverlayTest();

/// Gets pointer to the color table
/// @return pointer to the color table
///
unsigned int* LcdOverlayGetColorTable();

/// Gets pointer to the color table
/// @return number of colors from color table
///
unsigned int LcdOverlayGetLengthColorTable();
/// @}
#endif
