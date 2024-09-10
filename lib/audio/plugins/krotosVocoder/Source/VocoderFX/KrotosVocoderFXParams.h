#ifndef _KROTOS_VOCODERFXPARAMS_H_
#define _KROTOS_VOCODERFXPARAMS_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>

// Parameters IDs for the Wwise or RTPC.
// Those IDs map to the AudioEnginePropertyID attributes in the XML property definition
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_CARRIER_ID               = 0;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_CARRIERFREQUENCY_ID      = 1;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_CARRIERFREQUENCYMAX_ID   = 2;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_CARRIERFREQUENCYMIN_ID   = 3;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_CARRIERWAVEFORM_ID       = 4;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_ENVELOPEATTACKTIME_ID    = 5;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_ENVELOPERELEASETIME_ID   = 6;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_PITCHTRACKERALGORITHM_ID = 7;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_EQSLIDER1_ID             = 8;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_EQSLIDER2_ID             = 9;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_EQSLIDER3_ID             = 10;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_EQSLIDER4_ID             = 11;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_EQSLIDER5_ID             = 12;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_EQSLIDER6_ID             = 13;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_EQSLIDER7_ID             = 14;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_EQSLIDER8_ID             = 15;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_WETVOL_ID				 = 16;
static const AkPluginParamID KROTOS_VOCODER_FXPARAM_DRYVOL_ID				 = 17;

static const AkPluginParamID KROTOS_VOCODERFXPARAM_NUM                       = 18;

struct KrotosVocoderFXParams
{
    AkUInt32    iCarrier;
    AkReal32    fCarrierFrequency;
    AkReal32    fCarrierFrequencyMax;
    AkReal32    fCarrierFrequencyMin;
    AkUInt32    iCarrierWaveform;
    AkReal32    fEnvelopeAttackTime;
    AkReal32    fEnvelopeReleaseTime;
    AkUInt32    iPitchTrackerAlgorithm;
    AkReal32    fEQSlider[8 /*numberOfFftGains*/];
	AkReal32	wetVol;
	AkReal32	dryVol;

    void Reset()
    {
        // Init default parameters.
        iCarrier               = 1;         // Literal in Vocoder\PluginEditor.cpp => Vocoder::Carrier::OscillatorPitchTracking
        fCarrierFrequency      = 100.f;     //               "                     => 100
        fCarrierFrequencyMax   = 1000.f;    // vocoder::m_oscillatorFrequencyMaxInit
        fCarrierFrequencyMin   = 10.f;      // vocoder::m_oscillatorFrequencyMinInit
        iCarrierWaveform       = 2;         // vocoder::m_oscillatorShapeInit
        fEnvelopeAttackTime    = 1.0f;      // vocoder::m_envelopeDetectorAttackTimeInSecInit  * 1000.0f
        fEnvelopeReleaseTime   = 1.0f;      // vocoder::m_envelopeDetectorReleaseTimeInSecInit * 1000.0f
        iPitchTrackerAlgorithm = 3;         // vocoder::m_pitchTrackerAlgorithmInit
        for (int i = 0; i < 8 /*numberOfFftGains*/; ++i)
        {
            fEQSlider[i] = 1.0f;            // vocoder::m_eqGainInLinearInit
        }
		wetVol = 0;
		dryVol = -96.3f;
    }
} AK_ALIGN_DMA;

class CKrotosVocoderFXParams 
    : public AK::IAkPluginParam
    , public KrotosVocoderFXParams
{
public:

    /// Constructor.
    CKrotosVocoderFXParams( );

    /// Copy constructor.
    CKrotosVocoderFXParams( const CKrotosVocoderFXParams& in_rCopy );

    /// Destructor.
    ~CKrotosVocoderFXParams();

    /// Create parameter node duplicate.
    IAkPluginParam* Clone( AK::IAkPluginMemAlloc* in_pAllocator );

    /// Parameter node initialization.
    AKRESULT Init(    
        AK::IAkPluginMemAlloc*  in_pAllocator,                            
        const void*             in_pParamsBlock, 
        AkUInt32                in_ulBlockSize 
        );

    /// Parameter node termination.
    AKRESULT Term( AK::IAkPluginMemAlloc* in_pAllocator );

    /// Parameter block set.
    AKRESULT SetParamsBlock(
        const void* in_pParamsBlock, 
        AkUInt32    in_ulBlockSize
        );

    /// Update a single parameter.
    AKRESULT SetParam(
        AkPluginParamID in_ParamID,
        const void*     in_pValue,
        AkUInt32        in_ulParamSize
        );
};

#endif // _KROTOS_VOCODERFXPARAMS_H_
