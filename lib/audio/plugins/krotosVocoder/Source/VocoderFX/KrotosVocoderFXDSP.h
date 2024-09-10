#ifndef _KROTOS_VOCODERFXDSP_H_
#define _KROTOS_VOCODERFXDSP_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Plugin/PluginServices/AkFXTailHandler.h>
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "KrotosVocoderFXParams.h"
#include <AK/DSP/AkDelayLineMemoryStream.h>

#include "Vocoder.h"


/// Delay FX DSP Processing class
class CKrotosVocoderFXDSP
{
public:
    CKrotosVocoderFXDSP();
	~CKrotosVocoderFXDSP();

    void Setup( KrotosVocoderFXParams* pInitialParams, bool in_bIsSendMode, AkUInt32 in_uSampleRate, AK::IAkPluginMemAlloc* in_pAllocator);
    void Process( AkAudioBuffer* io_pBuffer, KrotosVocoderFXParams* pCurrentParams );

private:
    void UpdateVocoderParameters(KrotosVocoderFXParams& current, KrotosVocoderFXParams& prev);

	void applyGain(vector<float> &buffer, float gain);

	Vocoder*                vocoder{NULL};
    KrotosVocoderFXParams   m_PreviousParams;   /// Cache previous parameters for comparison
    AkUInt32                m_uSampleRate;      /// Sample rate
    bool                    m_bSendMode;        /// Effect used in aux send configuration (e.g. environmental effect)
	AK::IAkPluginMemAlloc*  m_pAllocator;

	float					m_outRMS;			/// Keep a record of the output RMS from last processed buffer
	AkInt32					m_MaxExtraSamples;	/// For safety we count down a max no of extra buffers when dealing with FX tail
} AK_ALIGN_DMA;

#endif // _KROTOS_VOCODERFXDSP_H_
