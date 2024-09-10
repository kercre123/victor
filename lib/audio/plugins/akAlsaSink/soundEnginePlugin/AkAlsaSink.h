/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: <VERSION>  Build: <BUILDNUMBER>
  Copyright (c) <COPYRIGHTYEAR> Audiokinetic Inc.
 ***********************************************************************/

#pragma once

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <alsa/asoundlib.h>
#include "audioEngine/plugins/akAlsaSinkPluginTypes.h"
#include "AkAlsaSinkParams.h"
#include "AkPluginsSync.h"
#include "AkPluginsExtApi.h"
#include "util/container/fixedCircularBuffer.h"

#include <functional>
#include <mutex>

/////////////////////////////////////////////////////
// Static buffer size
// Override Wwise Plugin UI value so designers can't break config, Safety first!
// See akAlsaSinkPluginTypes.h
/////////////////////////////////////////////////////

#define AKALSASINK_MAX_LATENCY_ERRORS 128
#define AKALSASINK_DEFAULT_LATENCY_THRESHOLD 5 //5 millisecond

class AkAlsaSink : public AK::IAkSinkPlugin
{
public:
    // Function will be called after allocating plugin instance in CreateAkAlsaSink()
    static std::function<void(AkAlsaSink*)> PostCreateAlsaFunc;

	AkAlsaSink();
	~AkAlsaSink();

	virtual AKRESULT Init( 
		AK::IAkPluginMemAlloc 	 * in_pAllocator,			// Interface to memory allocator to be used by the effect.
		AK::IAkSinkPluginContext * in_pSinkPluginContext,	// Interface to sink plug-in's context.
		AK::IAkPluginParam 		 * in_pParams,				// Interface to parameter node
		AkAudioFormat 			 & in_rFormat				// Audio data format of the input signal.
		);
	virtual AKRESULT Term( 
		AK::IAkPluginMemAlloc 	 * in_pAllocator 			// interface to memory allocator to be used by the plug-in
		);
	virtual AKRESULT Reset();
	virtual AKRESULT GetPluginInfo( 
		AkPluginInfo			 & out_rPluginInfo			// Reference to the plug-in information structure to be retrieved
		);
	virtual void Consume(
		AkAudioBuffer 			 * in_pInputBuffer,			// Input audio buffer data structure. Plugins should avoid processing data in-place.
		AkRamp					 in_gain					// Volume gain to apply to this input (prev corresponds to the beginning, next corresponds to the end of the buffer).
		);
	virtual void OnFrameEnd();
	virtual bool IsStarved();
	virtual void ResetStarved();
	virtual AKRESULT IsDataNeeded( AkUInt32 & out_uBuffersNeeded );

// TODO: Avoid public data (currently necessary to expose to static callback functions)

	AK::IAkSinkPluginContext *						m_pSinkPluginContext;
	unsigned int									m_uOutNumChannels;
	unsigned int *									m_pChannelMap;
	AkUInt32										m_uNumBuffers;
    char *				    						m_pDeviceName; /* Keep the device name */
    bool											m_bDeviceFound;
	AkChannelMask									m_uChannelMask;
	AkUInt16										m_uFrameSize;
	AkUInt32 										m_uUnderflowCount;
	AkUInt32 										m_uOverflowCount;
    AkUInt64	 									m_uLatencyMax;
    AkUInt32	 									m_uLatencyErrorCnt;
    AkUInt32										m_uSampleRate;
    bool											m_bDataReady;
    bool											m_bIsPrimary;
	AkAlsaSinkPluginParams *						m_pSharedParams;
	snd_pcm_t *										m_pPcmHandle;	//Alsa pcm handle
	snd_pcm_hw_params_t *							m_phwparams; // Alsa Hardware configuration parameters
	snd_pcm_sw_params_t *							m_pswparams; // Alsa Hardware configuration parameters
	bool											m_bMasterMode;
	AkUInt32										m_uOutputID;
	AkUInt64										m_LatencyReport;
	AkThread 										m_Alsathread;
	bool											m_bThreadRun;
	bool											m_bCustomThreadEnable;
	snd_async_handler_t *                           m_pPcm_callback_handle;
	AkUInt32										m_uCpuMask;

	using SinkPluginTypes = Anki::AudioEngine::PlugIns::AkAlsaSinkPluginTypes;
	
	// Custom local buffer to give us some slack between wwise audio production and alsa audio consumption
	Anki::Util::FixedCircularBuffer<SinkPluginTypes::AudioChunk, SinkPluginTypes::kAkAlsaSinkBufferCount> _sinkPluginBuffer;
	std::mutex _sinkPluginBufferMutex;

	// Callback is called at the end of every frame
	SinkPluginTypes::WriteBufferToAlsaCallbackFunc m_writeBufferToAlsaCallback = nullptr;
	

    // API external to the plug-in, to be used by the game.
    static void SetAlsaSinkCallbacks (AkOutputGetLatencyCallbackFunc in_pfnGetLatencyCallback=NULL, AkUInt32 in_uLatencyReportThresholdMsec=5);
  
    bool IsUsingSpeaker() const { return _usingSpeaker; }
  
    // Get the maximum speaker 'latency', which is the max delay between when we
    // command audio to be played and it actually gets played on the speaker
    uint32_t GetSpeakerLatency_ms() const { return _speakerLatency_ms; }
  
private:
    static AkOutputGetLatencyCallbackFunc m_pfnGetLatencyCallback;
    static AkUInt32						  m_uLatencyReportThresholdMsec; //Latency threshold value in ms, default 5 ms.

	std::atomic<bool> _usingSpeaker{false};
    std::atomic<uint32_t> _speakerLatency_ms{0};

};
