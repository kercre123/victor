/***************************************************************************
(C) Copyright 2015 Signal Essence; All Rights Reserved

   Module Name  - block_reader

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
#include "block_reader.h"
#if defined(USE_META_MMIF) // use alternate mmif & sediag prototypes
#include "meta_mmif.h"
#else
#include "mmif.h"
#endif

/*
example json entry:

the chanmap array [s0, s1, ...] means 
take the samples for channel 0 from the file's channel (s0),
and the samples for channel 1 from the file's channel (s1), etc

    "block_reader": {
        "dir_path": "../../../audio_data/xxx/160129-capture_noise_fan_on",

        "sin": {
            "fname": "sin.wav",
            "src_file_chan_index": [0, 1, 2, 3, 4, 5]
        },
        "refin": {
            "fname": "ref.wav",
            "src_file_chan_index": [0]
        },
        "rin": {
            "fname": "rin_48.wav",
            "src_file_chan_index": [0]
        }
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
    fflush(stdout);
    return pConfigText;
}

//
// prepend prefix path onto filename, return result
static void ComposePathAndFname(char* pDirPrefix, char* pFnameIn, char* pFnameOut)
{
    int lenPrefix;
    int lenFname;
    memset(pFnameOut, 0, BR_LEN_FNAME * sizeof(char));
    lenPrefix = strlen(pDirPrefix);
    lenFname = strlen(pFnameIn);
    SeAssert(lenPrefix + lenFname < BR_LEN_FNAME);

    if (lenFname>0)
    {
        // strncpy:
        // No null-character is implicitly appended at the end of destination
        // if source is longer than num. Thus, in this case, destination shall
        //not be considered a null terminated C string (reading it as such would overflow).
        strncpy(pFnameOut, pDirPrefix, lenPrefix);
        strncpy(pFnameOut + lenPrefix, pFnameIn,    lenFname);
        pFnameOut[BR_LEN_FNAME-1] = '\0';   // just to be absolutely sure
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
    cJSON *pNode0;
    pNode0 = cJSON_GetObjectItem(pNode, "fname");
    if (pNode0)
    {
        char *pFname;
        pFname = pNode0->valuestring;
        ComposePathAndFname(pDirPath, pFname, pPathAndFname);
    }
    else
    {
        pPathAndFname[0] = 0;
    }
}


/*
 extract channel map from json node
 new style
    {
        ...
        "src_file_chan_index": [0,1,2,3,4,5,6]
    }

*/
static void ParseChanMap(cJSON *pNode, int *pNumChan, int **ppSrcFileChanIndex)
{
    cJSON *pNode0;

    pNode0 = cJSON_GetObjectItem(pNode, "src_file_chan_index");
    if (pNode0==NULL)
    {
        // look for the alternative node name
        pNode0 = cJSON_GetObjectItem(pNode, "chanmap");
    }
    if (pNode0)
    {
        int m;

        *pNumChan = cJSON_GetArraySize(pNode0);
        *ppSrcFileChanIndex = (int *)malloc(*pNumChan * sizeof(int));
        SeAssert(NULL!=*ppSrcFileChanIndex);
        for (m=0;m<*pNumChan;m++)
        {
            cJSON *pNode1 = cJSON_GetArrayItem(pNode0, m);
            (*ppSrcFileChanIndex)[m] = pNode1->valueint;
        }
    }
}

static void ReadDirPath(cJSON *pNode0, char *pDirPath, int maxLen)
{
    cJSON *pNode1;

    pNode1 = cJSON_GetObjectItem(pNode0, "dir_path");
    memset(pDirPath, 0, maxLen * sizeof(*pDirPath));
    strncpy(pDirPath, pNode1->valuestring, maxLen);
    //SeAssert(strlen(pDirPath) > 0);
    SeAssert((int)strlen(pDirPath) < maxLen);

    // append '/' if necessary
    if (pDirPath[strlen(pDirPath)-1] != '/')
    {
        pDirPath[strlen(pDirPath)] = '/';
    }
}

//
// make a trivial channel mapping:
// file chan 0 -> input channel 0
static void MakeTrivialChanMap(int **ppSrcFileChanIndex)
{
    *ppSrcFileChanIndex = (int *)calloc(1, sizeof(int));
}

/*
    The old-style JSON specifies one mono-channel WAV file per microphone

    "inputs":
    {
        "dir_path": "../../../audio_data",

        "mmif_sin":
        [
            {"fname": "micChannel0.wav", "mic": 0},
            {"fname": "micChannel1.wav", "mic": 1},
            {"fname": "micChannel2.wav", "mic": 2},
            {"fname": "micChannel3.wav", "mic": 3}
        ],
        
        "rin_fname": "",
        "@comment":  "blah",
        "ref_in_fname": ""
    },
*/
static int ParseJsonInputsOldStyle(BlockReader_t *p, cJSON *pJsonNode)
{
    cJSON *pNode0;
    cJSON *pNode1;
    char pDirPath[255];

    pNode0 = cJSON_GetObjectItem(pJsonNode, "inputs");
    if (pNode0==NULL)
    {
        return 0;
    }

    p->useMultiMonoSinFiles = 1;  // old-style json specification

    ReadDirPath(pNode0, pDirPath, 255);

    //
    // for old sin specifications, read one filename per sin channel
    pNode1 = cJSON_GetObjectItem(pNode0, "mmif_sin");
    if (pNode1 != NULL)
    {
        int n;

        // one file per sin channel
        p->numSinFiles = cJSON_GetArraySize(pNode1);   
        SeAssert(p->numSinFiles <= MAX_NUM_SIN_FILES);
        
        for (n=0;n<p->numSinFiles;n++)
        {
            cJSON *pNode2;
            cJSON *pNode3;
            int destMic;

            pNode2 = cJSON_GetArrayItem(pNode1, n);
            SeAssert(pNode2 != NULL);

            /*
              this json node:
              {"fname": "mic.wav", "mic":3 }
              
              results in:
              ppFnameSin[3] = "dirpath/mic.wav"
            */
            pNode3 = cJSON_GetObjectItem(pNode2, "mic");
            destMic = pNode3->valueint;
            SeAssert((destMic >= 0) &&
                     (destMic < MAX_NUM_SIN_FILES));
            pNode3 = cJSON_GetObjectItem(pNode2, "fname");
            
            ComposePathAndFname(pDirPath,
                                pNode3->valuestring,  
                                p->ppFnameSin[destMic]);

        }
        
        // each sin file is single channel, so the mapping between
        // sin file and destination mic is determined by the sin array index
        MakeTrivialChanMap(&p->pSrcFileChanIndexSin);
    }
    
    pNode1 = cJSON_GetObjectItem(pNode0, "rin_fname");
    if (pNode1)
    {
        if (strlen(pNode1->valuestring) > 0)  // empty rin_fname is valid, in which skip it
        {
            ComposePathAndFname(pDirPath, pNode1->valuestring, p->pFnameRin);
            MakeTrivialChanMap(&p->pSrcFileChanIndexRin);
            p->lenChanMapRin = 1;
        }
    }

    pNode1 = cJSON_GetObjectItem(pNode0, "ref_in_fname");
    if (pNode1)
    {
        if (strlen(pNode1->valuestring) > 0)
        {
            ComposePathAndFname(pDirPath, pNode1->valuestring, p->pFnameRefIn);
            MakeTrivialChanMap(&p->pSrcFileChanIndexRefIn);
            p->lenChanMapRefIn = 1;
        }
    }

    return 1;
}

/*
 return 0 = no block_reader json node found
        1 = valid json node


"block_reader" : {
	"dir_path" : "../../../audio_data/hp_demo/voicebox-brussels/",
	
	"rin": {
		"fname": "voicebox-brussels-recording.wav"
		"src_file_chan_index": [0,1]
	},
	"refin" :   {	
	},
	"sin": {
		"fname": "voicebox_brussels_recording.wav",
		"src_file_chan_index": [0,1,2,3,4,5,6]
	}	
}
*/
static int ParseJsonInputs(BlockReader_t *p, cJSON *pJsonNode)
{
    cJSON *pNode0;
    cJSON *pNode1;
    char pDirPath[255];

    pNode0 = cJSON_GetObjectItem(pJsonNode, "block_reader");
    if (pNode0==NULL)
    {
        return 0;
    }

    ReadDirPath(pNode0, pDirPath, 255);

    // append '/' if necessary
    if (pDirPath[strlen(pDirPath)-1] != '/')
    {
        pDirPath[strlen(pDirPath)] = '/';
    }
    
    pNode1 = cJSON_GetObjectItem(pNode0, "rin");
    if (pNode1)
    {
        ParseFname(pNode1, pDirPath, p->pFnameRin);
        ParseChanMap(pNode1, &p->lenChanMapRin, &p->pSrcFileChanIndexRin);
    }

    pNode1 = cJSON_GetObjectItem(pNode0, "refin");
    if (pNode1)
    {
        ParseFname(pNode1, pDirPath, p->pFnameRefIn);
        ParseChanMap(pNode1, &p->lenChanMapRefIn, &p->pSrcFileChanIndexRefIn);
    }

    //
    // for "new" sin specifications, we are reading a single sin file
    pNode1 = cJSON_GetObjectItem(pNode0, "sin");
    if (pNode1)
    {
        p->numSinFiles = 1;
        ParseFname(pNode1, pDirPath, p->ppFnameSin[0]);
        ParseChanMap(pNode1, &p->lenChanMapSin, &(p->pSrcFileChanIndexSin));
    }

    return 1;
}

//
// parse config text as JSON, return BlockReader parameters
// returns:
// isValid
static int ParseConfigText(BlockReader_t *p, const char *pConfigText)
{
    int isValidJson;
    cJSON *pNode;

    pNode = cJSON_Parse(pConfigText);
    
    // try to parse json text
    isValidJson = ParseJsonInputs(p, pNode);
    if (isValidJson==0) // if that failed, try parsing the json for "old"-style specs.
    {
        isValidJson = ParseJsonInputsOldStyle(p, pNode);
    }

    cJSON_Delete(pNode);  // can't just free() a cJSON node!

    return isValidJson;
}

static void SetDefaultConfig(BlockReader_t *p)
{
    memset(p, 0, sizeof(*p));
    p->useMMIF=1;
}

//
// read json file, try to extract parameters
// return 1 if success
int BrInit(BlockReader_t *p, char *pJsonFname)
{
    int isValidJson;
    char *pConfigText;

    SetDefaultConfig(p);

    //
    // read text file into buffer and parse json
    pConfigText = ReadConfigFile(pJsonFname);
    isValidJson = ParseConfigText(p, pConfigText);
    free(pConfigText);

    return isValidJson;
}

/*
same as BrInit, except parse json in the char array pJsonText
*/
int BrInitFromJsonString(BlockReader_t *p, const char *pJsonText)
{
    int isValidJson;

    SetDefaultConfig(p);
    isValidJson = ParseConfigText(p, pJsonText);

    return isValidJson;
}

static void GetMMIfBlockSizes(BlockReader_t *p)
{
    p->blockSizeSin = MMIfGetBlockSize(PORT_SEND_SIN);
    SeAssert(p->blockSizeSin > 0);

    p->blockSizeRin = MMIfGetBlockSize(PORT_RCV_RIN);
    SeAssert(p->blockSizeRin > 0);

    p->blockSizeRefIn = MMIfGetBlockSize(PORT_SEND_REFIN);
    SeAssert(p->blockSizeRefIn > 0);
}

static void AssertValidSampleRate(int expectedSampleRateHz, int actualSampleRate)
{
    if (expectedSampleRateHz != actualSampleRate)
    {
        char pMsg[1000];
        sprintf(pMsg,"file sample rate (%d) != expected sample rate (%d)",actualSampleRate, expectedSampleRateHz);
        SeAssertString(0, pMsg);
    }
}

//
// query MMIF/sediag for expected sample rates
static void GetMMIfSampleRates(BlockReader_t *p)
{
    p->RinSampleRateHz = MMIfGetSampleRateHz(PORT_RCV_RIN);
    p->RefinSampleRateHz = MMIfGetSampleRateHz(PORT_SEND_REFIN);
    p->SinSampleRateHz = MMIfGetSampleRateHz(PORT_SEND_SIN);
}

//
// check file parameters (channels, samplerate) against
// MMIF input parameters (number of channels, samplerate)
//
// asserts if something goes wrong
static void BrValidateFileParamsWithMMIF(BlockReader_t *p)
{
    int mmifNumChans;
    int ok = 1;
    int m;
    int n;
    int numFilesToRead;
    int *pSrcFileChanIndex;
    SF_INFO *pSfInfo;

    // if sediag is initialized, then assume MMIF is initialized too
    SeAssert(SEDiagIsInitialized()==1);  
    
    SeAssert(p->filesAreOpen==1);

    GetMMIfBlockSizes(p);
    GetMMIfSampleRates(p);
    
    //
    // validate rin
    if (strlen(p->pFnameRin) > 0)
    {
        mmifNumChans = MMIfGetNumChans(PORT_RCV_RIN);
        numFilesToRead = (p->lenChanMapRin > 0);  // either 0 or 1
        pSrcFileChanIndex = p->pSrcFileChanIndexRin;
        pSfInfo = &p->sfInfoRin;
        SeAssert(pSfInfo != NULL);
        
        SeAssertString(p->lenChanMapRin == mmifNumChans, "number of channels in mapping != MMIF number of channels");
        // check that wav file will accomodate channel mapping
        SeAssert((numFilesToRead==0)||(numFilesToRead==1));
        for (m=0;m<mmifNumChans;m++)
        {
            ok |= (pSfInfo->channels > pSrcFileChanIndex[m]);
        }
        SeAssertString(ok==1, "channel map index exceeds number of channels in file");

        // check sample rate
        AssertValidSampleRate(p->RinSampleRateHz,pSfInfo->samplerate);

        // verify pcm
        SeAssertString(pSfInfo->format & SF_FORMAT_PCM_16, "file format is not 16-bit PCM");
    }

    //
    // validate refin
    if (strlen(p->pFnameRefIn) > 0)
    {
        mmifNumChans = MMIfGetNumChans(PORT_SEND_REFIN);
        numFilesToRead = (p->lenChanMapRefIn > 0); // either 0 or 1
        pSrcFileChanIndex = p->pSrcFileChanIndexRefIn;
        pSfInfo = &p->sfInfoRefIn;
        SeAssert(pSfInfo != NULL);
        
        SeAssertString(p->lenChanMapRefIn == mmifNumChans, "number of channels in mapping != MMIF number of channels");
        // check that wav file will accomodate channel mapping
        SeAssert((numFilesToRead==0)||(numFilesToRead==1));
        for (m=0;m<mmifNumChans;m++)
        {
            ok |= (pSfInfo->channels > pSrcFileChanIndex[m]);
        }
        SeAssertString(ok==1, "channel map index exceeds number of channels in file");

        AssertValidSampleRate(p->RefinSampleRateHz, pSfInfo->samplerate);

        SeAssertString(pSfInfo->format & SF_FORMAT_PCM_16, "file format is not 16-bit PCM");
    }

    // validate sin
    mmifNumChans = MMIfGetNumChans(PORT_SEND_SIN);
    pSrcFileChanIndex = p->pSrcFileChanIndexSin;

    // if single, multi-chan sin file, then
    // check that wav file will accomodate channel mapping
    if (p->useMultiMonoSinFiles==0)
    {
        numFilesToRead = (p->lenChanMapSin > 0); // either 0 or 1
        SeAssert((numFilesToRead==0)||(numFilesToRead==1));
        if (numFilesToRead > 0)
        {
            pSfInfo = &p->pSfInfoSinPerFile[0];
            SeAssert(pSfInfo != NULL);
            for (m=0;m<mmifNumChans;m++)
            {
                ok |= (pSfInfo->channels > pSrcFileChanIndex[m]);
            }
            SeAssertString(ok==1, "channel map index exceeds number of channels in file");
            SeAssertString(mmifNumChans==p->lenChanMapSin, "length of channel map != number of mics");
            AssertValidSampleRate(p->SinSampleRateHz, pSfInfo->samplerate);
        }
    }
    else // multiple mono-chan wav files
    {
        numFilesToRead = p->lenChanMapSin;  // 1 file per sin channel
        SeAssertString(numFilesToRead <= mmifNumChans, "# of sin files exceeds number of MMIF mics");
        
        for (n=0;n<numFilesToRead;n++)
        {
            if (strlen(p->ppFnameSin[n]) > 0)
            {
                pSfInfo = &p->pSfInfoSinPerFile[n];
                SeAssert(pSfInfo != NULL);
                SeAssertString(pSfInfo->channels == 1, "per-mic wav is not single channel");
                AssertValidSampleRate(p->SinSampleRateHz, pSfInfo->samplerate);
                SeAssertString(pSfInfo->format & SF_FORMAT_PCM_16, "file format is not 16-bit PCM");
            }
        }
    }
}

/*
validate block reader parameters without referring to MMIF
*/
static void BrValidateFileParams(BlockReader_t *p)
{
    int n;
    int numFilesToRead;
    SF_INFO *pSfInfo;

    SeAssert(p->filesAreOpen==1);

    //
    // validate rin
    if (strlen(p->pFnameRin) > 0)
    {
        pSfInfo = &p->sfInfoRin;
        SeAssert(pSfInfo != NULL);
        
        // set defacto samplerate
        p->RinSampleRateHz = pSfInfo->samplerate;

        // verify that user set blocksize
        SeAssertString(p->blockSizeRin > 0, "block size must be defined and > 0");

        // verify pcm
        SeAssertString(pSfInfo->format & SF_FORMAT_PCM_16, "file format is not 16-bit PCM");
    }

    //
    // validate refin
    if (strlen(p->pFnameRefIn) > 0)
    {
        numFilesToRead = (p->lenChanMapRefIn > 0); // either 0 or 1
        pSfInfo = &p->sfInfoRefIn;
        SeAssert(pSfInfo != NULL);
        
        // set defacto samplerate
        p->RefinSampleRateHz = pSfInfo->samplerate;

        // verify that user set blocksize
        SeAssertString(p->blockSizeRefIn > 0, "block size must be defined and > 0");
        
        SeAssertString(pSfInfo->format & SF_FORMAT_PCM_16, "file format is not 16-bit PCM");
    }

    // validate sin

    // if single, multi-chan sin file, then
    // check that wav file will accomodate channel mapping
    if (p->useMultiMonoSinFiles==0)
    {
        numFilesToRead = (p->lenChanMapSin > 0); // either 0 or 1
        SeAssert((numFilesToRead==0)||(numFilesToRead==1));
        if (numFilesToRead > 0)
        {
            pSfInfo = &p->pSfInfoSinPerFile[0];
            SeAssert(pSfInfo != NULL);

            // set defacto samplerate
            p->SinSampleRateHz = pSfInfo->samplerate;

            // verify that user set blocksize
            SeAssertString(p->blockSizeSin > 0, "block size must be defined and > 0");
        }
    }
    else // multiple mono-chan wav files
    {
        numFilesToRead = p->lenChanMapSin;  // 1 file per sin channel
        
        // verify that user set blocksize
        SeAssertString(p->blockSizeSin > 0, "block size must be defined and > 0");

        for (n=0;n<numFilesToRead;n++)
        {
            if (strlen(p->ppFnameSin[n]) > 0)
            {
                pSfInfo = &p->pSfInfoSinPerFile[n];
                SeAssert(pSfInfo != NULL);
                SeAssertString(pSfInfo->channels == 1, "per-mic wav is not single channel");
                // set defacto samplerate
                p->SinSampleRateHz = pSfInfo->samplerate;
                SeAssertString(pSfInfo->format & SF_FORMAT_PCM_16, "file format is not 16-bit PCM");
            }
        }
    }
}


/*
  Open the input files.

  If BlockReader.useMMIF = 1 (default), then
  query MMIF for the expected sample rates and block sizes.
  Otherwise, you must manually set the expected block sizes and sample rates
  in the BlockReader object--and thus skip the whole MMIF setup.
*/
void BrOpenFiles(BlockReader_t *p)
{
    char *pPathAndFname;
    int n;

    //
    // open sin file(s)
    for (n=0;n<p->numSinFiles;n++)
    {
        if (strlen(p->ppFnameSin[n]) > 0)
        {
            pPathAndFname = p->ppFnameSin[n];
            printf("sin: trying to open (%s)...", pPathAndFname);fflush(stdout);
            p->ppFileSinArray[n] =  sf_open(pPathAndFname, SFM_READ, &(p->pSfInfoSinPerFile[n]));
            
            if (p->ppFileSinArray[n]==NULL)
            {
                printf("failed\n");
                fflush(stdout);
                SeAssert(0);
            }
            else
            {
                printf("success\n");
                fflush(stdout);
            }
        }
        else
        {
            p->ppFileSinArray[n] = NULL;
        }
    }

    //
    // open rin
    if (strlen(p->pFnameRin) > 0)
    {
        pPathAndFname = p->pFnameRin;
        printf("rin: trying to open (%s)...", pPathAndFname);fflush(stdout);
        p->pFileRin =  sf_open(pPathAndFname, SFM_READ, &(p->sfInfoRin));

        if (p->pFileRin==NULL)
        {
            printf("failed\n");
            fflush(stdout);
            SeAssert(0);
        }
        else
        {
            printf("success\n");
            fflush(stdout);
        }
    }
    else
    {
        p->pFileRin = NULL;
    }

    //
    // open refIn
    // first check if refIn is even specified
    if (strlen(p->pFnameRefIn) > 0)
    {
        pPathAndFname = p->pFnameRefIn;
        printf("refin: trying to open (%s)...", pPathAndFname);fflush(stdout);
        p->pFileRefIn =  sf_open(pPathAndFname, SFM_READ, &(p->sfInfoRefIn));

        if (p->pFileRefIn==NULL)
        {
            printf("failed\n");
            fflush(stdout);
            SeAssert(0);
        }
        else
        {
            printf("success\n");
            fflush(stdout);
        }
    }
    else
    {
        p->pFileRefIn = NULL;
    }

    p->filesAreOpen = 1;

    if (p->useMMIF)
    {
        BrValidateFileParamsWithMMIF(p);
    }
    else
    {
        BrValidateFileParams(p);
    }
}

//
// read sample blocks from  N mono-chan WAV files,
// concatenate blocks together
//
// returns 1 if EOF
static int  ReadMultipleMonoSinFiles(SNDFILE **ppFileSinArray,
                                     SF_INFO *pSfInfoSinPerFile,
                                     int numChanSin,
                                     int blockSize,
                                     int16 *pSamps)
{
    int n;
    int isEof = 0;

    SeAssert(numChanSin <= MAX_NUM_SIN_FILES);

    //
    // for each sin channel, either read the file or generate zeros
    for (n=0;n<numChanSin;n++)
    {
        sf_count_t count;
        int destOffset;
        destOffset = n * blockSize;

        if (ppFileSinArray[n] != NULL)
        {
            SeAssertString(pSfInfoSinPerFile[n].channels == 1, "WAV file is not mono-channel");
            count = sf_read_short(ppFileSinArray[n], 
                                  pSamps + destOffset,
                                  (sf_count_t)blockSize);
            isEof |= (count != blockSize);
        }
        else
        {
            // no file corresponding to this sin chan, so fill with zeros
            memset(pSamps + destOffset, 0, sizeof(*pSamps) * blockSize);
        }
    }
    return isEof;
}

//
// read sample blocks from one multi-chan WAV file,
// re-order blocks to match mic ordering according to channel map
//
// returns 1 if EOF or some other problem
static int ReadAndDeinterleave(SNDFILE *pFile,
                               SF_INFO *pSfInfo,
                               int blockSize,
                               int lenChanMap,
                               int* pSrcFileChanIndex,
                               int16 *pSamps)
{
    int16 *pAllChans;
    sf_count_t count;
    int chan;
    int sampIndex;
    int numSampsToRead;
    int isEof;

    if (pFile==NULL)
    {
        // no file, so fill buffer with zeros
        memset(pSamps, 0, sizeof(*pSamps) * lenChanMap * blockSize);
        isEof = 0;
    }
    else
    {
        SeAssert(blockSize > 0);

        pAllChans = (int16 *)calloc(pSfInfo->channels * blockSize, sizeof(int16));
        SeAssert(NULL!=pAllChans);

        //
        // read a bunch of interleaved samples
        numSampsToRead = pSfInfo->channels * blockSize;
        SeAssert(numSampsToRead > 0);
        count = sf_read_short(pFile, pAllChans, (sf_count_t)numSampsToRead);
        isEof = (count != numSampsToRead);

        // deinterleave
        for (chan=0;chan<lenChanMap;chan++)
        {
            int destBlockStart;
            destBlockStart = chan * blockSize;
            for (sampIndex=0;sampIndex < blockSize; sampIndex++)
            {
                int srcBlockStart = (sampIndex * pSfInfo->channels);
                int sampOffset = pSrcFileChanIndex[chan];

                SeAssert(pSfInfo->channels > sampOffset);  // desired channel index does not exceed actual number of file channels
                SeAssert(srcBlockStart + sampOffset >= 0);

                pSamps[destBlockStart + sampIndex] = pAllChans[srcBlockStart + sampOffset];
            }
        }
        free(pAllChans);
    }

    return isEof;
}

//
// deal with sin file(s):
// we either have 1 multi-chan WAV file, or N single-chan WAV files
static int ReadSin(SNDFILE **ppFileSinArray,
                   SF_INFO *pSfInfoSinPerFile,
                   int useMultiMonoSinFiles,
                   int numChanSin,
                   int blockSize,
                   int lenChanMapSin,
                   int* pSrcFileChanIndexSin,
                   int16 *pSamps)
{
    int isEof = 0;
    //
    // "old" style: one mono-channel wav file per sin channel
    if (useMultiMonoSinFiles==1)
    {
        //
        // read N files and compose samples into a single block
        // or if file ptr is NULL, fills sample buffer with zeros
        isEof = ReadMultipleMonoSinFiles(ppFileSinArray,
                                         pSfInfoSinPerFile,
                                         numChanSin,
                                         blockSize,
                                         pSamps);
    }
    else // "new" style: single, multichannel WAV file
    {
        isEof = ReadAndDeinterleave(ppFileSinArray[0],
                                    &pSfInfoSinPerFile[0],
                                    blockSize,
                                    lenChanMapSin,
                                    pSrcFileChanIndexSin,
                                    pSamps);
        
    }
    return isEof;
}

//
// skip first N samples of each input file
void BrSkipSamples(BlockReader_t *p, int numSamplesToSkip)
{
    sf_count_t ret;
    int n;

    // skip rin
    if (p->pFileRin != NULL)
    {
        ret = sf_seek(p->pFileRin, numSamplesToSkip, SEEK_SET);
        SeAssert(ret != -1);
    }

    // skip refin
    if (p->pFileRefIn != NULL)
    {
        ret = sf_seek(p->pFileRefIn, numSamplesToSkip, SEEK_SET);
        SeAssert(ret != -1);
    }

    // skip sin (possibly multiple files)
    for (n=0;n<p->numSinFiles;n++)
    {
        if (p->ppFileSinArray[n] != NULL)
        {
            ret = sf_seek(p->ppFileSinArray[n], numSamplesToSkip, SEEK_SET);
            SeAssert(ret != -1);
        }
    }
}


//
// skip the first N seconds of each input file
void BrSkipSeconds(BlockReader_t *p, float32 skipSec)
{
    float32 sampleRateHz;
    int numSamplesToSkip;
    sf_count_t ret;
    int n;

    // skip rin
    if (p->pFileRin != NULL)
    {
        sampleRateHz = p->RinSampleRateHz;
        SeAssert(sampleRateHz > 0.0f);
        numSamplesToSkip = (int)(skipSec * sampleRateHz + 0.5);
        ret = sf_seek(p->pFileRin, numSamplesToSkip, SEEK_SET);
        SeAssert(ret != -1);
    }

    // skip refin
    if (p->pFileRefIn != NULL)
    {
        sampleRateHz = p->RefinSampleRateHz;
        SeAssert(sampleRateHz > 0.0f);
        numSamplesToSkip = (int)(skipSec * sampleRateHz + 0.5);
        ret = sf_seek(p->pFileRefIn, numSamplesToSkip, SEEK_SET);
        SeAssert(ret != -1);
    }

    // skip sin (possibly multiple files)
    sampleRateHz = p->SinSampleRateHz;
    SeAssert(sampleRateHz > 0.0f);
    numSamplesToSkip = (int)(skipSec * sampleRateHz + 0.5);
    for (n=0; n<MAX_NUM_SIN_FILES; n++)  // e.g. MMIF supports 16 sin channels, but "old style" supports only 7 sin files
    {
        if (p->ppFileSinArray[n] != NULL)
        {
            ret = sf_seek(p->ppFileSinArray[n], numSamplesToSkip, SEEK_SET);
            SeAssert(ret != -1);
        }
    }
}

/*
for pRinSamps,
    pRefInSamps,
    pSinSamps,
pass NULL if you don't want to read anything

return 0==all samples read
       1==EOF
*/
int BrGetSampleBlocks(BlockReader_t *p,
                      int16 *pRinSamps,
                      int16 *pRefInSamps,
                      int16 *pSinSamps)
{
    int isEof = 0;


    //
    // read sin
    if (pSinSamps)
    {
        int numChanSin;
        if (p->useMultiMonoSinFiles==1)
        {
            numChanSin = p->numSinFiles;
        }
        else
        {
            numChanSin = p->lenChanMapSin; //MMIfGetNumChans(PORT_SEND_SIN);
        }
        SeAssert(p->blockSizeSin > 0);
        isEof |= ReadSin(p->ppFileSinArray,
                         p->pSfInfoSinPerFile,
                         p->useMultiMonoSinFiles,
                         numChanSin,
                         p->blockSizeSin,
                         p->lenChanMapSin,
                         p->pSrcFileChanIndexSin,
                         pSinSamps);
    }
    
    //
    // read rin
    if (pRinSamps)
    {
        SeAssert(p->blockSizeRin > 0);
        isEof |= ReadAndDeinterleave(p->pFileRin,
                                     &p->sfInfoRin,
                                     p->blockSizeRin,
                                     p->lenChanMapRin,
                                     p->pSrcFileChanIndexRin,
                                     pRinSamps);
    }
    
    //
    // read refIn
    if (pRefInSamps)
    {
        SeAssert(p->blockSizeRefIn > 0);
        isEof |= ReadAndDeinterleave(p->pFileRefIn,
                                     &p->sfInfoRefIn,
                                     p->blockSizeRefIn,
                                     p->lenChanMapRefIn,
                                     p->pSrcFileChanIndexRefIn,
                                     pRefInSamps);
    }
    return isEof;
}

void BrDestroy(BlockReader_t *p)
{
    free(p->pSrcFileChanIndexRin);
    free(p->pSrcFileChanIndexRefIn);
    free(p->pSrcFileChanIndexSin);

    if (p->pFileRin)
    {
        sf_close(p->pFileRin);
    }
    if (p->pFileRefIn)
    {
        sf_close(p->pFileRefIn);
    }
    if (p->ppFileSinArray[0])
    {
        sf_close(p->ppFileSinArray[0]);
    }
}

