#ifdef __cplusplus
extern "C" {
#endif

/*
*************************************************************************
 (C) Copyright 2015 Signal Essence; All Rights Reserved
  
  Module Name  - block_writer
  
  Author: Robert Yu
**************************************************************************
*/

#ifndef block_writer_h
#define block_writer_h

#include "cjson.h"
#include "se_types.h"
#include <stdlib.h>
#include "sndfile.h"

//
// bw  = block reader
//
#define BW_LEN_FNAME 255

typedef struct
{
    int filesAreOpen;

    char pFnameRout[BW_LEN_FNAME];
    char pFnameSout[BW_LEN_FNAME];


    int numChanRout;
    int blockSizeRout;
    int sampleRateHzRout;

    int numChanSout;    
    int blockSizeSout;
    int sampleRateHzSout;

    // file pointers
    SNDFILE *pFileRout;
    SF_INFO sfInfoRout;

    SNDFILE  *pFileSout;
    SF_INFO sfInfoSout;

} BlockWriter_t;

int BwInit(BlockWriter_t *p, char *pJsonFname);

void BwOpenFiles(BlockWriter_t *p);

void BwWriteSampleBlocks(BlockWriter_t *p,
                        int16 *pRout,
                        int16* pSout);

void BwDestroy(BlockWriter_t *p);

#endif  // ifndef

#ifdef __cplusplus
}
#endif

