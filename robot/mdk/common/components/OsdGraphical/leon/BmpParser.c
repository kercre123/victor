///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief

// 1: Includes
// ----------------------------------------------------------------------------
#include "stdio.h"
#include "BmpParserApi.h"
#include "OsdGraphicalApi.h"
#include "assert.h"
#include "string.h"

// 2:  Source Specific #defines and types  (typedef, enum, struct)
// ----------------------------------------------------------------------------
typedef struct
{
    unsigned short bmptype;                /* Magic identifier - should be 'BM' if valid BMPfile */
    unsigned int   bmpsize;                /* File size in bytes                                 */
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int   offset;                 /* actual nr.of bytes until the actual data           */
} BMPFILEHEADER;

typedef struct
{
    unsigned int   size;                /* Header size in bytes                                  */
    int            width;               /* Width of the image                                    */
    int            height;              /* Height of the image                                   */
    unsigned short planes;              /* Number of color planes                                */
    unsigned short bits;                /* Bits per pixel = 4(16 colors)                         */
    unsigned int   compression;         /* Compression type - No compression, in our case ='0'   */
    unsigned int   imagesize;           /* Image size in bytes                                   */
    int            xresolution;         /* PixechangeShortEndianess per meter                    */
    int            yresolution;         /* PixechangeShortEndianess per meter                    */
    unsigned int   ncolors;             /* Number of colors                                      */
    unsigned int   importantcolors;     /* Important colors                                      */
} BMPINFOHEADER;

typedef struct
{
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char aB;                    /* 3B for RGB colors, 1B reserved for a_blending        */
} COLORPALETTE;

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

unsigned short changeShortEndianess(unsigned short x)
{
    unsigned short y;
    y = ((x >> 8) & 0xFF) | ((x << 8) & 0xFF00);
    return y;
}

unsigned int changeIntEndianess (unsigned int x)
{
    unsigned int y;
    y = ((x >> 24) & 0xFF) | (((x >> 16 ) & 0xFF) << 8) | (((x >> 8) & 0xff) << 16) | ((x & 0xFF) << 24);
    return y;
}

void parseBmpFile(unsigned char *bmp, frameBuffer* frBuf, unsigned int *bmpOffset, unsigned int* nrColors, unsigned int *bmpColorPalette)
{
	BMPFILEHEADER bmpHeader;
	BMPINFOHEADER bmpInfoHeader;

    unsigned int i;
    unsigned int c;
    int colorTable ;
    unsigned int size;

    memcpy(&bmpHeader.bmptype, bmp, sizeof(unsigned short));

    assert((changeShortEndianess(bmpHeader.bmptype) != 19778) && "The file is not a bmpFile");

    bmp = bmp + sizeof(unsigned short);

    memcpy(&bmpHeader.bmpsize, bmp, sizeof(unsigned int));

    bmp = bmp + sizeof(unsigned int);

    memcpy(&bmpHeader.reserved1, bmp, sizeof(unsigned short));

    bmp = bmp + sizeof(unsigned short);

    memcpy(&bmpHeader.reserved2, bmp, sizeof(unsigned short));

    bmp = bmp + sizeof(unsigned short);

    memcpy(&bmpHeader.offset, bmp, sizeof(unsigned int));
    *bmpOffset = changeIntEndianess(bmpHeader.offset);

    bmp = bmp + sizeof(unsigned int);

    memcpy(&bmpInfoHeader.size, bmp, sizeof(unsigned int));

    bmp = bmp + sizeof(unsigned int);

    memcpy(&bmpInfoHeader.width, bmp, sizeof(int));
    frBuf->spec.width = changeIntEndianess(bmpInfoHeader.width);

    bmp = bmp + sizeof(int);

    memcpy(&bmpInfoHeader.height, bmp, sizeof(int));
    frBuf->spec.height = changeIntEndianess(bmpInfoHeader.height);

    bmp = bmp + sizeof(int);

    memcpy(&bmpInfoHeader.planes, bmp, sizeof(unsigned short));

    bmp = bmp + sizeof(unsigned short);

    memcpy(&bmpInfoHeader.bits, bmp, sizeof(unsigned short));

    bmp = bmp + sizeof(unsigned short);

    memcpy(&bmpInfoHeader.compression, bmp, sizeof(unsigned int));

    bmp = bmp + sizeof(unsigned int);

    memcpy(&bmpInfoHeader.imagesize, bmp, sizeof(unsigned int));

    bmp = bmp + sizeof(unsigned int);

    memcpy(&bmpInfoHeader.xresolution, bmp, sizeof(int));

    bmp = bmp + sizeof(int);

    memcpy(&bmpInfoHeader.yresolution, bmp, sizeof(int));

    bmp = bmp + sizeof(int);

    memcpy(&bmpInfoHeader.ncolors, bmp, sizeof(unsigned int));
    *nrColors = changeIntEndianess(bmpInfoHeader.ncolors);

    assert((changeShortEndianess(bmpInfoHeader.ncolors) > 16) && "Number of colors isn`t equal to 16");

    bmp = bmp + sizeof(unsigned int);

    memcpy(&bmpInfoHeader.importantcolors, bmp, sizeof(unsigned int));

    size = (sizeof(unsigned int) * changeIntEndianess(bmpInfoHeader.ncolors));
    bmp = bmp + sizeof(unsigned int);
    bmp = bmp + sizeof(unsigned int);
    bmp = bmp + size;//skipping the unnecessary info provided and reaching directly to colorPalette

    memcpy(bmpColorPalette, bmp, size);

    for(i = 0; i < *nrColors ; i++)
    {
        colorTable = bmpColorPalette[i];
        bmpColorPalette[i] = (((colorTable & 0xFF) << 24) | (((colorTable >> 24) & 0xFF) << 16) | (((colorTable >> 16)& 0xFF) << 8 ) | ((colorTable >> 8 ) & 0xFF));
        bmpColorPalette[i] = (bmpColorPalette[i] | (0xFF000000)); // | 0xFF is done for changing the aB, which if 0, image would be transparent
    }

    bmp = bmp + (sizeof(int)* changeIntEndianess(bmpInfoHeader.ncolors));
    memcpy(frBuf->p1, bmp, changeIntEndianess(bmpInfoHeader.imagesize));

    return;
}



