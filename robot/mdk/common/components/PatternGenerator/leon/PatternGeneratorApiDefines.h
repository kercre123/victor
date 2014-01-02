
///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Pattern Generator Defines.
///

#ifndef _PATTERN_GENERATOR_API_DEFINES_H_
#define _PATTERN_GENERATOR_API_DEFINES_H_

// 1: Defines
// ----------------------------------------------------------------------------

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

typedef struct PatternPointELM
{
    unsigned int x;
    unsigned int y;
}PatternPoint;


typedef struct PatternYUVColorElm
{
    unsigned char Y;
    unsigned char U;
    unsigned char V;
}PatternYUVColor;


typedef struct PatternRGBColorElm
{
    unsigned int R;
    unsigned int G;
    unsigned int B;
}PatternRGBColor;


typedef enum PatternTypeDefinitions
{
    HorizontalColor,
    VerticalColor,
    ColorStripes,
    LinearGrey
}PatternType;



// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // _PATTERN_GENERATOR_API_DEFINES_H_
