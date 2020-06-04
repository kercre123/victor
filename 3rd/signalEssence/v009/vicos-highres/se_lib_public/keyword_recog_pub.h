#ifdef __cplusplus
extern "C" {
#endif

/* 
**************************************************************************
  (C) Copyright 2015 Signal Essence; All Rights Reserved
  
  Module Name  - keyword recognizer
  
  Author: Robert Yu
**************************************************************************
*/

#ifndef KEYWORD_RECOG_PUB_H
#define KEYWORD_RECOG_PUB_H
#include "se_types.h"
#include "sampledelayqueue.h"
#include "mmglobalsizes.h"
#include "tap_point.h"

#define KEYWORD_MAX_NUM_PROFILES 5

//
// if we implement additional recognition engines, then these config fields
// should be wrapped up in a separate struct and all the structs should be
// embedded in a union struct, as in meta_aec_pub.h.
typedef struct {
    int32 *(pProfileData[KEYWORD_MAX_NUM_PROFILES]);   // array of pointers to int
    int pLenProfile[KEYWORD_MAX_NUM_PROFILES];
    float32 threshold;
    int blockSize;
    float32 inputGain;
    int KeywordLenSampleQueue;
} KeywordConfig_t;

/*
general interface for keyword recognizer

define a structure of callback functions to support
different keyword recognizer implementations
*/
typedef void     (*KeywordInit_t)             (KeywordConfig_t *pCfg, void *pState);
typedef void     (*KeywordDetect_t)           (void *pState, int16 *pSamplesIn);
typedef void     (*KeywordDestroy_t)          (void *pState);

typedef struct {
    KeywordInit_t   initFunc;
    KeywordDetect_t detectFunc;
    KeywordDestroy_t destroyFunc;
} KeywordCallbacks_t;

typedef struct {
    void *pImplementationState;
    KeywordCallbacks_t *pCallbacks;
} KeywordRecognizer_t;

void KeywordInit(KeywordConfig_t *pCfg, KeywordRecognizer_t *pState);
void KeywordDetect(KeywordRecognizer_t *pState, int16 *pSamplesIn);
void KeywordDestroy(KeywordRecognizer_t *pState);

void KeywordSetDefaults(KeywordConfig_t *pCfg, int blockSize);

//
// this function is implementation-specific as well
void KeywordSetProfile(KeywordConfig_t *pCfg, int profileIndex, int32 *pProfileData, int lenProfile_i32);

//
// keyword detection is not automatically performed in the MMFX layer.
//
// the MMIF layer should use the MMIF helper function MMIfDetectKeyword() to invoke the 
// keyword detector.
void KeywordTapPointInstall(TapPoint_t *pTapPoint, KeywordRecognizer_t *pState);

#endif

#ifdef __cplusplus
}
#endif
