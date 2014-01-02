///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Definitions and types needed by the Math operations library
///
/// This is the file that contains all the Sensor definitions of constants, typedefs,
/// structures or enums or exported variables from this module

#ifndef _CIF_GENERIC_API_DEFINES_H_
#define _CIF_GENERIC_API_DEFINES_H_

#include "swcFrameTypes.h"
#include "DrvI2cMasterDefines.h"

// 1: Defines
// ----------------------------------------------------------------------------
//generate sync signal
#define GENERATE_SYNCS      1<<1

#ifndef CIFGENERIC_INTERRUPT_LEVEL
#define CIFGENERIC_INTERRUPT_LEVEL 3
#endif
// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
enum
{
    CAM1,
    CAM2
};

typedef struct
{
    unsigned int height;
    unsigned int width;
    unsigned int hBackP;
    unsigned int hFrontP;
    unsigned int vBackP;
    unsigned int vFrontP;
    unsigned int generateSync;
}CifTimingsCfg;

typedef struct
{
    CifTimingsCfg    cifTiming;
    unsigned int     idealRefFreq;
    unsigned int     cfgDelay;
    unsigned int     sensorI2CAddress;
    unsigned int     regNumber;
    unsigned int     regSize;
    const unsigned short   (*regValues)[2];
}CamSpec;

typedef frameBuffer* (allocateFn) (void);
typedef void (frameReadyFn) (frameBuffer *);
typedef unsigned char* (allocateBlockFn) (int);
typedef void (blockReadyFn) (int);
typedef void (EOFFn) (void);

typedef struct _sensorHandle SensorHandle;

struct _sensorHandle
{
    unsigned int             sensorNumber;
    volatile unsigned int    CIFFrameCount;
    I2CM_Device *            pI2cHandle;
    CamSpec*                 camSpec;
    frameSpec*               frameSpec;
    frameBuffer*             currentFrame;
    allocateFn*              cbGetFrame;
    frameReadyFn*            cbFrameReady;
    allocateBlockFn*         cbGetBlock;
    blockReadyFn*            cbBlockReady;
    EOFFn*                   cbEOF;
};


// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------


#endif //_CIF_GENERIC_API_DEFINES_H_
