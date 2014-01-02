///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     shave management functions and pointers
///

#include <stdio.h>
#include "DrvSvu.h"
#include "DrvTimer.h"
#include "assert.h"

#include "aacEncoderApi.h"
#include "shared.h"
#include "ioApi.h"
#include "swcLeonUtils.h"
#include "swcShaveLoader.h"

u32 usedShave;


// DRAM I/O buffers
unsigned char __attribute__((section(".ddr_audio.data"))) ddr_aacbuf_mem[AAC_BUFF_NR][2048];
unsigned char __attribute__((section(".ddr_audio.data"))) ddr_pcmbuf_mem[PCM_BUFF_NR][4096];



// shave start point
void              **SVE__startEncoder;

// buffer flag pointers
volatile int    (*SVE__used_pcmbuf)[PCM_BUFF_NR];
volatile int    (*SVE__used_aacbuf)[AAC_BUFF_NR];

// buffer pointers
unsigned char    *(*SVE__aacbuf)[AAC_BUFF_NR];
unsigned int     *(*SVE__pcmbuf)[PCM_BUFF_NR];

// encoder configuration shave pointers
unsigned long      *SVE__sampleRate;
unsigned long      *SVE__channelNumber;
unsigned long      *SVE__byteRate;

unsigned long      *SVE__pcmFrameSize;
int                *SVE__currentFrame;
unsigned long      *SVE__totalBytesAAC;

enum AACSTATUSFLAG *SVE__aacStatusFlag;


// buffer flags
volatile int  *shave_used_pcmbuf;
volatile int  *shave_used_aacbuf;

unsigned long *shave_pcmFrameSize;
int           *shave_currentFrame;
unsigned long *shave_totalBytesAAC;

volatile enum AACSTATUSFLAG *shave_statusFlag;

//monitoring flags
tyTimeStamp   tStamp;
u64 cycles;

unsigned int  writtenBytesTotal;
unsigned int  readBytesTotal;

unsigned long  localSampleRate;
unsigned short localChannelNumber;
unsigned long  localBytesPerSec;

void shaveSelectEncoder(unsigned int target)
{
    // shave start point
    extern void              *aacEncode_asmopt0_startEncoder;

    // buffer flag pointers
    extern volatile int    aacEncode_asmopt0_used_pcmbuf[PCM_BUFF_NR];
    extern volatile int    aacEncode_asmopt0_used_aacbuf[AAC_BUFF_NR];

    // buffer pointers
    extern unsigned char    *aacEncode_asmopt0_aacbuf[AAC_BUFF_NR];
    extern unsigned int     *aacEncode_asmopt0_pcmbuf[PCM_BUFF_NR];

    // encoder configuration shave pointers
    extern unsigned long      aacEncode_asmopt0_sampleRate;
    extern unsigned long      aacEncode_asmopt0_channelNumber;
    extern unsigned long      aacEncode_asmopt0_byteRate;

    extern unsigned long      aacEncode_asmopt0_pcmFrameSize;
    extern int                aacEncode_asmopt0_currentFrame;
    extern unsigned long      aacEncode_asmopt0_totalBytesAAC;

    extern enum AACSTATUSFLAG aacEncode_asmopt0_aacStatusFlag;

    // shave start point
    extern void              *aacEncode_noasm1_startEncoder;

    // buffer flag pointers
    extern volatile int    aacEncode_noasm1_used_pcmbuf[PCM_BUFF_NR];
    extern volatile int    aacEncode_noasm1_used_aacbuf[AAC_BUFF_NR];

    // buffer pointers
    extern unsigned char    *aacEncode_noasm1_aacbuf[AAC_BUFF_NR];
    extern unsigned int     *aacEncode_noasm1_pcmbuf[PCM_BUFF_NR];

    // encoder configuration shave pointers
    extern unsigned long      aacEncode_noasm1_sampleRate;
    extern unsigned long      aacEncode_noasm1_channelNumber;
    extern unsigned long      aacEncode_noasm1_byteRate;

    extern unsigned long      aacEncode_noasm1_pcmFrameSize;
    extern int                aacEncode_noasm1_currentFrame;
    extern unsigned long      aacEncode_noasm1_totalBytesAAC;

    extern enum AACSTATUSFLAG aacEncode_noasm1_aacStatusFlag;


    assert(target >= 0 && target <= 7);

    usedShave = target;


    switch(target) {
    case 0:
        // shave start point
        SVE__startEncoder   = &aacEncode_asmopt0_startEncoder;

        // buffer flag pointers
        SVE__used_pcmbuf    = &aacEncode_asmopt0_used_pcmbuf;
        SVE__used_aacbuf    = &aacEncode_asmopt0_used_aacbuf;

        // buffer pointers
        SVE__aacbuf         = &aacEncode_asmopt0_aacbuf;
        SVE__pcmbuf         = &aacEncode_asmopt0_pcmbuf;

        // encoder configuration shave pointers
        SVE__sampleRate     = &aacEncode_asmopt0_sampleRate;
        SVE__channelNumber  = &aacEncode_asmopt0_channelNumber;
        SVE__byteRate       = &aacEncode_asmopt0_byteRate;

        SVE__pcmFrameSize   = &aacEncode_asmopt0_pcmFrameSize;
        SVE__currentFrame   = &aacEncode_asmopt0_currentFrame;
        SVE__totalBytesAAC  = &aacEncode_asmopt0_totalBytesAAC;

        SVE__aacStatusFlag  = &aacEncode_asmopt0_aacStatusFlag;
        break;
    case 1:
        // shave start point
        SVE__startEncoder   = &aacEncode_noasm1_startEncoder;

        // buffer flag pointers
        SVE__used_pcmbuf    = &aacEncode_noasm1_used_pcmbuf;
        SVE__used_aacbuf    = &aacEncode_noasm1_used_aacbuf;

        // buffer pointers
        SVE__aacbuf         = &aacEncode_noasm1_aacbuf;
        SVE__pcmbuf         = &aacEncode_noasm1_pcmbuf;

        // encoder configuration shave pointers
        SVE__sampleRate     = &aacEncode_noasm1_sampleRate;
        SVE__channelNumber  = &aacEncode_noasm1_channelNumber;
        SVE__byteRate       = &aacEncode_noasm1_byteRate;

        SVE__pcmFrameSize   = &aacEncode_noasm1_pcmFrameSize;
        SVE__currentFrame   = &aacEncode_noasm1_currentFrame;
        SVE__totalBytesAAC  = &aacEncode_noasm1_totalBytesAAC;

        SVE__aacStatusFlag  = &aacEncode_noasm1_aacStatusFlag;
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    case 6:
        break;
    case 7:
        break;
    default:
        break;
    }

}

/// calculate adreses of bufers and various variables
void shaveCalcAddress()
{
    unsigned int   **shave_pcmbuf[PCM_BUFF_NR];
    unsigned char  **shave_aacbuf[AAC_BUFF_NR];
    int i;

    //get flag locations
    shave_used_pcmbuf = (unsigned int *)swcSolveShaveRelAddrAHB((u32)SVE__used_pcmbuf,usedShave);
    //set buffer locations
    for(i = 0; i < PCM_BUFF_NR; ++i)
    {
        shave_pcmbuf[i]     = (unsigned int **)swcSolveShaveRelAddrAHB((u32)&((*SVE__pcmbuf)[i]),usedShave);
        *shave_pcmbuf[i]    = (unsigned int *) ddr_pcmbuf_mem[i];
    }

    //get flag locations
    shave_used_aacbuf = (unsigned int *)swcSolveShaveRelAddrAHB((u32)*SVE__used_aacbuf,usedShave);
    //set buffer locations
    for(i = 0; i < AAC_BUFF_NR; ++i)
    {
        shave_aacbuf[i]  = (unsigned char **)swcSolveShaveRelAddrAHB((u32)&((*SVE__aacbuf)[i]),usedShave);
        *shave_aacbuf[i] = (unsigned char *)ddr_aacbuf_mem[i];
    }

    //get various variable locations
    shave_currentFrame      = (int *)swcSolveShaveRelAddrAHB((u32)SVE__currentFrame,usedShave);
    shave_pcmFrameSize = (unsigned long *)swcSolveShaveRelAddrAHB((u32)SVE__pcmFrameSize, usedShave);
    shave_statusFlag       = (enum AACSTATUSFLAG *)swcSolveShaveRelAddrAHB((u32)SVE__aacStatusFlag, usedShave);
    shave_totalBytesAAC    = (unsigned long *)swcSolveShaveRelAddrAHB(((u32)SVE__totalBytesAAC),usedShave);
}

/// Send the parameters of the stream
void shaveConfigureEncoder(
        unsigned long sampleRate,
        unsigned short channelNumber,
        unsigned long bytesPerSec
)
{
    //check if shave selected, if not, select first
    if(SVE__startEncoder == 0)
        shaveSelectEncoder(0);

    unsigned long *shave_sampleRate;
    unsigned long *shave_channelNumber;
    unsigned long *shave_byteRate;

    shave_sampleRate = (unsigned long *) swcSolveShaveRelAddrAHB((u32)SVE__sampleRate, usedShave);
    shave_channelNumber = (unsigned long *) swcSolveShaveRelAddrAHB((u32) SVE__channelNumber, usedShave);
    shave_byteRate = (unsigned long *) swcSolveShaveRelAddrAHB((u32) SVE__byteRate, usedShave);

    *shave_sampleRate    = sampleRate;
    *shave_channelNumber = channelNumber;
    *shave_byteRate      = bytesPerSec;

    localSampleRate = sampleRate;
    localChannelNumber = channelNumber;
    localBytesPerSec = bytesPerSec;

    // calculate
    shaveCalcAddress();
}

/// Start the shave which does the encoding
int shaveRun()
{
    int res;

    int aac_frame = 0;
    int pcm_frame = 0;

    unsigned int  writtenBytes;
    unsigned int  readBytes;

    printf("Start encoding ...\n");
    //init shave stack
    DrvSvutIrfWrite(usedShave, 19, 0x1c020000);
    //start shave
    swcStartShave(usedShave,(u32)SVE__startEncoder);

    DrvTimerStart(&tStamp);

    //run while shave is encoding and there are full output buffers
    for(;swcShaveRunning(usedShave) || shave_used_aacbuf[aac_frame%AAC_BUFF_NR] != -1;)
    {
        //if there are pcm frames to be fetched
        if ( ( pcm_frame >= swcLeonReadNoCacheI32(shave_currentFrame) ) &&
                ( swcLeonReadNoCacheI32( shave_used_pcmbuf + pcm_frame%PCM_BUFF_NR ) == -1 ) )
        {
            //get input stream
            res = getStream(
                    (void *)ddr_pcmbuf_mem[pcm_frame%PCM_BUFF_NR],
                    *shave_pcmFrameSize,
                    &readBytes);

            // mark buffer ready for processing
            shave_used_pcmbuf[pcm_frame%PCM_BUFF_NR] = readBytes;
            readBytesTotal += readBytes;
            ++pcm_frame;
        }

        //if there are aac frames to be written
        if ( ( aac_frame < swcLeonReadNoCacheI32 (shave_currentFrame) )
                &&	( swcLeonReadNoCacheI32( shave_used_aacbuf + aac_frame%AAC_BUFF_NR ) != -1 ))
        {
            //write output stream
            res = writeStream(
                    (void *)ddr_aacbuf_mem[aac_frame%AAC_BUFF_NR],
                    shave_used_aacbuf[aac_frame%AAC_BUFF_NR],
                    &writtenBytes);

            // mark buffer free
            shave_used_aacbuf[aac_frame%AAC_BUFF_NR] = -1;
            writtenBytesTotal += writtenBytes;
            ++aac_frame;
        }
    }
    //encoding finished
    cycles = DrvTimerElapsedTicks(&tStamp);
    printf("Finished encoding ...\n");

    switch(*shave_statusFlag)
    {

    case FAILURE:
    {
        printf(" !!!!!! encoding aac stream failed !!!!!!");
        return 0;
    }break;

    case CONFIG_FAILURE:
    {
        printf(" !!!!!! configuring aac encoder failed !!!!!!");
        return 0;
    }break;

    case IDLE:
    {
        printf(" !!!!!! aac encoder was not started !!!!!!");
        return 0;
    }break;

    case SUCCESS:
    {
        // check write mismatch
        if(*shave_totalBytesAAC != writtenBytesTotal)
            printf("Mismatch: encoder output = %d bytes | written bytes = %d\n", *shave_totalBytesAAC, writtenBytesTotal);
        // notify result
        printf("\n Wrote >> %d audio frames === %d bytes\n", aac_frame, writtenBytesTotal);

        return 1;
    }break;
    default:
        return 0;

    }

    return 1;
}



void showReport()
{
    float msTime = (float)DrvTimerTicksToMs(cycles);
    float audioLength = 1;

    //TODO : here
    audioLength = ((float)readBytesTotal / ( (float)localSampleRate * (float)localChannelNumber * (float)localBytesPerSec));

    printf("\nReport:\n");
    printf("Algorithm total time:       %d ms\n", (u32)msTime);
    printf("Algorithm time / frame:     %d ms\n", (u32)(msTime / *shave_currentFrame));
    printf("Algorithm time / real time: 0.%d \n", (u32)(msTime / audioLength));

    printf("\n\n");
}





