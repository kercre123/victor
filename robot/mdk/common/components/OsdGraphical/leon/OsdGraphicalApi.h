///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup OsdGraphical
/// @{
/// @brief     Simple OSD Functionality.

#ifndef _OSD_GRAPHICAL_API_H_
#define _OSD_GRAPHICAL_API_H_

#include "swcFrameTypes.h"
// 1: Defines
// ----------------------------------------------------------------------------

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

// 4:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 5:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

// For very simple function parameters the following simple style of documentation is sufficent
// The @a parameter highlights the following word as deserving of highlighting

/// Initializes the Osd layer (clears it)
/// @param[in] frmSpc framebuffer*
/// @return    void
void OsdGraphicalClearBuffer(frameBuffer* frmSpc);

/// Puts 2 coloured dots on the OSD
/// @param[in] x X coordinate to place dots
/// @param[in] y Y coordinate to place dots
/// @param[in] color  color of the dots
/// @param[in] frame
/// @return    void
void OsdGraphicalPut2Dots(unsigned int x, unsigned int y, unsigned int color, frameBuffer* frame);

/// Loads a xpm file into the OSD layer, setting transparency
/// @param[in] xpmPalette  a xpm_palette structure from a Movidius X BitMap file
/// @param[in] xpmImage  a xpm structure from any X BitMap file
/// @param[in] semitransparentColor  the index in the X BitMap file to make semitransparent
/// @param[in] frame
/// @return    void
void OsdGraphicalLoadxpm(unsigned int* xpmPalette, unsigned char* xpmImage,
        unsigned int semitransparentColor, frameBuffer* frame);

/// Loads a bmp file into the OSD layer, setting transparency
/// @param[in] OSDcolorPalette  a color palette structure from a bmpFile
/// @param[in] bmpImage  the structure from any bmpFile
/// @param[in] semitransparentColor  the index in bmpFile to make semitransparent
/// @param[in] bmpOffset  Offset of the bmpFile
/// @param[in] frame
/// @return    void
void OsdGraphicalLoadbmp(unsigned int* OSDcolorPalette, unsigned char* bmpImage,
        unsigned int semitransparentColor, unsigned int bmpOffset, frameBuffer* frame);
/// @}
#endif /* _OSD_GRAPHICAL_API_H_ */
