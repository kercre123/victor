#ifdef __cplusplus
extern "C" {
#endif

#ifndef LRHPF_PUB_XXX_H
#define LRHPF_PUB_XXX_H 

#include "se_types.h"

typedef struct  {
    int16 betaMin;
    int16 betaMax;
    int32 sampleRate_kHz;
    int32 delayMs;
    int32 attackTimeConstantMs;
    int32 releaseTimeConstantMs;
    int32 blockSize;
    float32 coeffCutoffHz;
} LevelResponsiveFilterConfig_t;

#endif // LRHPF

#ifdef __cplusplus
}
#endif
