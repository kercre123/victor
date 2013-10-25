///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief
///

// 1: Includes
// ----------------------------------------------------------------------------
#include "pcKernelProcessingApi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myriadModel.h"

#include <swcFrameTypes.h>
#include "ImageKernelProcessingApi.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
// ----------------------------------------------------------------------------

#define FRAME_MAX_WIDTH 1280
#define FRAME_MAX_HEIGHT 720

u32        inAddr = DDR_DATA_START_ADR;
u32       outAddr = DDR_DATA_START_ADR + ((FRAME_MAX_HEIGHT * FRAME_MAX_WIDTH) * 3 / 2);
frameBuffer outFrBuffPC;
frameBuffer inFrBuffPC;
frameSpec inFrSpcPC;
frameSpec outFrSpcPC;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
// 6: Functions Implementation
// ----------------------------------------------------------------------------


void pcInt(int w, int h)
{
    inFrSpcPC.width = w;
    inFrSpcPC.height = h;
    inFrSpcPC.type = YUV420p;

    outFrSpcPC.width = w / 2;
    outFrSpcPC.height = h / 2;
    outFrSpcPC.type = YUV420p;

    outFrBuffPC.spec = outFrSpcPC;
    inFrBuffPC.spec = inFrSpcPC;

    inFrBuffPC.p1 = (u8*) inAddr;
    inFrBuffPC.p2 = (u8*) inAddr + inFrBuffPC.spec.width * inFrBuffPC.spec.height;
    inFrBuffPC.p3 = (u8*) inAddr + inFrBuffPC.spec.width * inFrBuffPC.spec.height * 5 / 4;

    outFrBuffPC.p1 = (u8*) outAddr;
    outFrBuffPC.p2 = (u8*) outAddr + outFrBuffPC.spec.width * outFrBuffPC.spec.height;
    outFrBuffPC.p3 = (u8*) outAddr + outFrBuffPC.spec.width * outFrBuffPC.spec.height * 5 / 4;
}

void prepInputFrame(frameBuffer* inFr, char * inData, char * outData)
{
    mmSetMemcpy((u32)inFr->p1, inData, inFr->spec.width * inFr->spec.height * 3 / 2);
}

void prepOuputFrame(frameBuffer* outFr, char * outData, int outFrPos)
{
    mmGetMemcpy(outData, (u32)outFr->p1, outFr->spec.width * outFr->spec.height * 3 / 2);
}

void pcKernelProcessing(char * inFileName, int width, int height, int nrFrames, char * outFileName)
{
    pcInt(width, height);

    FILE *fpIn, *fpOut;
    int x, frRead = 0;
    int sz = 0; //file size
    char * frOutData;
    char * frInData;

    fpIn = fopen(inFileName, "r");

    // fopen returns 0, the NULL pointer, on failure
    if (fpIn == 0)
    {
        printf( "Could not open file %s\n", inFileName);
    }
    else
    {
        // Setup myriad model and actually call the myriad code we want to test
        mmCreate();

        fseek(fpIn, 0, SEEK_END);
        sz = ftell(fpIn);
        fseek(fpIn, 0, SEEK_SET);

        if (nrFrames != (sz / (width * height *3 / 2)))
        {
            printf("Incorrect number of frames\n");
            return;
        }

        // read one frame
        frInData = (char * )malloc(width * height * 3 / 2 * sizeof(char));
        frOutData = (char * )malloc(width * height * 3 / 2 * sizeof(char));
        fpOut = fopen(outFileName, "w");

        while (frRead < nrFrames)
        {
            if ((x = fread(frInData, sizeof(char), width * height * 3 / 2, fpIn)) > 0)
            {
                prepInputFrame(&inFrBuffPC, frInData, frOutData);
                if (x != (width * height * 3 / 2))
                {
                    printf("Frame read problem!\n");
                    return;
                }

                ImageKernelProcessingStart(&outFrBuffPC, &inFrBuffPC);

                prepOuputFrame(&outFrBuffPC, frOutData, frRead);

                frRead++;
                if (1 != fwrite(frOutData, width / 2 * height / 2 * 3 / 2 * sizeof(char), 1, fpOut))
                {
                    printf("ERROR: cannot write frame to output file (%s)\n", outFileName);
                }
            }
            else return;
        }

        fclose(fpIn);
        fclose(fpOut);
        mmDestroy();
    }
    return;
}
