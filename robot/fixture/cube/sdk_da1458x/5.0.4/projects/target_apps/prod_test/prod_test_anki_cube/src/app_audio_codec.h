/**
 ****************************************************************************************
 *                 @@@           @@                  @@
 *                @@@@@          @@   @@             @@
 *                @@@@@          @@                  @@
 *       .@@@@@.  @@@@@      @@@ @@   @@     @@@@    @@     @@@       @@@
 *     @@@@@@   @@@@@@@    @@   @@@   @@        @@   @@   @@   @@   @@   @@
 *    @@@@@    @@@@@@@@    @@    @@   @@    @@@@@@   @@   @@   @@   @@   @@
 *   @@@@@@     @@@@@@@    @@    @@   @@   @@   @@   @@   @@   @@   @@   @@
 *   @@@@@@@@     @@@@@    @@   @@@   @@   @@   @@   @@   @@   @@   @@   @@
 *   @@@@@@@@@@@    @@@     @@@@ @@   @@    @@@@@    @@     @@@       @@@@@
 *    @@@@@@@@@@@  @@@@                                                  @@
 *     @@@@@@@@@@@@@@@@                                                  @@
 *       "@@@@@"  @@@@@    S  E  M  I  C  O  N  D  U  C  T  O  R     @@@@@
 *
 *
 * @file app_audio_codec.h
 *
 * @brief Functions for compressing Audio samples from 439
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 *
 *  IMA ADPCM
 *  This IMA ADPCM has been implemented based on the paper spec of 
 *  IMA Digital Audio Focus and Technical Working Groups,
 *  Recommended Practices for Enchancing Digital Audio Compatibility 
 *  Revision 3.00, 21 October 1992. 
 *  http://www.cs.columbia.edu/~hgs/audio/dvi/IMA_ADPCM.pdf
 * 
 *  There are some enhancement/modifications.
 *  1) Encoder: The divisor is calculated with full precision by upshifting
 *     both the stepSize and Difference by 3.
 *  2) The new predicted difference is calculated with full precision by
 *     doing the mult/shift-add 3 bits more. 
 *  3) The exact same prediction is used in Encoder and Decoder. Note that
 *     some implementations of IMA-ADPCM do not do this.
 *  4) There is an alternative (but bit-true) calculation of the prediction
 *     difference using a multiplier instead of shift add. May be faster.
 *
 ****************************************************************************************
 */


#ifndef __APP_CODEC_H__
#define __APP_CODEC_H__

#include <stdint.h>

#define FILTER_LENGTH 38

typedef struct s_IMAData {
    int16_t  *inp;
    uint8_t  *out;
    int      len;
    int16_t  index;
    int16_t  predictedSample;
    int      imaSize;
    int      imaAnd;
    int      imaOr;
} t_IMAData;

#define INIT_IMA_DATA {0,0,0,0,0}

/**
 ****************************************************************************************
 * @brief IMA Adpcm Encoding, block based
 *
 * @param[inout] state: encoding state with input/output pointers and states
 *
 * @return void
 ****************************************************************************************
 */
extern void app_ima_enc(t_IMAData *state);

/**
 ****************************************************************************************
 * @brief ALaw Encoding, sample based
 *
 * @param[in] pcm_sample: input sample
 *
 * @return Alaw sample
 ****************************************************************************************
 */
extern uint8_t audio439_aLaw_encode(int16_t pcm_sample);
#define DC_BLOCK
#ifdef DC_BLOCK
/**
 ****************************************************************************************
 * @brief DC Blocking Filter
 *
 * Remove DC with differentiator and leaky integrator 
 * See http://www.dspguru.com/dsp/tricks/fixed-point-dc-blocking-filter-with-noise-shaping
 *
 * The DC high pass filter should have cutt of freq of 200Hz. 
 * With Fc=1/2*PI*RC, and alpha = RC/(RC+dt), an dt=1/Fs, (and PI=22/7)
 * then alpha = calc "1-(1/(200*2*22/7))/((1/(200*2*22/7))+(1/16000))"
 * = 0.0728
 ****************************************************************************************
 */
typedef struct s_DcBlockData {
    int16_t *inp;
    int16_t *out;
    int32_t  len;
    int32_t yyn1;
    int16_t beta;
    /* States */
    int16_t xn1;
    int16_t fcnt;
    int16_t fade;
    int16_t fade_step;
} t_DCBLOCKData;

#define TOINT(a) (a*32768)
#define APP_AUDIO_DCB_BETA (32768*0.0728)
#define INIT_DCBLOCK_DATA {0,0,40,APP_AUDIO_DCB_BETA,0,0,-20,0,0}


/**
 ****************************************************************************************
 * @brief DC Blocking Filter function
 *
 * This function will do DC Blocking on a block of len samples.
 * Note that input and output pointer may point to the same array (inplace).
 *
 * @param[in] inp: PCM input sample
 *
 * @return Alaw sample
 ****************************************************************************************
 */
extern void app_audio_dcblock(t_DCBLOCKData *pDcBlockData);
#endif


/**
 ****************************************************************************************
 * @brief downSample
 *
 * @param[in] len:     number of input samples
 * @param[in] samples: Inplace buffer for input samples and output samples
 *
 * @return
 ****************************************************************************************
 */

void audio439_downSample(int len, int16_t *inpSamples, int16_t *outSamples, int16_t *taps);

#endif

