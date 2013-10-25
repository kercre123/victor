///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     main leon file 
///

// 1: Includes
// ----------------------------------------------------------------------------

#ifndef PC_MODEL
    #include "isaac_registers.h"
    #include "mv_types.h"
    #include "sysClkCfg/sysClk180_180.h"
    #include "assert.h"
    #include "swcShaveLoader.h"
    #include "app_config.h"

    #include "HdmiTxIteApi.h"
    #include "LcdApi.h"
    #include "BoardApi.h"
    #include "brdMv0153.h"

    #include "CIFGenericApi.h"
    #include "OV5_720p30.h"
    #include "LcdCEA720p60.h"
	#include "CifOV5642Api.h"
#else
    #include <stdio.h>
    #include <stdlib.h>
    #include "pcKernelProcessingApi.h"
#endif

#include <swcFrameTypes.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#define RES_WIDTH           1280
#define RES_HEIGHT          720
#define IN_FRAME_SIZE       (RES_WIDTH * RES_HEIGHT * 3 / 2)
#define OUT_FRAME_SIZE      (RES_WIDTH / 2 * RES_HEIGHT / 2 * 3 / 2)
#define MAX_SHAVE_STACK     0x1C01E7FC
#define SHAVE_NUMBER        1

#ifdef __APPLE__
#define DDR_CAMBUF
#else
#define DDR_CAMBUF __attribute__((section(".ddr_direct.bss")))
#endif

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
extern void*  (SVE1_ImageKernelProcessingStart);

// 4: Static Local Data
// ----------------------------------------------------------------------------
// 3 Buffers used for camera sensor output (used 3 to avoid frame tearing)
frameBuffer camBuffs[3];
// 3 Buffers used for effect output (used 3 to avoid frame tearing)
frameBuffer effectBuffs[3];
// LCD out buffer
frameBuffer* lcdBuffer;

#ifndef PC_MODEL
    SensorHandle hndl1;
    LCDHandle lcdHndl;
#endif

    volatile unsigned int cifFrameCnt = 0;
    volatile unsigned int effectFrameCnt = 0;
    volatile unsigned int cam1BufferPosition = 0;
    volatile unsigned int effectBufferPosition = 0;
    volatile unsigned int LCDeffectBufferPosition = 0;
    volatile unsigned int firstShaveRun = 0;
    volatile unsigned int dones = 0;

// DDR addresses defined for camera output buffers
DDR_CAMBUF unsigned char CAM1_capt1[IN_FRAME_SIZE];
DDR_CAMBUF unsigned char CAM1_capt2[IN_FRAME_SIZE];
DDR_CAMBUF unsigned char CAM1_capt3[IN_FRAME_SIZE];

DDR_CAMBUF unsigned char CAM2_capt1[OUT_FRAME_SIZE];
DDR_CAMBUF unsigned char CAM2_capt2[OUT_FRAME_SIZE];
DDR_CAMBUF unsigned char CAM2_capt3[OUT_FRAME_SIZE];

frameSpec CIFFrSpc;
frameSpec LCDFrSpc;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
// 6: Functions Implementation
// ----------------------------------------------------------------------------

// Allocate memory to the buffers and initiate frame specs
void Init (int width, int height)
{
    CIFFrSpc.height  = height;
    CIFFrSpc.width   = width;
    CIFFrSpc.stride  = width;
    CIFFrSpc.type    = YUV420p;
    CIFFrSpc.bytesPP = 1;

    LCDFrSpc.height  = height / 2;
    LCDFrSpc.width   = width / 2;
    LCDFrSpc.stride  = width / 2;
    LCDFrSpc.type    = YUV420p;
    LCDFrSpc.bytesPP = 1;

    camBuffs[0].spec = CIFFrSpc;
    camBuffs[0].p1   = CAM1_capt1;
    camBuffs[0].p2   = CAM1_capt1 + width * height;
    camBuffs[0].p3   = CAM1_capt1 + width * height * 5 / 4;

    camBuffs[1].spec = CIFFrSpc;
    camBuffs[1].p1   = CAM1_capt2;
    camBuffs[1].p2   = CAM1_capt2 + width * height;
    camBuffs[1].p3   = CAM1_capt2 + width * height * 5 / 4;

    camBuffs[2].spec = CIFFrSpc;
    camBuffs[2].p1   = CAM1_capt3;
    camBuffs[2].p2   = CAM1_capt3 + width * height;
    camBuffs[2].p3   = CAM1_capt3 + width * height * 5 / 4;

    effectBuffs[0].spec = LCDFrSpc;
    effectBuffs[0].p1   = CAM2_capt1;
    effectBuffs[0].p2   = CAM2_capt1 + width / 2 * height / 2;
    effectBuffs[0].p3   = CAM2_capt1 + width / 2 * height / 2 * 5 / 4;

    effectBuffs[1].spec = LCDFrSpc;
    effectBuffs[1].p1   = CAM2_capt2;
    effectBuffs[1].p2   = CAM2_capt2 + width / 2 * height / 2;
    effectBuffs[1].p3   = CAM2_capt2 + width / 2 * height / 2 * 5 / 4;

    effectBuffs[2].spec = LCDFrSpc;
    effectBuffs[2].p1   = CAM2_capt3;
    effectBuffs[2].p2   = CAM2_capt3 + width / 2 * height / 2;
    effectBuffs[2].p3   = CAM2_capt3 + width / 2 * height / 2 * 5 / 4;
}

#ifndef PC_MODEL
    frameBuffer* allocateFnLeft(void)
    {
        cam1BufferPosition = (++cifFrameCnt) % 3;
        return &camBuffs[cam1BufferPosition];
    }

    void effectDone(u32 x)
    {
        lcdBuffer = &effectBuffs[(effectBufferPosition + 2) %3];
        dones++;
        return;
    }

    void frameReadyFnLeft (frameBuffer * f)
    {
        if ((!swcShaveRunning(0)) || (firstShaveRun == 0))
        {
            firstShaveRun = 1;
            //Reset the shave used
            swcResetShave(1);
            //Set stack according to calling convention
            swcSetDefaultStack(SHAVE_NUMBER);

            //Starting effect on shave
            swcStartShaveAsyncCC(SHAVE_NUMBER, (u32)&SVE1_ImageKernelProcessingStart, effectDone, "ii", ((unsigned int)&effectBuffs[effectBufferPosition]), ((unsigned int)&camBuffs[(cam1BufferPosition + 1) % 3]));

            effectBufferPosition = (++effectFrameCnt) % 3;
        }
    }

    frameBuffer* allocateLcdFrame(int layer)
    {
        switch(layer)
        {
        case VL1:{
            return lcdBuffer;
            break;
        }
        default:{
            return NULL;
            break;
        }
        }
    }

    void SetHighFreq()
    {
        brd153ExternalPllConfigure(EXT_PLL_CFG_148MHZ);
        return;
    }

    void SetLowFreq()
    {
        brd153ExternalPllConfigure(EXT_PLL_CFG_74MHZ);
        return;
    }
#endif


void check()
{

#ifndef PC_MODEL

    if (cifFrameCnt == 120)
    {

        LCDInit(&lcdHndl, &lcdSpec720p60, &LCDFrSpc, LCD1);
        LCDSetupCallbacks(&lcdHndl, &allocateLcdFrame, NULL, &SetHighFreq, &SetLowFreq);
        LCDStart(&lcdHndl);
    }
#endif
}

int main ( int argc, char *argv[] )
{

#ifdef PC_MODEL


    if ( argc != 6 ) // argc should be 2 for correct execution
    {
        //Program name usage
        printf( "Usage: ./exec_name [input stream] [input_width] [input_height] [number of input frames] [output stream name]\n");
    }
    else
    {
        pcKernelProcessing(argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), argv[5]);
    }

#else

    initClocksAndMemory();

    BoardInitialise();

    HdmiSetup(gAppDevHndls.i2c2Handle);

    HdmiConfigure();

    Init(RES_WIDTH, RES_HEIGHT);

    CifInit(&hndl1, &camSpec720p30, &CIFFrSpc, gAppDevHndls.i2c1Handle, CAM1);
    CifSetupCallbacks(&hndl1, &allocateFnLeft, &frameReadyFnLeft, NULL, NULL);
    CifStart(&hndl1, NULL);
	CifOV5642FlipMirror(&hndl1, &camSpec720p30, !BoardGetDIPSwitchState(4));


    // Infinite loop
    while(1)
    {
        check();
    }
#endif
    return 0;
}
