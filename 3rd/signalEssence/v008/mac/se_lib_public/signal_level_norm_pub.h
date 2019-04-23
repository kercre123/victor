/***************************************************************************
 (C) Copyright 2016 Signal Essence LLC; All Rights Reserved

 Module Name  - signal_level_norm
                Signal level normalizer
     
 History:    ryu - Robert Yu
             2016-02-05   ryu  first implementation

**************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#ifndef SIGNAL_LEVEL_NORM_PUB_H
#define SIGNAL_LEVEL_NORM_PUB_H

#include "se_types.h"

typedef struct
{
    float32 gainSinDb;
} SlnConfig_t;

#endif //SIGNAL_LEVEL_NORM_PUB_H

#ifdef __cplusplus
}
#endif
