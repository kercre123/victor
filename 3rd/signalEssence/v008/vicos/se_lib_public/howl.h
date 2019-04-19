#ifdef __cplusplus
extern "C" {
#endif

#ifndef HOWL_H
#define HOWL_H

#include "se_types.h"
#include "vgentone.h"
#include "rfir_f.h"

typedef struct 
{
    int blockSize;
    float32 sampleRateHz;
    float32 deltaShiftHz;
    uint16 enableBypass;
} HowlSuppressorConfig_t;

typedef struct
{
    float32 currDeltaShiftHz;
    HowlSuppressorConfig_t config;
    VGenTone_t modulator;
    VGenTone_t demodulator;
    struct RFirF_t lpf;

} HowlSuppressor_t;


void HowlSetDefaultConfig(HowlSuppressorConfig_t *pConfig,
                          int blockSize,
                          int sampleRateHz);
void HowlInit(HowlSuppressor_t *pState, const HowlSuppressorConfig_t *pConfig);
void HowlDestroy(HowlSuppressor_t *pState);
void HowlSuppress_f32(HowlSuppressor_t *pState,
                      const float32 *pIn,
                      float32 *pOut);
void HowlSuppress_q15(HowlSuppressor_t *pState,
                      const int16 *pIn,
                      int16 *pOut);

#endif

#ifdef __cplusplus
}
#endif
