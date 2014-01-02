///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Performs a look-up table transform of an array.
///

#ifndef __LOOKUP_TABLE_H__
#define __LOOKUP_TABLE_H__

// 1: Includes
// ----------------------------------------------------------------------------
#include <mv_types.h>

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

#ifdef __cplusplus
namespace mvcv{
#endif

#ifdef __PC__

    #define LUT8to8_asm LookupTable
#endif

#ifdef __PC__

    #define LUT12to16_asm LUT12to16
#endif

#ifdef __PC__

    #define LUT10to16_asm LUT10to16
#endif

#ifdef __PC__

    #define LUT10to8_asm LUT10to8
#endif

#ifdef __PC__

    #define LUT12to8_asm LUT12to8
#endif

//!@{
///Performs a look-up table transform of a line. The function fills the destination line with values from the look-up table. Indices of the entries are taken from the source line
///@param[in]  src    - Pointer to input line
///@param[out] dest   - Pointer to output line
///@param[in]  lut    - Look-up table of 256 elements; should have the same depth as the input line. 
/// In the case of multi-channel source and destination lines, the table should either have a single-channel 
///(in this case the same table is used for all channels) or the same number of channels as the source/destination line.
///@param[in]  width  - width of input line
///@param[in]  height - the number of lines (defaulted to one line)

#ifdef __MOVICOMPILE__
    extern "C"
    void LUT8to8_asm(u8** src, u8** dest, const u8* lut, u32 width, u32 height);
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void LUT12to16_asm(u16** src, u16** dest, const u16* lut, u32 width, u32 height);
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void LUT10to16_asm(u16** src, u16** dest, const u16* lut, u32 width, u32 height);
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void LUT10to8_asm(u16** src, u8** dest, const u8* lut, u32 width, u32 height);
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void LUT12to8_asm(u16** src, u8** dest, const u8* lut, u32 width, u32 height);
#endif

void LUT8to8(u8** src, u8** dest, const u8* lut, u32 width, u32 height);

void LUT12to16(u16** src, u16** dest, const u16* lut, u32 width, u32 height);

void LUT10to16(u16** src, u16** dest, const u16* lut, u32 width, u32 height);

void LUT10to8(u16** src, u8** dest, const u8* lut, u32 width, u32 height);

void LUT12to8(u16** src, u8** dest, const u8* lut, u32 width, u32 height);
//!@}
#ifdef __cplusplus
}
#endif

#endif
// 4: inline function implementations
// ----------------------------------------------------------------------------
