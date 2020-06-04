#ifdef __cplusplus
extern "C" {
#endif

/*
*************************************************************************
 (C) Copyright 2015 Signal Essence; All Rights Reserved
  
  Module Name  - block_reader
  
  Author: Robert Yu
**************************************************************************
*/

#ifndef block_reader_h
#define block_reader_h

#include "cjson.h"
#include "se_types.h"
#include <stdlib.h>
#include "sndfile.h"

//
// br  = block reader
//
#define BR_LEN_FNAME 255
#define MAX_NUM_SIN_FILES 7 // arbitrary number, but used only by old-style declarations
typedef struct
{
    int filesAreOpen;
    int useMMIF;
    
    char ppFnameSin[MAX_NUM_SIN_FILES][BR_LEN_FNAME];
    char pFnameRin[BR_LEN_FNAME];
    char pFnameRefIn[BR_LEN_FNAME];

    int lenChanMapRin;      // e.g. 2
    int *pSrcFileChanIndexRin;    // e.g. [4,5] -> wav file channel 4 -> rin0, chan 5-> rin1
    int blockSizeRin;

    int lenChanMapRefIn;    
    int *pSrcFileChanIndexRefIn;
    int blockSizeRefIn;

    int useMultiMonoSinFiles; // use old-style json specification
    int numSinFiles;
    int lenChanMapSin;
    int *pSrcFileChanIndexSin;
    int blockSizeSin;

    // file pointers
    SNDFILE *(ppFileSinArray[MAX_NUM_SIN_FILES]);
    SF_INFO pSfInfoSinPerFile[MAX_NUM_SIN_FILES];

    SNDFILE  *pFileRin;
    SF_INFO sfInfoRin;

    SNDFILE  *pFileRefIn;
    SF_INFO sfInfoRefIn;

    // parameters from MMIF
    float32 RinSampleRateHz;
    float32 RefinSampleRateHz;
    float32 SinSampleRateHz;
    
} BlockReader_t;

int BrInit(BlockReader_t *p, char *pJsonFname);
int BrInitFromJsonString(BlockReader_t *p, const char *pJsonText);

void BrOpenFiles(BlockReader_t *p);

int BrGetSampleBlocks(BlockReader_t *p,
                      int16 *pRinSamps,
                      int16 *pRefInSamps,
                      int16 *pSinSamps);
void BrSkipSeconds(BlockReader_t *p, float32 skipSec);
void BrSkipSamples(BlockReader_t *p, int numSamplesToSkip);

void BrDestroy(BlockReader_t *p);

#endif  // ifndef

#ifdef __cplusplus
}
#endif

