///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     main leon file
///

// 1: Includes
//
#include "ImageKernelProcessingApi.h"
#include <swcFrameTypes.h>

#ifndef PC_MODEL
#include "svuCommonShave.h"
#include <mvcv.h>
#else
#include "../kernels/gaussHx2.h"
#include "../k∫ernels/gaussVx2.h"
#include "myriadModel.h"
#include <stdio.h>
#include "../../../common/swCommon/pcModel/myriadModel.h"
#include "svuCommonShaveDefines.h"
#endif

using namespace mvcv;

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#define LINES_NUM 7
#define MAXWIDTH 1280
#define GAUSSVX2_MAX 5

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
// ----------------------------------------------------------------------------
// copy from in address to CMX on 2 lines
unsigned char linesCMXShave[LINES_NUM * MAXWIDTH];
// copy from CMX to out address on 2 lines
unsigned char outLinesShave[LINES_NUM * MAXWIDTH];
volatile int first = 0;

#ifdef PC_MODEL
unsigned int cmxLines1Addr = CMX_DATA_START_ADR;
unsigned int cmxLines2Addr = CMX_DATA_START_ADR + LINES_NUM * MAXWIDTH;

unsigned char *outLines = (unsigned char *) cmxLines2Addr;
unsigned char *linesCMX = (unsigned char *) cmxLines1Addr;
#else
//For the shave target, these pointers are just initialized with the
//lines addresses
unsigned char *outLines = (unsigned char *) &linesCMXShave[0];
unsigned char *linesCMX = (unsigned char *) &outLinesShave[0];
#endif

// in address lines from CMX, will be input for kernel
unsigned char* linesAddr[LINES_NUM] = {
        (unsigned char*)0x0,
        (unsigned char*)0x0,
        (unsigned char*)0x0,
        (unsigned char*)0x0,
        (unsigned char*)0x0,
        (unsigned char*)0x0,
        (unsigned char*)0x0,
};

unsigned char* linesGaussVx2Addr[GAUSSVX2_MAX] = {
        (unsigned char*)0x0,
        (unsigned char*)0x0,
        (unsigned char*)0x0,
        (unsigned char*)0x0,
        (unsigned char*)0x0,
};
// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
void kernel(unsigned char* inputLines[LINES_NUM], unsigned char outLine[LINES_NUM], unsigned int lineWidth, unsigned int pos);
void makeBlack(unsigned char* src, unsigned int plane);
void processPlane(frameBuffer* outFrBuff, frameBuffer* inFrBuff, swcPlaneType plane);

// 6: Functions Implementation
// ----------------------------------------------------------------------------
void ImageKernelProcessingStart(frameBuffer* outFrBuff, frameBuffer* inFrBuff)
{
#ifndef PC_MODEL
    linesCMX = linesCMXShave;
    outLines = outLinesShave;
#endif
    processPlane(outFrBuff, inFrBuff, Y);
    processPlane(outFrBuff, inFrBuff, U);
    processPlane(outFrBuff, inFrBuff, V);

    //Exit point.
#ifndef PC_MODEL
    exit(1);
#endif
}

void kernel(unsigned char* inputLines[LINES_NUM], unsigned char* outLine, unsigned int lineWidth, unsigned int pos)
{
    int i = 0;

    if (first == 0)
    {
        /// Gauss horizontal kernel, needs 1 lines to output one line
        GaussHx2((u8*)&inputLines[pos    ][0], (u8*)&outLine[(pos    ) * lineWidth / 2], lineWidth);
        GaussHx2((u8*)&inputLines[pos + 1][0], (u8*)&outLine[(pos + 1) * lineWidth / 2], lineWidth);
        GaussHx2((u8*)&inputLines[pos + 2][0], (u8*)&outLine[(pos + 2) * lineWidth / 2], lineWidth);
        GaussHx2((u8*)&inputLines[pos + 3][0], (u8*)&outLine[(pos + 3) * lineWidth / 2], lineWidth);
        GaussHx2((u8*)&inputLines[pos + 4][0], (u8*)&outLine[(pos + 4) * lineWidth / 2], lineWidth);

        /// Gauss vertical kernel, needs 5 lines to output one line
        /// for first line needs exactly first 5 lines from input
        for (i = 0; i < GAUSSVX2_MAX; i++)
        {
            linesGaussVx2Addr[i] = &outLine[pos + i * lineWidth / 2];
        }
        first = 1;
    }
    else
    {
        GaussHx2((u8*)&inputLines[pos    ][0], (u8*)&outLine[(pos    ) * lineWidth / 2], lineWidth);
        GaussHx2((u8*)&inputLines[pos + 1][0], (u8*)&outLine[(pos + 1) * lineWidth / 2], lineWidth);
        /// adding 2 new input lines near the last 3 from the previous output line
        linesGaussVx2Addr[3] = &outLine[(pos    ) * lineWidth / 2];
        linesGaussVx2Addr[4] = &outLine[(pos + 1) * lineWidth / 2];
    }

    /// Gauss vertical kernel, needs 5 lines to output one line
    GaussVx2((u8**)&linesGaussVx2Addr[0], (u8*)&outLine[pos * lineWidth / 2], lineWidth / 2);

    /// Gauss vertical kernell needs 5 lines to output one line
    /// after the first out line the rest are made out of the last 3 input lines used for
    /// the previous output line and 2 new input lines
    for (i = 0; i < 3; i++)
    {
        linesGaussVx2Addr[i] = linesGaussVx2Addr[i + 2];
    }

}

void makeBlack(unsigned char* src, unsigned int plane)
{
    if (plane == Y)
    {
        // Y plane
#ifndef PC_MODEL
        src[0] = 0x0;
#else
        mmGetPtr((u32) &src[0])[0] = 0x0;
#endif
    }
    else
    {
        // U or V plane
#ifndef PC_MODEL
        src[0] = 0x80;
#else
        mmGetPtr((u32) &src[0])[0] = 0x80;
#endif
    }

}


void processPlane(frameBuffer* outFrBuff, frameBuffer* inFrBuff, swcPlaneType plane)
{
    int i;
    unsigned int inWidth, inHeight, outWidth, outHeight, outLineCnt = 0;
    unsigned int outAddress;
    unsigned int inAddress;
    int lineCnt = 0, linesLeft = 0, linesCMXCnt = 0;

    switch (plane)
    {
    case Y:
    {
        inWidth = inFrBuff->spec.width;
        inHeight= inFrBuff->spec.height;
        outWidth = outFrBuff->spec.width;
        outHeight = outFrBuff->spec.height;
        outAddress = (unsigned int)outFrBuff->p1;
        inAddress = (unsigned int)inFrBuff->p1;
        break;
    }
    case U:
    {
        inWidth = inFrBuff->spec.width / 2;
        inHeight= inFrBuff->spec.height / 2;
        outWidth = outFrBuff->spec.width / 2;
        outHeight = outFrBuff->spec.height / 2;
        outAddress = (unsigned int)outFrBuff->p2;
        inAddress = (unsigned int)inFrBuff->p2;
        break;
    }
    case V:
    {
        inWidth = inFrBuff->spec.width / 2;
        inHeight= inFrBuff->spec.height / 2;
        outWidth = outFrBuff->spec.width / 2;
        outHeight = outFrBuff->spec.height / 2;
        outAddress = (unsigned int)outFrBuff->p3;
        inAddress = (unsigned int)inFrBuff->p3;
        break;
    }
    default:
        return;
    }
    //// Y plane
    linesCMXCnt = 0;
    outLineCnt = 0;
    lineCnt = 0;

    // in address lines from CMX for Y plane
    for (i = 0; i < LINES_NUM; i++)
    {
        linesAddr[i] = &linesCMX[i * inWidth];
    }

    for (i = 0; i < GAUSSVX2_MAX; i++)
    {
        linesGaussVx2Addr[i] = (unsigned char*)0x0;
        first = 0;
    }


    scDmaSetupStrideDst(DMA_TASK_0, (unsigned char*)inAddress + lineCnt++ * inWidth, &linesCMX[linesCMXCnt++ * inWidth], inWidth, inWidth, 0);
    scDmaSetupStrideDst(DMA_TASK_1, (unsigned char*)inAddress + lineCnt++ * inWidth, &linesCMX[linesCMXCnt++ * inWidth], inWidth, inWidth, 0);
    scDmaSetupStrideDst(DMA_TASK_2, (unsigned char*)inAddress + lineCnt++ * inWidth, &linesCMX[linesCMXCnt++ * inWidth], inWidth, inWidth, 0);
    scDmaSetupStrideDst(DMA_TASK_3, (unsigned char*)inAddress + lineCnt++ * inWidth, &linesCMX[linesCMXCnt++ * inWidth], inWidth, inWidth, 0);
    scDmaStart(START_DMA0123);
    scDmaWaitFinished();
    scDmaSetupStrideDst(DMA_TASK_0, (unsigned char*)inAddress + lineCnt++ * inWidth, &linesCMX[linesCMXCnt++ * inWidth], inWidth, inWidth, 0);
    scDmaStart(START_DMA0);
    scDmaWaitFinished();

    //copy height lines for the Y plane
    while (outLineCnt < (outHeight))
    {
        linesLeft = inHeight - lineCnt;

        if (linesLeft > 0)
        {
            scDmaSetupStrideDst(DMA_TASK_0, (unsigned char*)inAddress + lineCnt++ * inWidth, &linesCMX[linesCMXCnt++ * inWidth], inWidth, inWidth, 0);
            if (linesLeft > 1)
            {
                scDmaSetupStrideDst(DMA_TASK_1, (unsigned char*)inAddress + lineCnt++ * inWidth, &linesCMX[linesCMXCnt++ * inWidth], inWidth, inWidth, 0);
                scDmaStart(START_DMA01);
            }
            else
                scDmaStart(START_DMA0);
        }

        if (linesCMXCnt > 5)
            linesCMXCnt = 0;
        else
            linesCMXCnt = 5;

        //Apply kernel to CMX line, inWidth because is Y plane
        kernel(linesAddr, outLines, inWidth, linesCMXCnt);

        if (linesLeft > 0)
        {
            // start in address to CMX copy
            scDmaWaitFinished();
        }

        // CMX to in address copy
        //not using mirroring so ignoring first 3 and last 3 columns, by making it black
        makeBlack((u8*)&outLines[linesCMXCnt * outWidth], plane);
        makeBlack((u8*)&outLines[linesCMXCnt * outWidth + outWidth - 1], plane);
        if (plane == Y) // for Y plane
        {
            makeBlack((u8*)&outLines[linesCMXCnt * outWidth + 1], plane);
            makeBlack((u8*)&outLines[linesCMXCnt * outWidth + outWidth - 2], plane);
        }

        scDmaSetupStrideDst(DMA_TASK_0, &outLines[(linesCMXCnt + 0) * outWidth], (unsigned char*)outAddress  + outLineCnt++ * outWidth, outWidth, outWidth, 0);
        scDmaStart(START_DMA0);
        scDmaWaitFinished();

    }
}

