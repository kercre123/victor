
/* 
**************************************************************************
  (C) Copyright 2014 Signal Essence; All Rights Reserved
  
  History
  2014-08-15  robert yu
  
**************************************************************************
*/
#ifndef _OUTPUT_INJECTOR_PUB_H_
#define _OUTPUT_INJECTOR_PUB_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef enum 
{
    INJECT_MODE_FIRST_DONT_USE = 0,      // dont use
    INJECT_NONE = 0,
    INJECT_SINE,
    INJECT_WHITENOISE,
    INJECT_LAST                          // dont use
} InjectorSignal_t;

typedef struct
{
    int32 sampleRateHzRout;
    int32 sampleRateHzSout;
    InjectorSignal_t signalToInject;

    // Sinewave generator
    float32              sineFreqHz;
    int16               sineRxMagnitudeQ15;
    int16               sineTxMagnitudeQ15;

    // white noise gen
    int16              whiteNoiseTxMagnitudeQ15;
    int16              whiteNoiseRxMagnitudeQ15;

} InjectorConfig_t;

#ifdef __cplusplus
}
#endif
#endif


