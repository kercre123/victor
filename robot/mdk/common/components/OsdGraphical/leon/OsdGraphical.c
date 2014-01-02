///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Simple OSD Functionality.
/// 
/// We can continue this brief description of the module to add more detail
/// This can actually include quite a long description if necessary.
/// 
/// @todo It can be useful to keep a list of outstanding tasks for your module
/// @todo You can have more than one item on your todo list
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------

#include "stdio.h"
#include "BmpParserApi.h"
#include "OsdGraphicalApi.h"
#include "OsdGraphicalPrivateDefines.h"
#include "DrvSvu.h"
#include "DrvTimer.h"

// 2:  Source Specific #defines and types  (typedef, enum, struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

//16 colors palette
/*static unsigned int OSD_GRAPHICAL_DATA_SECTION(OSDColorPalette)[16]={
        //  0xAABBGGRR      R,  G,  B
          White    0xFFFFFFFF,   255,255,255
            Yellow   0xFF00FFFF,   255,255,0
            Fuchsia     0xFFFF00FF,     255,0,255
            Red         0xFF0000FF,   255,0, 0
            Silver     0xFFC0C0C0,   192,192,192
            Gray     0xFF808080,   128,128,128
            Olive     0xFF008080,   128,128,0
            Purple     0xFF800080,   128,0,128
            Maroon     0xFF000080,   128,0,0
            Aqua     0xFFFFFF00,   0,255,255
            Lime     0xFF00FF00,   0,255,0
            Teal     0xFF808000,   0,128,128
            Green     0xFF008000,   0,128,0
            Blue     0xFFA00000,   0,0,255
            Navy     0xFF800000,   0,0,128
            Black     0xFF000000    0,0,0
};*/

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

void OSD_GRAPHICAL_CODE_SECTION(OsdGraphicalClearBuffer)(frameBuffer* frmSpc)
{
    unsigned int x, y;
    for (x = 0; x < (((frmSpc->spec.width) >> 1) * (frmSpc->spec.height)); x++)
    {
        //Fill with black which is set as the transparent color
        frmSpc->p1[x] = (15 << 4) | 15;
    }
    return;
}

void OSD_GRAPHICAL_CODE_SECTION(OsdGraphicalPutDot)(unsigned int x, unsigned int y, unsigned int color, frameBuffer* frame)
{
    if ((x & 0x1) == 0)
    {
        frame->p1[((y * (frame->spec.width)/2) + x/2)] = (frame->p1[((y * (frame->spec.width)/2) + x/2)] & 0xF0) | color;
    }else
    {
        frame->p1[((y * (frame->spec.width)/2) + x/2)] = (frame->p1[((y * (frame->spec.width)/2) + x/2)] & 0x0F) | (color << 4);
    }
    return;
}

void OSD_GRAPHICAL_CODE_SECTION(OsdGraphicalPut2Dots)(unsigned int x, unsigned int y, unsigned int color, frameBuffer* frame)
{
    frame->p1[((y * (frame->spec.width))/2 + x/2)] = color;
    return;
}

void OSD_GRAPHICAL_CODE_SECTION(OsdGraphicalLoadxpm)(unsigned int* xpmPalette, unsigned char* xpmImage,
        unsigned int semitransparentColor, frameBuffer* frame)
{
    unsigned int x, y;
    //Set palette color transparency
    xpmPalette[semitransparentColor] = (xpmPalette[semitransparentColor] & 0x00FFFFFF) | 0x80000000;
    //Fill in the structure dot by dot
    for (y = 0; y < frame->spec.height; y++)
    {
        for (x = 0; x < frame->spec.width; x++)
        {
            //Putting 2 dots at once since that's a byte and it's faster this way
            OsdGraphicalPut2Dots(x, y, xpmImage[y * (frame->spec.width)/2 + x/2], frame);
        }
    }
    return;
}

void OSD_GRAPHICAL_CODE_SECTION(OsdGraphicalLoadbmp)(unsigned int* OSDColorPalette, unsigned char* bmpImage,
        unsigned int semitransparentColor, unsigned int bmpOffset, frameBuffer* frame)
{
    tyTimeStamp executionTimer;
    u64 cycles64;
    unsigned int x, y;

    OSDColorPalette[semitransparentColor] = (OSDColorPalette[semitransparentColor] & 0x00FFFFFF) | 0x80000000;

    DrvTimerStart(&executionTimer);
    for (y = frame->spec.height; y > 0 ; y--)
    {
        for (x = 0; x < frame->spec.width; x++)
        {
            //Putting 2 dots at once since that's a byte and it's faster this way
            OsdGraphicalPut2Dots(x, (frame->spec.height - y),
                    (((bmpImage[(y - 1) * (frame->spec.width)/2 + x/2 + bmpOffset]) & 0xF0) >> 4) |
                    (((bmpImage[(y - 1) * (frame->spec.width)/2 + x/2 + bmpOffset]) & 0x0F) << 4), frame);
        }
    }
    cycles64 = DrvTimerElapsedTicks(&executionTimer);
    printf("Exec cycles of OsdGraphicalPut2Dots: %d\n", (u32)cycles64);
    printf("Exec uS of OsdGraphicalPut2Dots: %d\n", (u32)(DrvTimerTicksToMs(cycles64)*1000));
    printf("Exec mS of OsdGraphicalPut2Dots: %d\n", (u32)(DrvTimerTicksToMs(cycles64)));
    return;
}
