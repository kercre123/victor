///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Frametypes used in Myriad common code
///

#ifndef _SWC_FRAME_TYPES_H_
#define _SWC_FRAME_TYPES_H_

typedef enum frameTypes
{
     YUV422i,   // interleaved 8 bit
     YUV420p,   // planar 4:2:0 format
     YUV422p,   // planar 8 bit
     YUV400p,   // 8-bit greyscale
     RGBA8888,  // RGBA interleaved stored in 32 bit word
     RGB888,    // Planar 8 bit RGB data
     LUT2,      // 1 bit  per pixel, Lookup table (used for graphics layers)
     LUT4,      // 2 bits per pixel, Lookup table (used for graphics layers)
     LUT16,     // 4 bits per pixel, Lookup table (used for graphics layers)
     RAW16,     // save any raw type (8, 10, 12bit) on 16 bits
     NONE
}frameType;

typedef enum planeTypes
{
    Y,     // Luminance
    U,     // U-Chroma
    V,     // U-Chroma
    R,     // Red
    G,     // Green
    B,     // Blue
    A      // Alpha/Opacity
}swcPlaneType;

typedef struct frameSpecs
{
     frameType      type;
     unsigned int   height;    // width in pixels
     unsigned int   width;     // width in pixels
     unsigned int   stride;    // defined as distance in bytes from pix(y,x) to pix(y+1,x)
     unsigned int	bytesPP;   // bytes per pixel (for LUT types set this to 1)
}frameSpec;


typedef struct frameElements
{
     frameSpec spec;
     unsigned char* p1;  // Pointer to first image plane
     unsigned char* p2;  // Pointer to second image plane (if used)
     unsigned char* p3;  // Pointer to third image plane  (if used)
} frameBuffer;

#endif // _SWC_FRAME_TYPES_H_
