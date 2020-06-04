#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   (C) Copyright 2012 SignalEssence; All Rights Reserved

    Module Name: meta_aec

    Author: Robert Yu

    Description:
    meta AEC:  a unified API for acoustic echo canceller implementations

    History:
    2012-06-28   ryu   first edit

    Machine/Compiler:
    (ANSI C)
**************************************************************************/
#ifndef META_AEC_PUB_H_
#define META_AEC_PUB_H_

#include "se_types.h"
#include "aec_td_pub.h"
#include "aec_pbfd_pub.h"
#include "aec_msu_pub.h"
#include "aec_stereo_pub.h"

typedef enum
{
    META_AEC_FIRST_DONT_USE,
    META_AEC_TD,    // time-domain LMS
    META_AEC_PBFD,  // partitioned-block freq domain (freq domain adaptation and cancellation)
    META_AEC_MSU, // partitioned-block freq domain + cycle-load smoothing
    META_AEC_STEREO,// stereo pbfd canceller
    META_AEC_LAST   // illegal value
} MetaAecImplementation_t;


typedef struct
{
    //
    // specify which AEC to use
    MetaAecImplementation_t aecType;

    void* pCallbacks;   // opaque pointer to implementation-specific callbacks

    union 
    {
        AecTdConfig_t td;   // time domain AEC config
        AecPbfdConfig_t pbfd; // partitioned block freq domain 
        AecMsuConfig_t msu; // partitioned block freq domain 
        AecStConfig_t stereo; // partitioned block freq domain 
    } aecConfigUnion;
} MetaAecConfig_t;


#endif // META_AEC_H_

#ifdef __cplusplus
}
#endif

