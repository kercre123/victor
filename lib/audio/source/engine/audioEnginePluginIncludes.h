/*
 * File: audioEnginePluginIncludes.h
 *
 * Author: Jordan Rivas
 * Created: 11/28/2016
 *
 * Description: Link audio engine (Wwise) plugins
 *
 * Copyright: Anki, Inc. 2016
 *
 */

#ifndef __AnkiAudio_AudioEnginePluginIncludes_H__
#define __AnkiAudio_AudioEnginePluginIncludes_H__

// DSP plugins

// Don't want to load all plugins...
// #include <AK/Plugin/AllPluginsFactories.h>

// ...Instead, Load specific plugins by adding the appropriate defines at the project level

// Effect plugins

#ifdef AE_AK_COMPRESSOR_FX
#include <AK/Plugin/AkCompressorFXFactory.h>                  // Compressor
#endif

#ifdef AE_AK_DELAY_FX
#include <AK/Plugin/AkDelayFXFactory.h>                       // Delay
#endif

#ifdef AE_AK_MATRIX_REVERB_FX
#include <AK/Plugin/AkMatrixReverbFXFactory.h>                // Matrix reverb
#endif

#ifdef AE_AK_METER_FX
#include <AK/Plugin/AkMeterFXFactory.h>                       // Meter
#endif

#ifdef AE_AK_EXPANDER_FX
#include <AK/Plugin/AkExpanderFXFactory.h>                    // Expander
#endif

#ifdef AE_AK_PARAMETRIC_EQ_FX
#include <AK/Plugin/AkParametricEQFXFactory.h>                // Parametric equalizer
#endif

#ifdef AE_AK_GAIN_FX
#include <AK/Plugin/AkGainFXFactory.h>                        // Gain
#endif

#ifdef AE_AK_PEAK_LIMITER_FX
#include <AK/Plugin/AkPeakLimiterFXFactory.h>                 // Peak limiter
#endif

#ifdef AE_AK_SOUND_SEED_IMPACT_FX
#include <AK/Plugin/AkSoundSeedImpactFXFactory.h>             // SoundSeed Impact
#endif

#ifdef AE_AK_ROOM_VERB_FX
#include <AK/Plugin/AkRoomVerbFXFactory.h>                    // RoomVerb
#endif

#ifdef AE_AK_GUITAR_DISTORTION_FX
#include <AK/Plugin/AkGuitarDistortionFXFactory.h>            // Guitar distortion
#endif

#ifdef AE_AK_STEREO_DELAY_FX
#include <AK/Plugin/AkStereoDelayFXFactory.h>                 // Stereo delay
#endif

#ifdef AE_AK_PITCH_SHIFTER_FX
#include <AK/Plugin/AkPitchShifterFXFactory.h>                // Pitch shifter
#endif

#ifdef AE_AK_TIME_STRETCH_FX
#include <AK/Plugin/AkTimeStretchFXFactory.h>                 // Time stretch
#endif

#ifdef AE_AK_FLANGER_FX
#include <AK/Plugin/AkFlangerFXFactory.h>                     // Flanger
#endif

#ifdef AE_AK_CONVOLUTION_REVERB_FX
#include <AK/Plugin/AkConvolutionReverbFXFactory.h>           // Convolution reverb
#endif

#ifdef AE_AK_TREMOLO_FX
#include <AK/Plugin/AkTremoloFXFactory.h>                     // Tremolo
#endif

#ifdef AE_AK_HARMONIZER_FX
#include <AK/Plugin/AkHarmonizerFXFactory.h>                  // Harmonizer
#endif

#ifdef AE_AK_RECORDER_FX
#include <AK/Plugin/AkRecorderFXFactory.h>                    // Recorder
#endif

// McDSP plugins
#ifdef AE_MCDSP_FUTZ_BOX_FX
#include <AK/Plugin/McDSPFutzBoxFXFactory.h>                  // FutzBox
#endif

#ifdef AE_MCDSP_LIMITER_FX
#include <AK/Plugin/McDSPLimiterFXFactory.h>                  // ML1 Limiter
#endif

// Crankcase plugins
#ifdef AE_CRANKCASE_REV_FX
#include <AK/Plugin/CrankcaseAudioREVModelPlayerFXFactory.h>  // REV
#endif

// iZotope plugins
#ifdef AE_iZotope_HybridReverb_FX
#include <AK/Plugin/iZHybridReverbFXFactory.h>                // iZotope Hybrid Reverb
#endif

#ifdef AE_iZotope_TrashBoxModeler_FX
#include <AK/Plugin/iZTrashBoxModelerFXFactory.h>             // iZotope Trash Box Modeler
#endif

#ifdef AE_iZotope_TrashDelay_FX
#include <AK/Plugin/iZTrashDelayFXFactory.h>                  // iZotope Trash Delay
#endif

#ifdef AE_iZotope_TrashDistortion_FX
#include <AK/Plugin/iZTrashDistortionFXFactory.h>             // iZotope Trash Distortion
#endif

#ifdef AE_iZotope_TrashDynamics_FX
#include <AK/Plugin/iZTrashDynamicsFXFactory.h>               // iZotope Trash Dynamics
#endif

#ifdef AE_iZotope_TrashFilters_FX
#include <AK/Plugin/iZTrashFiltersFXFactory.h>                // iZotope Trash Filters
#endif

#ifdef AE_iZotope_TrashMultibandDistortion_FX
#include <AK/Plugin/iZTrashMultibandDistortionFXFactory.h>    // iZotope Trash Multiband Distortion
#endif

// Custom Plugin built for Victor
#ifdef AE_Krotos_Vocoder_FX
#include "KrotosVocoderFXFactory.h"                           // Krotos Vocoder
#endif


// Sources plugins
#ifdef AE_AK_SILENCE_SOURCE
#include <AK/Plugin/AkSilenceSourceFactory.h>                 // Silence generator
#endif

#ifdef AE_AK_SINE_GENERATOR_SOURCE
#include <AK/Plugin/AkSineSourceFactory.h>                    // Sine wave generator
#endif

#ifdef AE_AK_TONE_GENERATOR_SOURCE
#include <AK/Plugin/AkToneSourceFactory.h>                    // Tone generator
#endif

#ifdef AE_AK_AUDIO_INPUT_SOURCE
#include <AK/Plugin/AkAudioInputSourceFactory.h>              // Audio input
#endif

#ifdef AE_AK_SOUND_SEED_WOOSH_SOURCE
#include <AK/Plugin/AkSoundSeedWooshSourceFactory.h>          // SoundSeed Woosh
#endif

#ifdef AE_AK_SOUND_SEED_WIND_SOURCE
#include <AK/Plugin/AkSoundSeedWindSourceFactory.h>           // SoundSeed Wind
#endif

#ifdef AE_AK_SYNTH_ONE_SOURCE
#include <AK/Plugin/AkSynthOneSourceFactory.h>                // SynthOne
#endif


// Sink plugins
#ifdef AE_AK_ALSA_SINK
#include "akAlsaSinkFactory.h"                                // AkAlsaSink
#endif


// Required by codecs plugins
#include <AK/Plugin/AkVorbisDecoderFactory.h>
#ifdef AK_APPLE
#include <AK/Plugin/AkAACFactory.h>      // Note: Useable only on Apple devices.
#endif                                   //       Ok to include it on other platforms as long as it is not referenced.


#endif /* __AnkiAudio_AudioEnginePluginIncludes_H__ */
