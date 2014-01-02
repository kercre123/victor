///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @addtogroup OsdGraphical OsdGraphical component
/// @{
/// @brief BMP parsing API

#ifndef _BMP_PARSER_API_H_
#define _BMP_PARSER_API_H_

#include "swcFrameTypes.h"
// 1: Includes
// ----------------------------------------------------------------------------

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Function that parses the bmp file
/// @param[in] bmp pointer to the bmp file that needs to be parsed
/// @param[in] frBuf framebuffer*
/// @param[in] bmpOffset offset of the bmpfile
/// @param[in] nrColors number of colors from color table
/// @param[in] bmpColorPalette pointer to color palette of the bmp file
/// @return    void
void parseBmpFile(unsigned char *bmp, frameBuffer* frBuf, unsigned int *bmpOffset, unsigned int *nrColors, unsigned int *bmpColorPalette);
/// @}
#endif /* _BMP_PARSER_API_H_ */
