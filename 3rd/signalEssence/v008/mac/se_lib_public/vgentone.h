#ifdef __cplusplus
extern "C" {
#endif


#ifndef _VGENTONE_H
#define _VGENTONE_H
#include "se_types.h"

typedef struct {
    float32 frequencyHz;
    float32 sampleFreqHz;
    float32 phase_r;
    float32 phase_i;
    float32 delta_r;
    float32 delta_i;
    float32 magnitude;
} VGenTone_t;

void VGenToneInit       (VGenTone_t *state, float frequency, float fs, float magnitude);

void VGenToneSetPhase(VGenTone_t *state, float angleRad);

void VGenToneRun        (VGenTone_t *state, int16 *dest, int32 blocksize);
void VGenToneRun_flt32  (VGenTone_t *state, float32 *dest, int32 blocksize);

void VGenToneRunComplex (VGenTone_t *state, int16 *dest, int32 blocksize);
void VGenToneRunComplex_f32 (VGenTone_t *state, float32 *dest, int32 blocksize);

float VGenToneGetFreq(VGenTone_t *state);

float VGenToneGetMagnitude(VGenTone_t *state);

#endif
#ifdef __cplusplus
}
#endif

