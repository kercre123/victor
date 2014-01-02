/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Definitions and types needed by the Lcd operations library
///

#ifndef _LCD_API_DEFINES_H_
#define _LCD_API_DEFINES_H_

#include "swcFrameTypes.h"
// 1: Defines
// ----------------------------------------------------------------------------
#ifndef LCD_DATA_SECTION
#define LCD_DATA_SECTION(x) x
#endif

#ifndef LCD_CODE_SECTION
#define LCD_CODE_SECTION(x) x
#endif

#ifndef LCDGENERIC_INTERRUPT_LEVEL
#define LCDGENERIC_INTERRUPT_LEVEL 3
#endif

#define N_COEFS 12

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
enum
{
    LCD1,
    LCD2
};

enum{
    VL1,
    VL2,
    GL1,
    GL2
};

enum layerEnabledMask {
    VL1_ENABLED = 0x1000,
    VL2_ENABLED = 0x0100,
    GL1_ENABLED = 0x0010,
    GL2_ENABLED = 0x0001,
};

typedef struct
{
    unsigned int x;
    unsigned int y;
}LCDLayerOffset;

typedef struct
{
    unsigned int width;          // = 1280,
    unsigned int hPulseWidth;    // = 40
    unsigned int hBackP;         // = 220,
    unsigned int hFrontP;        // = 110
    unsigned int height;         // = 720,
    unsigned int vPulseWidth;    // = 5,
    unsigned int vBackP;         // = 20,
    unsigned int vFrontP;        // = 5
    unsigned int outputFormat;   // = D_LCD_OUTF_FORMAT_YCBCR444 |D_LCD_OUTF_PCLK_EXT,
    unsigned int control;         // = D_LCD_CTRL_OUTPUT_RGB |D_LCD_CTRL_PROGRESSIVE
    unsigned int pixelClockkHz;  // = 74250
}LCDDisplayCfg;

typedef frameBuffer* (allocateLcdFn) (int layer);
typedef void (frameReadyLcdFn) (frameBuffer *VL1Buffer, frameBuffer *VL2Buffer, frameBuffer *GL1Buffer, frameBuffer *GL2Buffer);
typedef void (freqLcdFn)( void );

struct _lcdHandle
{
    unsigned int            lcdNumber;
    volatile unsigned int   LCDFrameCount;
    const LCDDisplayCfg    *lcdSpec;
    const frameSpec        *frameSpec;
    const frameSpec        *VL1frameSpec;
    const frameSpec        *VL2frameSpec;
    const frameSpec        *GL1FrameSpec;
    const frameSpec        *GL2FrameSpec;
    LCDLayerOffset          VL1offset;
    LCDLayerOffset          VL2offset;
    LCDLayerOffset          GL1offset;
    LCDLayerOffset          GL2offset;
    frameBuffer            *currentFrameVL1;
    frameBuffer            *currentFrameVL2;
    frameBuffer            *currentFrameGL1;
    frameBuffer            *currentFrameGL2;
    int                     layerEnabled;
    allocateLcdFn          *cbGetFrame;
    frameReadyLcdFn        *cbFrameReady;
    freqLcdFn              *callBackFctHighRes;
    freqLcdFn              *callBackFctLowRes;
    int                     firstFrameQueued;   // used for one-shot mode only
    frameBuffer            *queuedFrames[3][4]; // used for one-shot mode only
    volatile int            qfront, qback;       // used for one-shot mode only
};

typedef struct _lcdHandle LCDHandle;

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------
#endif // _LCD_API_DEFINES_H_;
