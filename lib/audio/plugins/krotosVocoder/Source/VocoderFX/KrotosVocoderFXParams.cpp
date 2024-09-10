#include <AK/Tools/Common/AkAssert.h>
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include <math.h>
#include "KrotosVocoderFXParams.h"
#include "Vocoder.h"


// Constructor.
CKrotosVocoderFXParams::CKrotosVocoderFXParams( )
{
}

// Destructor.
CKrotosVocoderFXParams::~CKrotosVocoderFXParams( )
{
}

// Copy constructor.
CKrotosVocoderFXParams::CKrotosVocoderFXParams( const CKrotosVocoderFXParams& in_rCopy )
{
    *this = in_rCopy;
}

// Create parameter node duplicate.
AK::IAkPluginParam* CKrotosVocoderFXParams::Clone( AK::IAkPluginMemAlloc* in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CKrotosVocoderFXParams( *this ) );
}

// Parameter node initialization.
AKRESULT CKrotosVocoderFXParams::Init(
                                AK::IAkPluginMemAlloc*  in_pAllocator,
                                const void*             in_pParamsBlock,
                                AkUInt32                in_ulBlockSize  )
{
    if ( in_ulBlockSize == 0)
    {
        Reset();
        return AK_Success;
    }
    return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

// Parameter interface termination.
AKRESULT CKrotosVocoderFXParams::Term( AK::IAkPluginMemAlloc* in_pAllocator )
{
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Parameter block set.
AKRESULT CKrotosVocoderFXParams::SetParamsBlock( 
    const void* in_pParamsBlock,
    AkUInt32 in_ulBlockSize )
{
    AKRESULT eResult = AK_Success;
    AkUInt8* pParamsBlock = (AkUInt8*)in_pParamsBlock;

    iCarrier               = READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );
    fCarrierFrequency      = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
    fCarrierFrequencyMax   = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
    fCarrierFrequencyMin   = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
    iCarrierWaveform       = READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );
    fEnvelopeAttackTime    = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
    fEnvelopeReleaseTime   = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
    iPitchTrackerAlgorithm = READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );
    fEQSlider[0]           = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
    fEQSlider[1]           = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
    fEQSlider[2]           = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
    fEQSlider[3]           = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
    fEQSlider[4]           = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
    fEQSlider[5]           = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
    fEQSlider[6]           = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
    fEQSlider[7]           = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	wetVol                 = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	dryVol                 = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
    return eResult;
}

// Update a single parameter.
AKRESULT CKrotosVocoderFXParams::SetParam(    
                                    AkPluginParamID in_ParamID,
                                    const void*     in_pValue, 
                                    AkUInt32        in_ulParamSize )
{
    AKRESULT eResult = AK_Success;

    switch ( in_ParamID )
    {
    // Note RTPC parameters are always of type float regardless of property type in XML plugin description
    case KROTOS_VOCODER_FXPARAM_CARRIER_ID:               iCarrier               = (AkUInt32)(*(AkReal32*)(in_pValue)); break;
    case KROTOS_VOCODER_FXPARAM_CARRIERFREQUENCY_ID:      fCarrierFrequency      =            *(AkReal32*)(in_pValue);  break;
    case KROTOS_VOCODER_FXPARAM_CARRIERFREQUENCYMAX_ID:   fCarrierFrequencyMax   =            *(AkReal32*)(in_pValue);  break;
    case KROTOS_VOCODER_FXPARAM_CARRIERFREQUENCYMIN_ID:   fCarrierFrequencyMin   =            *(AkReal32*)(in_pValue);  break;
    case KROTOS_VOCODER_FXPARAM_CARRIERWAVEFORM_ID:       iCarrierWaveform       = (AkUInt32)(*(AkReal32*)(in_pValue)); break;
    case KROTOS_VOCODER_FXPARAM_ENVELOPEATTACKTIME_ID:    fEnvelopeAttackTime    =            *(AkReal32*)(in_pValue);  break;
    case KROTOS_VOCODER_FXPARAM_ENVELOPERELEASETIME_ID:   fEnvelopeReleaseTime   =            *(AkReal32*)(in_pValue);  break;
    case KROTOS_VOCODER_FXPARAM_PITCHTRACKERALGORITHM_ID: iPitchTrackerAlgorithm = (AkUInt32)(*(AkReal32*)(in_pValue)); break;
    case KROTOS_VOCODER_FXPARAM_EQSLIDER1_ID:             fEQSlider[0]           =            *(AkReal32*)(in_pValue);  break;
    case KROTOS_VOCODER_FXPARAM_EQSLIDER2_ID:             fEQSlider[1]           =            *(AkReal32*)(in_pValue);  break;
    case KROTOS_VOCODER_FXPARAM_EQSLIDER3_ID:             fEQSlider[2]           =            *(AkReal32*)(in_pValue);  break;
    case KROTOS_VOCODER_FXPARAM_EQSLIDER4_ID:             fEQSlider[3]           =            *(AkReal32*)(in_pValue);  break;
    case KROTOS_VOCODER_FXPARAM_EQSLIDER5_ID:             fEQSlider[4]           =            *(AkReal32*)(in_pValue);  break;
    case KROTOS_VOCODER_FXPARAM_EQSLIDER6_ID:             fEQSlider[5]           =            *(AkReal32*)(in_pValue);  break;
    case KROTOS_VOCODER_FXPARAM_EQSLIDER7_ID:             fEQSlider[6]           =            *(AkReal32*)(in_pValue);  break;
    case KROTOS_VOCODER_FXPARAM_EQSLIDER8_ID:             fEQSlider[7]           =            *(AkReal32*)(in_pValue);  break;
	case KROTOS_VOCODER_FXPARAM_WETVOL_ID:				  wetVol                 =            *(AkReal32*)(in_pValue);  break;
	case KROTOS_VOCODER_FXPARAM_DRYVOL_ID:                dryVol                 =            *(AkReal32*)(in_pValue);  break;

    default:
        AKASSERT(!"Invalid parameter.");
        eResult = AK_InvalidParameter;
    }

    return eResult;
}
