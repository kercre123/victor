/***************************************************************************
(C) Copyright 2015 Signal Essence; All Rights Reserved

   Module Name  - block_writer

   Author: Robert Yu
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "se_error.h"
#include "cjson.h"
#include <string.h>
#include <math.h>
#include <float.h>
#include "se_snprintf.h"
#include "block_writer.h"
#if defined(USE_META_MMIF) // use alternate mmif & sediag prototypes
#include "meta_mmif.h"
#else
#include "mmif.h"
#endif


/*
example json entry:

    "block_writer": {
        "dir_path": "./out",

        "rout": "sin.wav",
        "sout": "sout.wav"
    },

 */

//
// read configuration json file, return
// text in malloc'd array
static char *ReadConfigFile(char *pFname)
{
    FILE *pStrm = NULL;
    int len;
    char *pConfigText;
    size_t numRead;

    printf("ReadConfigFile:  reading (%s).......",pFname); 
    fflush(stdout);

    pStrm=fopen(pFname,"rb");
    if (pStrm==NULL)
    {
        printf("Can't open config file\n");
        fflush(stdout);
        SeAssert(NULL!=pStrm);
    }

    fseek(pStrm,0,SEEK_END);
    
    len = ftell(pStrm);
    fseek(pStrm,0,SEEK_SET);

    pConfigText = (char *)malloc(len + 1);
    SeAssert(NULL!=pConfigText);

    numRead = fread(pConfigText, 1, len, pStrm);
    SeAssert((int)numRead==len);
    fclose(pStrm);

    printf("done\n");
    return pConfigText;
}

//
// prepend prefix path onto filename, return result
static void ComposePathAndFname(char* pDirPrefix, char* pFnameIn, char* pFnameOut)
{
    int lenPrefix;
    int lenFname;
    memset(pFnameOut, 0, BW_LEN_FNAME * sizeof(char));
    lenPrefix = strlen(pDirPrefix);
    lenFname = strlen(pFnameIn);
    SeAssert(lenPrefix + lenFname < BW_LEN_FNAME);

    if (lenFname>0)
    {
        // strncpy:
        // No null-character is implicitly appended at the end of destination
        // if source is longer than num. Thus, in this case, destination shall
        //not be considered a null terminated C string (reading it as such would overflow).
        strncpy(pFnameOut, pDirPrefix, lenPrefix);
        strncpy(pFnameOut + lenPrefix, pFnameIn,    lenFname);
        pFnameOut[BW_LEN_FNAME-1] = '\0';   // just to be absolutely sure
    }
    else
    {
        //
        // empty FnameIn, so empty FnameOut 
        // (dont bother prepending path)
        pFnameOut[0] = '\0';
    }
}

//
// extract filename from json node, prepend directory path
/*
    "sin": {
        "fname" : "blah.wav"

    }
*/
static void ParseFname(cJSON *pNode, char *pDirPath, char *pPathAndFname)
{
    char *pFname;

    pFname = pNode->valuestring;
    SeAssert(strlen(pFname) < BW_LEN_FNAME);
    ComposePathAndFname(pDirPath, pFname, pPathAndFname);
}



/*
 return 0 = json node found
        1 = valid json node
*/
static int ParseJsonInputs(BlockWriter_t *p, cJSON *pJsonNode)
{
    cJSON *pNode0;
    cJSON *pNode1;
    char pDirPath[255];

    pNode0 = cJSON_GetObjectItem(pJsonNode, "block_writer");
    if (pNode0==NULL)
    {
        return 0;
    }

    //
    // read and format directory path
    memset(pDirPath, 0, sizeof(pDirPath));
    pNode1 = cJSON_GetObjectItem(pNode0, "dir_path");
    strncpy(pDirPath, pNode1->valuestring, 255);
    SeAssert(strlen(pDirPath) > 0);
    SeAssert(strlen(pDirPath) < 255);

    // append '/' if necessary
    if (pDirPath[strlen(pDirPath)-1] != '/')
    {
        pDirPath[strlen(pDirPath)] = '/';
    }
    
    pNode1 = cJSON_GetObjectItem(pNode0, "rout");
    if (pNode1)
    {
        ParseFname(pNode1, pDirPath, p->pFnameRout);
    }

    pNode1 = cJSON_GetObjectItem(pNode0, "sout");
    if (pNode1)
    {
        ParseFname(pNode1, pDirPath, p->pFnameSout);
    }


    return 1;
}

//
// read specified json file, parse parameters
// return 0 = invalid json file
//        1 = valid json
static int ParseJson(BlockWriter_t *p, char *pJsonFname)
{
    int isValidJson;
    cJSON *pNode;
    char *pConfigText;

    //
    // read text file into buffer and parse json
    pConfigText = ReadConfigFile(pJsonFname);
    pNode = cJSON_Parse(pConfigText);
    isValidJson = ParseJsonInputs(p, pNode);
    free(pConfigText);
    cJSON_Delete(pNode);  // can't just free() a cJSON node!

    return isValidJson;
}

//
// read json file, try to extract parameters
// return 1 if success
int BwInit(BlockWriter_t *p, char *pJsonFname)
{
    int isValidJson;

    memset(p, 0, sizeof(*p));
    isValidJson = ParseJson(p, pJsonFname);

    return isValidJson;
}


static void GetParamsFromMMIF(MMIfPortID_t port,
                              int *pNumChan,
                              int *pSampleRateHz,
                              int *pBlockSize)
{
    *pSampleRateHz = (int32)MMIfGetSampleRateHz(port);
    *pNumChan     = MMIfGetNumChans(port);
    *pBlockSize    = MMIfGetBlockSize(port);
}

static void SetLibSndInfo(SF_INFO *pSfInfo,
                          int numChan,
                          int sampleRateHz)
{
    pSfInfo->samplerate = sampleRateHz;
    pSfInfo->channels   = numChan;
    pSfInfo->format     = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
}

void BwOpenFiles(BlockWriter_t *p)
{
    char *pPathAndFname;

    SeAssertString(SEDiagIsInitialized()==1, "do not call BwOpenFiles until after MMIf is initialized");

    //
    // open rout 
    if (strlen(p->pFnameRout) > 0)
    {
        pPathAndFname = p->pFnameRout;
        printf("rout: trying to open (%s)...", pPathAndFname);
        GetParamsFromMMIF(PORT_RCV_ROUT,
                          &p->numChanRout,
                          &p->sampleRateHzRout,
                          &p->blockSizeRout);
        SetLibSndInfo(&p->sfInfoRout,
                      p->numChanRout,
                      p->sampleRateHzRout);

        p->pFileRout =  sf_open(pPathAndFname, SFM_WRITE, &(p->sfInfoRout));

        if (p->pFileRout==NULL)
        {
            printf("failed\n");
            SeAssert(0);
        }
        else
        {
            printf("success\n");
        }
    }
    else
    {
        p->pFileRout = NULL;
    }

    //
    // open sout
    if (strlen(p->pFnameSout) > 0)
    {
        pPathAndFname = p->pFnameSout;
        printf("sout: trying to open (%s)...", pPathAndFname);
        GetParamsFromMMIF(PORT_SEND_SOUT,
                          &p->numChanSout,
                          &p->sampleRateHzSout,
                          &p->blockSizeSout);

        SetLibSndInfo(&p->sfInfoSout,
                      p->numChanSout,
                      p->sampleRateHzSout);

        p->pFileSout =  sf_open(pPathAndFname, SFM_WRITE, &(p->sfInfoSout));

        if (p->pFileSout==NULL)
        {
            printf("failed\n");
            SeAssert(0);
        }
        else
        {
            printf("success\n");
        }
    }
    else
    {
        p->pFileSout = NULL;
    }

    p->filesAreOpen = 1;
}



// interleave and write
static void InterleaveAndWrite(SNDFILE *pFile,
                               int blockSize,
                               int numChan,
                               int16 *pSamps)
{
    int16 *pInterleaved;
    int chan;
    int sampIndex;
    int framesWritten;
    pInterleaved = (int16 *)calloc(numChan * blockSize, sizeof(int16));

    for (chan=0;chan<numChan;chan++)
    {
        for (sampIndex=0;sampIndex<blockSize;sampIndex++)  
        {
            /*
                  | a a a a | b b b b |

                       ||||||||
                       vvvvvvvvv
                        
                  | a | b | a | b | a | b | a | b|
             */
            pInterleaved[(numChan*sampIndex) + chan] = pSamps[(chan*blockSize) + sampIndex];
        }
    }
    framesWritten = (int)sf_writef_short(pFile, pInterleaved, (sf_count_t)blockSize);
    SeAssertString(framesWritten==blockSize, "sf_writef_short failed");
    free(pInterleaved);
}


//
// return 0==all samples read
//        1==EOF
void BwWriteSampleBlocks(BlockWriter_t *p,
                        int16 *pRout,
                        int16 *pSout)
{

    //
    // rout
    if (p->pFileRout != NULL)
    {
        InterleaveAndWrite(p->pFileRout,
                           p->blockSizeRout,
                           p->numChanRout,
                           pRout);
    }
    //
    // sout
    if (p->pFileSout != NULL)
    {
        InterleaveAndWrite(p->pFileSout,
                           p->blockSizeSout,
                           p->numChanSout,
                           pSout);
    }

}

void BwDestroy(BlockWriter_t *p)
{
    if (p->pFileRout)
    {
        sf_close(p->pFileRout);
    }
    if (p->pFileSout)
    {
        sf_close(p->pFileSout);
    }
}

