
#include <mv_types.h>
#include <LookupTable.h>
//#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
namespace mvcv{
#endif

void LUT8to8(u8** src, u8** dest, const u8* lut, u32 width, u32 height)
{
    int i, j;
    u8* in_line  = *src;
    u8* out_line = *dest;
    for (j = 0; j < width; j++)
    {
    	out_line[j] = lut[in_line[j]];
    }
}

void LUT12to16(u16** src, u16** dest, const u16* lut, u32 width, u32 height)
{
    int i, j;
    u16* in_line = *src;
    u16* out_line = *dest;

    for (j = 0; j < width; j++)
    {
        out_line[j] = lut[in_line[j]&0x0FFF];
    }

}


void LUT12to8(u16** src, u8** dest, const u8* lut, u32 width, u32 height)
{
    int i, j;
    u16* in_line = *src;
    u8* out_line = *dest;

    for (j = 0; j < width; j++)
    {
        out_line[j] = lut[in_line[j]&0x0FFF];
    }

}


void LUT10to16(u16** src, u16** dest, const u16* lut, u32 width, u32 height)
{
    int i, j;
    u16* in_line = *src;
    u16* out_line = *dest;
    for (j = 0; j < width; j++)
    {
    	out_line[j] = lut[in_line[j]&0x03FF];
    }
}

void LUT10to8(u16** src, u8** dest, const u8* lut, u32 width, u32 height)
{
	int i, j;
	u16* in_line = *src;
	u8* out_line = *dest;
	for (j = 0; j < width; j++)
	{
		out_line[j] = lut[in_line[j]&0x03FF];
	}
}

#ifdef __cplusplus
}
#endif
