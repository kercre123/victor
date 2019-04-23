/***************************************************************************
(C) Copyright 2017 Signal Essence; All Rights Reserved

   "simple": Example use of MMIF/MMFx

   Author: Robert Yu
*************************************************************************/

#include "block_reader.h"
#include "block_writer.h"
#include "mmif_proj.h"
#include "mmif.h"
#include <assert.h>
#include <string.h>

#define BLOCKSIZE_SIN   160
#define BLOCKSIZE_SOUT  160
#define BLOCKSIZE_REFIN 160
#define BLOCKSIZE_RIN   160
#define BLOCKSIZE_ROUT  160
#define NUM_MICS        4
#define NUM_ROUT_CHAN   1
#define NUM_SOUT_CHAN   1

//
// approximate delay between REFIN and SIN.
// This value is determined by your hardware configuration.
// The delay specified here should be slightly less (5 msec) than the actual delay.
#define REF_BULK_DELAY_SEC  ((float)0.0)

// demonstrate sediag interface
static void demo_sediag()
{
    int id;
    SEDiagType_t type;
    uint16 val_u16;
    //float32 val_f32;
    SEDiagRW_t readWrite;
    const char *pDescr;
    int numElements;
    const char *pIdNumBeams = "fdsearch_num_beams_to_search";
    const char *pIdDirection = "fdsw_current_winner_beam";
    //const char *pErle = "aecmon_ave_erle_op_db";

    // 
    // get values associated with direction search num_beams_to_search
    id = SEDiagGetIndex(pIdNumBeams);
    type = SEDiagGetType(id);
    readWrite = SEDiagGetRW(id);
    pDescr = SEDiagGetHelp(id);
    numElements = SEDiagGetLen(id);
    val_u16 = SEDiagGetUInt16(id);

    printf("--------------------\n");
    printf("diag name: (%s) = %d\n", pIdNumBeams, val_u16);
    printf("type: %d (where %d == SE_DIAG_UINT16)\n", (int)type, SE_DIAG_UINT16);
    printf("readWrite: %d (where %d == SE_DIAG_R)\n", (int)readWrite, SE_DIAG_R);
    printf("numElements: %d\n", numElements);
    printf("description: %s\n", pDescr);

    //
    // get current direction
    id = SEDiagGetIndex(pIdDirection);
    pDescr = SEDiagGetHelp(id);
    val_u16 = SEDiagGetUInt16(id);

    printf("--------------------\n");
    printf("diag name: (%s) = %d\n", pIdDirection, val_u16);
    printf("description: %s\n", pDescr);
#if 0
    //
    // get final ERLE, averaged over all mics
    id = SEDiagGetIndex(pErle);
    val_f32 = SEDiagGetFloat32(id);
    pDescr = SEDiagGetHelp(id);
    printf("--------------------\n");
    printf("diag name: (%s) = %f\n", pErle, val_f32);
    printf("description: %s\n", pDescr);
#endif
}

static void verify_params(void)
{
    // verify blocksizes against blocksizes configured in MMIF
    assert(MMIfGetBlockSize(PORT_SEND_SIN)   == BLOCKSIZE_SIN);
    assert(MMIfGetBlockSize(PORT_SEND_SOUT)  == BLOCKSIZE_SOUT);
    assert(MMIfGetBlockSize(PORT_SEND_REFIN) == BLOCKSIZE_REFIN);

    assert(MMIfGetBlockSize(PORT_RCV_RIN)  == BLOCKSIZE_RIN);
    assert(MMIfGetBlockSize(PORT_RCV_ROUT) == BLOCKSIZE_ROUT);

    // verify number of mics against MMIF configuration
    assert(MMIfGetNumMicrophones() == NUM_MICS);
}

int main()
{
    BlockReader_t blockReader;
    BlockWriter_t blockWriter;
    int isEof;
    int16 sinPtr[BLOCKSIZE_SIN   * NUM_MICS];
    int16 soutPtr[BLOCKSIZE_SOUT * NUM_SOUT_CHAN];
    int16 refinPtr[BLOCKSIZE_REFIN];
    int16 rinPtr[BLOCKSIZE_RIN];
    int16 routPtr[BLOCKSIZE_ROUT * NUM_ROUT_CHAN];
    int blockIndex=0;

    //
    // initialize MMIF, specify refin->sin delay
    MMIfInit(REF_BULK_DELAY_SEC, NULL);
    printf("version=(%s)\n",MMIfGetVersionString());

    verify_params();

    //
    // initialize fileio (block-reader, block-writer)
    BrInit(&blockReader, "config.json");
    BrOpenFiles(&blockReader);
    BwInit(&blockWriter, "config.json");
    BwOpenFiles(&blockWriter);

    isEof = 0;
    while(isEof==0)
    {
        isEof = BrGetSampleBlocks(&blockReader, 
                                  rinPtr,       // input to receive path
                                  refinPtr,     // reference signal (input to transmit path)
                                  sinPtr);      // mic signals (inputs to transmit path)

        // process receive (loudspeaker) path 
        //
        // rin  = input signals
        // rout = processed signal, destined for loudspeaker
        RcvIfProcessReceivePath(rinPtr, routPtr);

        // process transmit (microphone) path
        //
        // refinPtr = reference input; depending on system, maybe same as routPtr
        // sinPtr   = mic inputs
        // soutPtr  = processed signal
        MMIfProcessMicrophones(refinPtr, 
                               sinPtr, 
                               soutPtr);

        BwWriteSampleBlocks(&blockWriter,
                            routPtr,
                            soutPtr);

        if (blockIndex % 100 == 0) 
        {
            printf(".");fflush(stdout);
        }
        blockIndex++;
    }
    printf("\n");

    //
    // demonstrate sediag
    demo_sediag();

    MMIfDestroy();
    BrDestroy(&blockReader);
    BwDestroy(&blockWriter);
    return 0;
}
