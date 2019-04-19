#include "block_reader.h"
#include "svad.h"
#include <assert.h>
#include <string.h>

#define BLOCKSIZE_SIN   160
#define NUM_MICS 7

#define NUM_STATS 1000
typedef struct {
    int     blockIndex;
    int     Activity[NUM_STATS];
    int     ReturnValue[NUM_STATS];
    float   ActivityRatio[NUM_STATS];
    float   AvePowerInBlock[NUM_STATS];
    float   PowerTrk[NUM_STATS];
}
VadStats_t;

VadStats_t VadStats;

/*
write collected statistics as CSV
*/
static void PrintReportCsv(const VadStats_t *pStats)
{
    int n;
    
    printf("%15s, %15s, %15s, %15s, %15s\n",
           "blockindex",
           "activity",
           "activityratio",
           "avepowerinblock",
           "powertrk");
    for (n=0;n<pStats->blockIndex;n++)
    {
        printf("%15d, %15d, %15.2f, %15.2f, %15.2f\n",
               n,
               pStats->Activity[n],
               pStats->ActivityRatio[n],
               pStats->AvePowerInBlock[n],
               pStats->PowerTrk[n]);
    }
}

int main()
{
    BlockReader_t blockReader;
    int isEof;
    int16 sinPtr[BLOCKSIZE_SIN * NUM_MICS];
    int blockIndex=0;
    SVadConfig_t SVadConfig;
    SVadObject_t SVadObj;

    /* set up VAD */
    SVadSetDefaultConfig(&SVadConfig, BLOCKSIZE_SIN, 16000.0f);
    SVadConfig.AbsThreshold = 400.0f;
    SVadConfig.HangoverCountDownStart = 25;  // make 25 blocks (1/4 second) to see it actually end a couple times
    SVadInit(&SVadObj, &SVadConfig);

    /* set up file reader */
    BrInit(&blockReader, "config.json");
    blockReader.useMMIF = 0;
    blockReader.blockSizeSin = 160;
    BrOpenFiles(&blockReader);
    isEof = 0;
    while(isEof==0)
    {
        int activityFlag;
        
        isEof = BrGetSampleBlocks(&blockReader, 
                                  NULL,
                                  NULL,
                                  sinPtr);

        // call VAD
        activityFlag = DoSVad(  &SVadObj,    // object
                                1.0f,        // confidence it is okay to measure noise floor, i.e. no known activity like gear noise
                                sinPtr);     // pointer to input data

        // save record of interesting diagnostics
        VadStats.ReturnValue[blockIndex] = activityFlag;
        VadStats.blockIndex = blockIndex;
        VadStats.Activity[blockIndex] = SVadObj.Activity;
        VadStats.ActivityRatio[blockIndex] = SVadObj.ActivityRatio;
        VadStats.AvePowerInBlock[blockIndex] = SVadObj.AvePowerInBlock;
        VadStats.PowerTrk[blockIndex] = SVadObj.PowerTrk;

        if (blockIndex % 100 == 0) 
        {
            printf(".");fflush(stdout);
        }
        blockIndex++;
    }
    printf("\n");

    BrDestroy(&blockReader);

    PrintReportCsv(&VadStats);
    return 0;
}
