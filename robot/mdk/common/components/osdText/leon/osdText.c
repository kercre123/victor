/*!     \file OsdText.c
        \brief Incorporate all the functionality for write OSD texts

        Author      : Marius Truica
        Date        : 05-sept-2012
        E-mail      : marius.truica@movidius.com
        Copyright   :  Movidius Srl 2012,  Movidius Ltd 2012
*/

/****************************************************************************/
/*!    1: Included the APIs headers file */
/******************************************************************************/
#include "osdTextApi.h"
#include "string.h"

/****************************************************************************/
/*! 2:  Source File Specific #defines */
/****************************************************************************/



/*!    settable values , can  be changed, depend by your desired. Used as default values */
#define _OsdTextNumOffGliphsPerLine_        (60u)             /**< represent the number of characters that can be represent on horizontally */
#define _OsdTextNumOffLines_                (8u)              /**< represent the number of text lines that can be represent vertically */
#define _OsdTextLinesVerticalStride_        (0u)              /**< represent the number of pixels between 2 text lines */
#define _OsdTextXLayerPosition_             (10u)             /**< Lcd text layer position X up corner coordinate */
#define _OsdTextYLayerPosition_             (590u)            /**< Lcd text layer position Y up corner coordinate */
#define _OsdTextTextColor_                  (0x00300000)      /**< text Color */
#define _OsdTextFondColor_                  (0x00EEEEEE)      /**< Fond Color */
#define _OsdTextMakeFondTransparent_        (1u)              /**< 1 for make it transparent , 0 for make it visible */


/*!       \briefThis define decide what memory section will be use for glyph lib
        Glyph lib for this project have 1536b .
        contain all Ascii characters between 0x20 to 0x7E in 8x16 size, 1bpp
*/
#ifndef MEM_SECTION_GLIPHS
#define MEM_SECTION_GLIPHS __attribute__((section(".osdTextGlyph.data")))
#endif

// useful for generate different section for every function
#define SECTION1_NAME(x,name) name##x
#define SECTION1_NAMEs(y) #y
#define SECTION1_NAMEs2(j) SECTION1_NAMEs(j)

//Osd Text Module Sections
    #define SECT_OsdText_FUNCTION(x) __attribute__((section(SECTION1_NAMEs2(SECTION1_NAME(x,.sectCode.osdText.fct))))) x
    #define SECT_OsdText_DATA(x) __attribute__((section(SECTION1_NAMEs2(SECTION1_NAME(x,.sectData.osdText.var))))) x
    #define SECT_OsdText_FUNCTION2(x) __attribute__((section(SECTION1_NAMEs2(SECTION1_NAME(x,.sectCode.osdText1.fct))))) x
    #define SECT_OsdText_DATA2(x) __attribute__((section(SECTION1_NAMEs2(SECTION1_NAME(x,.ddr.sectData.osdText1.var))))) x



/*!    Hard-coded values, can not be changed, depend by the libs structure and module capability */
#define _OsdTextGliphsWidth_            (8u)      /**< Glyph width */
#define _OsdTextGliphsHeight_           (16u)     /**< Glyph height */
#define _OsdTextBpp_                    (1u)      /**< Glyph bit per pixel */
#define _OsdTextFirstAsciiGliphs_       (0x20)    /**< First accepted Glyph index from Ascii table  */
#define _OsdTextLastAsciiGliphs_        (0x7E)    /**< Last accepted Glyph index from Ascii table */

/*!    values calculated from parameters define */
#define _OsdTextNumberOffGlips_     (_OsdTextLastAsciiGliphs_ - _OsdTextFirstAsciiGliphs_ + 2)

#define  PAD_RIGHT   1
#define  PAD_ZERO    2

/*! the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12


/******************************************************************************/
/*! 3:  Local variables */
/******************************************************************************/
///@{

/*!     \var static u8 osdTextGliphs[_OsdTextNumberOffGlips_][_OsdTextGliphsHeight_]
        \brief This are the lib for Glyph, in 8x16 formats for 0x20-0x7E Ascii characters.
*/
static u8 MEM_SECTION_GLIPHS osdTextGliphs[_OsdTextNumberOffGlips_][_OsdTextGliphsHeight_] = 
{
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0, 0x10, 0x10, 0x0, 0x0 },
    { 0x0, 0x0, 0x44, 0x44, 0x44, 0x44, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x48, 0x48, 0x48, 0x7e, 0x24, 0x24, 0x7e, 0x12, 0x12, 0x12, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x10, 0x7c, 0x92, 0x12, 0x1c, 0x70, 0x90, 0x92, 0x7c, 0x10, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x8c, 0x52, 0x52, 0x2c, 0x10, 0x10, 0x68, 0x94, 0x94, 0x62, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x38, 0x44, 0x44, 0x44, 0x38, 0x9c, 0xa2, 0x42, 0x62, 0x9c, 0x0, 0x0 },
    { 0x0, 0x0, 0x10, 0x10, 0x10, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x20, 0x10, 0x10, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x10, 0x10, 0x20, 0x0 },
    { 0x0, 0x0, 0x0, 0x4, 0x8, 0x8, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x8, 0x8, 0x4, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10, 0x92, 0x54, 0x38, 0x54, 0x92, 0x10, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10, 0x10, 0x10, 0xfe, 0x10, 0x10, 0x10, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x18, 0x10, 0x10, 0x8 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7e, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x18, 0x18, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x40, 0x40, 0x20, 0x10, 0x10, 0x8, 0x8, 0x4, 0x2, 0x2, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x18, 0x24, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x24, 0x18, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x10, 0x18, 0x14, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x42, 0x40, 0x30, 0x8, 0x4, 0x2, 0x2, 0x7e, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x42, 0x40, 0x38, 0x40, 0x40, 0x42, 0x42, 0x3c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x20, 0x30, 0x28, 0x24, 0x22, 0x22, 0x7e, 0x20, 0x20, 0x20, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x7e, 0x2, 0x2, 0x2, 0x3e, 0x40, 0x40, 0x40, 0x42, 0x3c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x38, 0x4, 0x2, 0x2, 0x3e, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x7e, 0x40, 0x40, 0x20, 0x20, 0x20, 0x10, 0x10, 0x10, 0x10, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x42, 0x42, 0x3c, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x42, 0x42, 0x7c, 0x40, 0x40, 0x40, 0x20, 0x1c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x18, 0x18, 0x0, 0x0, 0x0, 0x18, 0x18, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x18, 0x18, 0x0, 0x0, 0x0, 0x18, 0x10, 0x10, 0x8, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x40, 0x20, 0x10, 0x8, 0x4, 0x8, 0x10, 0x20, 0x40, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7e, 0x0, 0x0, 0x0, 0x7e, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x4, 0x8, 0x10, 0x20, 0x10, 0x8, 0x4, 0x2, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x42, 0x40, 0x20, 0x10, 0x10, 0x0, 0x10, 0x10, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x38, 0x44, 0x52, 0x6a, 0x4a, 0x4a, 0x4a, 0x72, 0x4, 0x78, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x18, 0x24, 0x24, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x42, 0x42, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x3e, 0x42, 0x42, 0x42, 0x3e, 0x42, 0x42, 0x42, 0x42, 0x3e, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x42, 0x2, 0x2, 0x2, 0x2, 0x42, 0x42, 0x3c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x1e, 0x22, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x22, 0x1e, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x7e, 0x2, 0x2, 0x2, 0x3e, 0x2, 0x2, 0x2, 0x2, 0x7e, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x7e, 0x2, 0x2, 0x2, 0x3e, 0x2, 0x2, 0x2, 0x2, 0x2, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x42, 0x2, 0x2, 0x72, 0x42, 0x42, 0x62, 0x5c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x42, 0x42, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x42, 0x42, 0x42, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0xf8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x22, 0x22, 0x1c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x42, 0x22, 0x12, 0xa, 0x6, 0x6, 0xa, 0x12, 0x22, 0x42, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x7e, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x42, 0x42, 0x66, 0x66, 0x5a, 0x5a, 0x42, 0x42, 0x42, 0x42, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x42, 0x46, 0x46, 0x4a, 0x4a, 0x52, 0x52, 0x62, 0x62, 0x42, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x3e, 0x42, 0x42, 0x42, 0x3e, 0x2, 0x2, 0x2, 0x2, 0x2, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x5a, 0x66, 0x3c, 0xc0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x3e, 0x42, 0x42, 0x42, 0x3e, 0x12, 0x22, 0x22, 0x42, 0x42, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x42, 0x2, 0xc, 0x30, 0x40, 0x42, 0x42, 0x3c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0xfe, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x82, 0x82, 0x82, 0x44, 0x44, 0x44, 0x28, 0x28, 0x10, 0x10, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x42, 0x42, 0x42, 0x42, 0x5a, 0x5a, 0x66, 0x66, 0x42, 0x42, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x42, 0x42, 0x24, 0x24, 0x18, 0x18, 0x24, 0x24, 0x42, 0x42, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x82, 0x82, 0x44, 0x44, 0x28, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x7e, 0x40, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x2, 0x7e, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x70, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x70, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x2, 0x2, 0x4, 0x8, 0x8, 0x10, 0x10, 0x20, 0x40, 0x40, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0xe, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0xe, 0x0 },
    { 0x0, 0x0, 0x18, 0x24, 0x42, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xfe, 0x0 },
    { 0x0, 0x4, 0x8, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x40, 0x7c, 0x42, 0x42, 0x62, 0x5c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x2, 0x2, 0x2, 0x3a, 0x46, 0x42, 0x42, 0x42, 0x42, 0x46, 0x3a, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x2, 0x2, 0x2, 0x2, 0x42, 0x3c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x40, 0x40, 0x40, 0x5c, 0x62, 0x42, 0x42, 0x42, 0x42, 0x62, 0x5c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x42, 0x7e, 0x2, 0x2, 0x42, 0x3c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x30, 0x8, 0x8, 0x8, 0x3e, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x40, 0x5c, 0x22, 0x22, 0x22, 0x1c, 0x4, 0x3c, 0x42, 0x42, 0x3c },
    { 0x0, 0x0, 0x0, 0x2, 0x2, 0x2, 0x3a, 0x46, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x10, 0x10, 0x0, 0x18, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x20, 0x20, 0x0, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x12, 0xc },
    { 0x0, 0x0, 0x0, 0x0, 0x2, 0x2, 0x22, 0x12, 0xa, 0x6, 0xa, 0x12, 0x22, 0x42, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x18, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x6e, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3a, 0x46, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3a, 0x46, 0x42, 0x42, 0x42, 0x42, 0x46, 0x3a, 0x2, 0x2 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5c, 0x62, 0x42, 0x42, 0x42, 0x42, 0x62, 0x5c, 0x40, 0x40 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3a, 0x46, 0x42, 0x2, 0x2, 0x2, 0x2, 0x2, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x2, 0xc, 0x30, 0x40, 0x42, 0x3c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x8, 0x8, 0x3e, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x30, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x62, 0x5c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x42, 0x42, 0x42, 0x24, 0x24, 0x24, 0x18, 0x18, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x82, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x6c, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x42, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x42, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x42, 0x42, 0x42, 0x42, 0x42, 0x64, 0x58, 0x40, 0x40, 0x3c },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7e, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x7e, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x30, 0x8, 0x8, 0x10, 0x10, 0x8, 0x8, 0x10, 0x10, 0x8, 0x8, 0x30, 0x0 },
    { 0x0, 0x0, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 },
    { 0x0, 0x0, 0x0, 0xc, 0x10, 0x10, 0x8, 0x8, 0x10, 0x10, 0x8, 0x8, 0x10, 0x10, 0xc, 0x0 },
    { 0x0, 0x0, 0x0, 0x8c, 0x92, 0x62, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }
};

/*!     \var static u32 OsdTextColorTable[]
        \brief text color table that will be load on Lcd registers  
*/
static u32 SECT_OsdText_DATA(OsdTextColorTable)[] = { _OsdTextFondColor_, _OsdTextTextColor_ };


/*!     \var static u8  *bufferTextPointAddress
        \brief Text buffer address.
*/
static u8*  SECT_OsdText_DATA(bufferTextPointAddress) = (u8*) (0x00); /// if you don't call setup function will be played nothing

/// all this can be change by setup function
static u32 SECT_OsdText_DATA(osdTextNumOffGliphsPerLine)    = _OsdTextNumOffGliphsPerLine_;    /**< represent the number of characters that can be represent on horizontally */
static u32 SECT_OsdText_DATA(osdTextNumOffLines)            = _OsdTextNumOffLines_;            /**< represent the number of text lines that can be represent vertically */
static u32 SECT_OsdText_DATA(osdTextLinesVerticalStride)    = _OsdTextLinesVerticalStride_;    /**< represent the number of pixels between 2 text lines */
static u32 SECT_OsdText_DATA(osdTextXLayerPosition)         = _OsdTextXLayerPosition_;         /**< Lcd text layer position X up corner coordinate */
static u32 SECT_OsdText_DATA(osdTextYLayerPosition)         = _OsdTextYLayerPosition_;         /**< Lcd text layer position Y up corner coordinate */
static u32 SECT_OsdText_DATA(osdTextTextColor)              = _OsdTextTextColor_;              /**< text Color */
static u32 SECT_OsdText_DATA(osdTextFondColor)              = _OsdTextFondColor_;              /**< Fond Color */
static u32 SECT_OsdText_DATA(osdTextMakeFondTransparent)    = _OsdTextMakeFondTransparent_;    /**< 1 for make it transparent , 0 for make it visible */
/// for internal simplified calculation
static u32  SECT_OsdText_DATA(osdTextLayerWidth)             =(_OsdTextNumOffGliphsPerLine_ * _OsdTextGliphsWidth_ * _OsdTextBpp_);
static u32  SECT_OsdText_DATA(osdTextBytesWidth)             =(_OsdTextNumOffGliphsPerLine_ * _OsdTextBpp_);
static u32  SECT_OsdText_DATA(osdTextLayerHeight)            =(_OsdTextNumOffLines_ * (_OsdTextGliphsHeight_ + _OsdTextLinesVerticalStride_));


static u32 SECT_OsdText_DATA(use_leading_plus) = 0 ;


static char* SECT_OsdText_DATA(curentTextBufferPosition) = (u8*) (0x00);
static u32 SECT_OsdText_DATA(currentPrintfCursorLine) = 0x00;
static u32 SECT_OsdText_DATA(currentPrintfCursorColm) = 0x00;

static double SECT_OsdText_DATA(round_nums)[8] = {
   0.5,
   0.05,
   0.005,
   0.0005,
   0.00005,
   0.000005,
   0.0000005,
   0.00000005
} ;


///@}


static int osdTextprint (char **out, unsigned n, va_list va, const char *format, ... );
static int osdTextisValid(int pc, int n);
static void osdTextprintchar (char **str, char c);
static int osdTextprints (char *   *out, int n, const char *string, int width, int pad);
static int osdTextprinti (char **out, int n, int i, int b, int sg, int width, int pad, int letbase);
static unsigned osdTextdbl2stri(char *outbfr, double dbl, unsigned dec_digits);
static u32 osdTextmy_strlen(char *str);


/******************************************************************************/
/*!    4: Functions Implementation */
/******************************************************************************/
unsigned int* SECT_OsdText_FUNCTION(osdTextGetColorTable) ()
{
    return OsdTextColorTable;
}

/*! Clean all the text Osd layer */
u32 SECT_OsdText_FUNCTION(osdTextCleanAll)(void)
{
    memset ((void*)(bufferTextPointAddress), 0x00, osdTextLayerHeight * osdTextNumOffGliphsPerLine);
    return 1;
}

/*! Clean a specific area from text layer */
u32 SECT_OsdText_FUNCTION(osdTextCleanArea)(u32 lineStartIdx,u32 colStartIdx,u32 noOffLines,u32 noOffCol)
{
    u32 returnVal = 0;
    if (((colStartIdx + noOffCol) > osdTextNumOffGliphsPerLine) | ((lineStartIdx + noOffLines)>osdTextNumOffLines))
    {
        returnVal = 0;
    }
    else
    {
        u32 i, j;
        for (i = 0; i < noOffLines; i++)
        {
            u32 LineStartAddress = (u32)&bufferTextPointAddress[(lineStartIdx+i) * osdTextNumOffGliphsPerLine * _OsdTextGliphsHeight_ + colStartIdx];
            for (j = 0; j < _OsdTextGliphsHeight_; j++)
            {
                memset ((void*)(LineStartAddress), 0x00, noOffCol);
                LineStartAddress += osdTextNumOffGliphsPerLine;
            }
        }
        returnVal = 1;
    }
    return returnVal;
}

/*! Write one text line on the Lcd output text buffer. */
u32 SECT_OsdText_FUNCTION(osdTextWriteText)(u8 *textMessage,u32 lineIdx, u32 colStartIdx)
{
    u8 *pointToTheText = (u8 *)textMessage;
    u32 i = 0;
    u32 j = 0;
    u32 returnVal = 0;

    if ((colStartIdx > osdTextNumOffGliphsPerLine) | (lineIdx>osdTextNumOffLines))
    {
        returnVal = 0;
    }
    else
    {
        while(pointToTheText[i] != 0x00)
        {
            for (j = 0; j < 16; j++)
            {
                bufferTextPointAddress[((lineIdx * _OsdTextGliphsHeight_ +j) * (osdTextNumOffGliphsPerLine)) + colStartIdx + i] = osdTextGliphs[pointToTheText[i]-0x20][j];
            }
            i++;
        }
    }
    return returnVal;
}

/*! Set the text osd properties, in conformity with wanted scenario. */
u32  SECT_OsdText_FUNCTION(osdTextSetup)(frameBuffer * osdTextFrame,
                    u32 numLines,
                    u32 numCharPerLine,
                    u32 lineSpacingPixels,
                    u32 xLayerPossition,
                    u32 yLayerPossition,
                    u32 textColor,
                    u32 fondColor,
                    u32 makeFondTransparent
                )
{
    bufferTextPointAddress         = osdTextFrame->p1;
    osdTextNumOffGliphsPerLine     = numCharPerLine;        /**< represent the number of characters that can be represent on orizontaly */
    osdTextNumOffLines             = numLines;              /**< represent the number of text lines that can be represent verticaly */
    osdTextLinesVerticalStride     = lineSpacingPixels;     /**< represent the number of pixels between 2 text lines */
    osdTextXLayerPosition          = xLayerPossition;       /**< Lcd text layer position X up corner coordonate */
    osdTextYLayerPosition          = yLayerPossition;       /**< Lcd text layer position Y up corner coordonate */
    osdTextTextColor               = textColor;             /**< text Color */
    osdTextFondColor               = fondColor;             /**< Fond Color */
    osdTextMakeFondTransparent     = makeFondTransparent;   /**< 1 for make it transparent , 0 for make it visible */
    OsdTextColorTable[0]           = osdTextFondColor;
    OsdTextColorTable[1]           = osdTextTextColor;

    osdTextLayerWidth              =(osdTextNumOffGliphsPerLine * _OsdTextGliphsWidth_ * _OsdTextBpp_);
    osdTextBytesWidth              =(osdTextNumOffGliphsPerLine * _OsdTextBpp_);
    osdTextLayerHeight             =(osdTextNumOffLines * (_OsdTextGliphsHeight_ + osdTextLinesVerticalStride));
    curentTextBufferPosition    = bufferTextPointAddress;
    currentPrintfCursorLine     = 0x00;
    currentPrintfCursorColm     = 0x00;

    osdTextFrame->spec.height   = osdTextLayerHeight;
    osdTextFrame->spec.width    = osdTextLayerWidth;
    osdTextFrame->spec.stride   = 0;
    osdTextFrame->spec.type     = LUT2;
    osdTextFrame->spec.bytesPP  = 1;
    return (osdTextNumOffGliphsPerLine * osdTextNumOffLines * _OsdTextGliphsHeight_);
}


/*! PRINTF IMPLEMENTATION */

/*!  */
int SECT_OsdText_FUNCTION(osdTextprintf) (const char *format, ...)
{
   int pc;
   va_list va;

 //  isPrintf = 1;

   va_start(va, format);

   pc = osdTextprint (&curentTextBufferPosition, -1, va, format);

   va_end(va);

   return pc;
}

/*!  */
static int SECT_OsdText_FUNCTION(osdTextprint) (char **out, unsigned n, va_list va, const char *format, ... )
{
    int post_decimal ;
    int width = 0;
    int pad ;
    int dec_width = -1 ;
    int pc = 0 ;
    int output ;
    int nchar = (signed)n;

    char scr[2];
    char bfr[81] ;

    char *s;

    double dbl = 0;

    use_leading_plus = 0 ;  //  start out with this clear

    for (; *format != 0; ++format)
    {
//%[flags][width][.precision][length]specifier
        if (*format == '%')                                 //%
        {
            ++format;
            width = pad = 0;
            switch (*format)
            {
                case '\0':                                  //\0 finished
                {
                }
                break;
                case '%':                                   //%% = "%"
                {
                    if( osdTextisValid(pc, nchar))
                    {
                        osdTextprintchar (out, *format);
                    }
                    ++pc;
                }
                break;
                default:
                {
//[flags]
                    if (*format == '-')
                    {
                        ++format;
                        pad = PAD_RIGHT;
                    }
                    if (*format == '+')
                    {
                        ++format;
                        use_leading_plus = 1 ;
                    }
                    while (*format == '0')
                    {
                        ++format;
                        pad |= PAD_ZERO;
                    }
//[width][.precision]
                    post_decimal = 0 ;
                    if (*format == '.'  || (*format >= '0' &&  *format <= '9'))
                    {
                        while (1)
                        {
                            if (*format == '.')
                            {
                                post_decimal = 1 ;
                                dec_width = 0 ;
                                format++ ;
                            }
                            else
                            {
                                if ((*format >= '0' &&  *format <= '9'))
                                {
                                    if (post_decimal)
                                    {
                                        dec_width *= 10;
                                        dec_width += *format - '0';
                                    }
                                    else
                                    {
                                        width *= 10;
                                        width += *format - '0';
                                    }
                                    format++ ;
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
//[length]
                    if ((*format == 'l') || (*format == 'L'))
                    {
                        ++format;
                    }

                    if( (signed)n > 0)
                    {
                        if( ((signed)n - pc) < 0)
                        {
                            nchar = (signed)n - pc;
                        }
                    }
//specifier
                    switch (*format)
                    {
                        case 's':
                        {
                            s = va_arg( va, char *);
                            pc += osdTextprints (out, nchar, s ? s : "(null)", width, pad);
                            use_leading_plus = 0 ;  //  reset this flag after printing one value
                        }
                        break;
                        case 'i':
                        case 'd':
                        {
                            output = va_arg( va, int );
                            pc += osdTextprinti (out, nchar, output, 10, 1, width, pad, 'a');
                            use_leading_plus = 0 ;  //  reset this flag after printing one value
                        }
                        break;
                        case 'x':
                        {
                            output = va_arg( va, int );
                            pc += osdTextprinti (out, nchar, output, 16, 0, width, pad, 'a');
                            use_leading_plus = 0 ;  //  reset this flag after printing one value
                        }
                        break;
                        case 'X':
                        {
                            output = va_arg( va, int );
                            pc += osdTextprinti (out, nchar, output, 16, 0, width, pad, 'A');
                            use_leading_plus = 0 ;  //  reset this flag after printing one value
                        }
                        break;
                        case 'u':
                        {
                            output = va_arg( va, unsigned int );
                            pc += osdTextprinti (out, nchar, output, 10, 0, width, pad, 'a');
                            use_leading_plus = 0 ;  //  reset this flag after printing one value
                        }
                        break;
                        case 'c':
                        {
                            /* char are converted to int then pushed on the stack */
                            scr[0] = (char)va_arg( va, int );
                            scr[1] = '\0';
                            pc += osdTextprints (out, nchar, scr, width, pad);
                            use_leading_plus = 0 ;  //  reset this flag after printing one value
                        }
                        break;
                        case 'f':
                        {
                            dbl = va_arg( va, double);
                            if( -1 == dec_width )
                            dec_width = 6;
                            osdTextdbl2stri(bfr, dbl, dec_width) ;
                            pc += osdTextprints (out, nchar, bfr, width, pad);
                            use_leading_plus = 0 ;  //  reset this flag after printing one value
                            dec_width = -1; //  reset this flag after printing one value
                        }
                        break;
                        case 'p':
                        {
                            output = (int)va_arg( va, int *);
                            pad |= PAD_ZERO;
                            width = 8; // pad with zeros
                            pc += osdTextprinti (out, nchar, output, 16, 0, width, pad, 'A');
                            use_leading_plus = 0 ;
                        }
                        break;
                        case 'o':
                        {
                            output = va_arg( va, int);
                            pc += osdTextprinti (out, nchar, output, 8, 0, width, pad, 'a');
                            use_leading_plus = 0 ;
                        }
                        break;
                        case 'n': break;
                        default:
                        {
                            osdTextprintchar (out, '%');
                            osdTextprintchar (out, *format);
                            use_leading_plus = 0 ;  //  reset this flag after printing one value
                        }
                        break;
                    }
                }
                break;
            }
        }
        else
        {
            if( osdTextisValid(pc, nchar))
            {
                osdTextprintchar (out, *format);
            }
            ++pc;
        }
    }
    return pc;
}

/*!  */
static int SECT_OsdText_FUNCTION(osdTextisValid)(int pc, int n)
{
    return  (((n != 0) && (pc < n)) || (n == -1)); // n == -1 print all, n == 0 print nothing,
                                                   // else print until no of printed characters (pc) >= n
}

/*!  */
static void SECT_OsdText_FUNCTION(osdTextprintchar) (char **str, char c)
{
    int j;

    if (str)
    {
        if (currentPrintfCursorLine == osdTextNumOffLines)
        {
            u32 *copyTo = (u32*)&(bufferTextPointAddress[0]);
            u32 *copyFrom = (u32*)&(bufferTextPointAddress[_OsdTextGliphsHeight_* osdTextNumOffGliphsPerLine]);

            for ( j = 0; j<(((osdTextNumOffLines-1)*_OsdTextGliphsHeight_* osdTextNumOffGliphsPerLine)>>2) ; j++)
            {
                *copyTo = *copyFrom;
                copyFrom++;
                copyTo++;
            }
            memset ((void*)(&(bufferTextPointAddress[(osdTextNumOffLines-1)*_OsdTextGliphsHeight_* osdTextNumOffGliphsPerLine])), 0x00, _OsdTextGliphsHeight_*osdTextNumOffGliphsPerLine);
            currentPrintfCursorLine--;
            currentPrintfCursorColm = 0;


        }
        if(0x0A == c)
        {
            currentPrintfCursorColm = 0;
            currentPrintfCursorLine++;
        }
        else
        {
            for (j = 0; j < 16; j++)
            {
                bufferTextPointAddress[((currentPrintfCursorLine * _OsdTextGliphsHeight_ +j) * (osdTextNumOffGliphsPerLine)) + currentPrintfCursorColm] = osdTextGliphs[c-0x20][j];
            }
            currentPrintfCursorColm++;
            if(currentPrintfCursorColm == osdTextNumOffGliphsPerLine)
            {
                currentPrintfCursorColm = 0;
                currentPrintfCursorLine++;
            }
        }
    }
}

/*!  */
static int SECT_OsdText_FUNCTION(osdTextprints) (char *   *out, int n, const char *string, int width, int pad)
{
    register int pc = 0, padchar = ' ';
    int len;
    const char *ptr;
    if (width > 0)
    {
        len = 0;
        for (ptr = string; *ptr; ++ptr)
        {
            ++len;
        }
        if (len >= width)
        {
            width = 0;
        }
        else
        {
            width -= len;
        }
        if (pad & PAD_ZERO)
        {
            padchar = '0';
        }
    }
    if (!(pad & PAD_RIGHT))
    {
        for (; width > 0; --width)
        {
            if( osdTextisValid(pc, n))
            {
                osdTextprintchar (out, padchar);
            }
            ++pc;
        }
    }
    for (; *string; ++string)
    {
        if( osdTextisValid(pc, n))
        {
            osdTextprintchar (out, *string);
        }
        ++pc;
    }
    for (; width > 0; --width)
    {
        if( osdTextisValid(pc, n))
        {
            osdTextprintchar (out, padchar);
        }
        ++pc;
    }
    return pc;
}

/*!  */
static int SECT_OsdText_FUNCTION(osdTextprinti) (char **out, int n, int i, int b, int sg, int width, int pad, int letbase)
{
    char print_buf[PRINT_BUF_LEN];
    char *s;
    int t, neg = 0, pc = 0;
    unsigned u = (unsigned) i;
    if (i == 0)
    {
        print_buf[0] = '0';
        print_buf[1] = '\0';
        return osdTextprints (out, n, print_buf, width, pad);
    }
    if (sg && b == 10 && i < 0)
    {
        neg = 1;
        u = (unsigned) -i;
    }
    //  make sure print_buf is NULL-term
    s = print_buf + PRINT_BUF_LEN - 1;
    *s = '\0';

    while (u)
    {
        t = u % b;
        if (t >= 10)
        {
            t += letbase - '0' - 10;
        }
        *--s = t + '0';
        u /= b;
    }
    if (neg)
    {
        if (width && (pad & PAD_ZERO))
        {
            osdTextprintchar (out, '-');
            ++pc;
            --width;
        }
        else
        {
            *--s = '-';
        }
    }
    else
    {
        if (use_leading_plus)
        {
            *--s = '+';
        }
    }
    return pc + osdTextprints (out, n, s, width, pad);
}

/*!  */
static unsigned SECT_OsdText_FUNCTION(osdTextdbl2stri)(char *outbfr, double dbl, unsigned dec_digits)
{
   static char local_bfr[128] ;
   char *output = (outbfr == 0) ? local_bfr : outbfr ;
   u32 mult = 1 ;
   u32 idx ;
   u32 wholeNum;
   u32 decimalNum;
   char tbfr[128] ;

   //*******************************************
   //  extract negative info
   //*******************************************
   if (dbl < 0.0) {
      *output++ = '-' ;
      dbl *= -1.0 ;
   } else {
      if (use_leading_plus) {
         *output++ = '+' ;
      }
   }

   //  handling rounding by adding .5LSB to the floating-point data
   if (dec_digits < 8) {
      dbl += round_nums[dec_digits] ;
   }

   //**************************************************************************
   //  construct fractional multiplier for specified number of digits.
   //**************************************************************************
   for (idx = 0; idx < dec_digits; idx++)
      mult *= 10 ;

    //printf("mult=%u\n", mult) ;

   //*******************************************
   //  convert integer portion
   //*******************************************
   idx = 0 ;
   wholeNum =  (u32)dbl;
   decimalNum = (u32)(( dbl - wholeNum )*mult );

   while (wholeNum != 0) {
      tbfr[idx++] = '0' + (wholeNum % 10) ;
      wholeNum /= 10 ;
   }
   // printf("--> whole=%s, dec=%d\n", tbfr, decimalNum) ;
   if (idx == 0) {
      *output++ = '0' ;
   } else {
      while (idx > 0) {
         *output++ = tbfr[idx-1] ;  //lint !e771
         idx-- ;
      }
   }
  if (dec_digits > 0) {
      *output++ = '.' ;

      //*******************************************
      //  convert fractional portion
      //*******************************************
      idx = 0 ;
      while ( decimalNum != 0 ) {
         tbfr[idx++] = '0' + (decimalNum % 10) ;
         decimalNum /= 10 ;
      }
      //  pad the decimal portion with 0s as necessary;
      //  We wouldn't want to report 3.093 as 3.93, would we??
      while (idx < dec_digits) {
         tbfr[idx++] = '0' ;
      }
      // printf("decimal=%s\n", tbfr) ;
      if (idx == 0) {
         *output++ = '0' ;
      } else {
         while (idx > 0) {
            *output++ = tbfr[idx-1] ;
            idx-- ;
         }
      }
   }
   *output = 0 ;

   //  prepare output
   output = (outbfr == 0) ? local_bfr : outbfr ;
   return osdTextmy_strlen(output) ;
}


/*!  */
static u32 SECT_OsdText_FUNCTION(osdTextmy_strlen)(char *str)
{
    u32 slen;
    if (str == 0)
    {
      return 0;
    }
    slen = 0 ;
    while (*str != 0)
    {
        slen++ ;
        str++ ;
    }
    return slen;
}


/*! Sets the position indicator associated with stream to the beginning of the file. */
void SECT_OsdText_FUNCTION(osdTextrewind)( void )
{
    currentPrintfCursorLine     = 0x00;
    currentPrintfCursorColm     = 0x00;
}

/*! Sets the position indicator associated with the stream to a new position defined by adding offset to a reference position specified by origin. */
int SECT_OsdText_FUNCTION(osdTextfseek) (long int offset, int origin )
{
    int returnVal = 0;
    u32 row = 0;
    u32 colum = 0;
    int wantedPosition = 0;
    switch (origin)
    {
        case OST_TEXT_SEEK_SET: //Beginning of file
        {
            wantedPosition =  offset;
            if ((wantedPosition>(osdTextNumOffGliphsPerLine*osdTextNumOffLines))&&(wantedPosition<0))
            {
                returnVal = OST_TEXT_SEEK_SET;
            }
            else
            {
                currentPrintfCursorLine = (u32)(wantedPosition/osdTextNumOffGliphsPerLine);
                currentPrintfCursorColm = (u32)(wantedPosition%osdTextNumOffGliphsPerLine);
            }
        }
        break;
        case OST_TEXT_SEEK_END:
        {
            wantedPosition =  (osdTextNumOffGliphsPerLine*osdTextNumOffLines) + offset;
            if ((wantedPosition>(osdTextNumOffGliphsPerLine*osdTextNumOffLines))&&(wantedPosition<0))
            {
                returnVal = OST_TEXT_SEEK_END;
            }
            else
            {
                currentPrintfCursorLine = (u32)(wantedPosition/osdTextNumOffGliphsPerLine);
                currentPrintfCursorColm = (u32)(wantedPosition%osdTextNumOffGliphsPerLine);
            }
        }
        break;
        default:
        {
            wantedPosition =  (currentPrintfCursorLine * osdTextNumOffGliphsPerLine + currentPrintfCursorColm) + offset;
            if ((wantedPosition>(osdTextNumOffGliphsPerLine*osdTextNumOffLines))&&(wantedPosition<0))
            {
                returnVal = OST_TEXT_SEEK_CUR;
            }
            else
            {
                currentPrintfCursorLine = (u32)(wantedPosition/osdTextNumOffGliphsPerLine);
                currentPrintfCursorColm = (u32)(wantedPosition%osdTextNumOffGliphsPerLine);
            }
        }
        break;
    }
    return returnVal;
}

/*! Returns the current value of the position indicator of the stream. */
long int SECT_OsdText_FUNCTION(osdTextftell) (void)
{
    return (currentPrintfCursorLine * osdTextNumOffGliphsPerLine + currentPrintfCursorColm);
}

/*! clrscr C function */
void SECT_OsdText_FUNCTION(osdTextclrscr)(void)
{
    osdTextCleanAll();
    osdTextrewind();
}

/*! gotoxy function */
void SECT_OsdText_FUNCTION(osdTextgotoxy)(u32 X, u32 Y)
{
    if ((Y>=1)&&(Y<=osdTextNumOffLines))
    {
        currentPrintfCursorLine     = Y-1;
    }
    if((X>=1)&&(X<=osdTextNumOffGliphsPerLine))
    {
        currentPrintfCursorColm     = X-1;
    }
}

