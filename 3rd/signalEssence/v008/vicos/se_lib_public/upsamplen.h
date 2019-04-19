/***************************************************************************
   (C) Copyright 2016 SignalEssence; All Rights Reserved

    Module Name: upsamplen

    Author: Robert Yu

    Description:
    upsampler
**************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#ifndef UPSAMPN_H
#define UPSAMPN_H

#include "se_types.h"
#include "rfir_f.h"

typedef struct 
{
    int32 numChans;
    struct RFirF_t **ppFir;
    int32 blocksizeOut;
    int32 blocksizeIn;
    int32 ratio;
    int32 bypass; // true if the ratio is 1.
} UpsampleN_t;

void UpsampleNInit(UpsampleN_t *p, int32 numChans, int32 blocksizeIn, int32 ratio);
void UpsampleNDestroy(UpsampleN_t *p);
void UpsampleN_f32(UpsampleN_t *p, const float32 *pSrc, float32 *pDest);
void UpsampleN_i16_i16(UpsampleN_t *p, const int16 *pSrc, int16 *pDest);

#endif

#ifdef __cplusplus
}
#endif
