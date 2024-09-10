//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// Multichannel ring buffer (assumes single producer and consumer). 
// Internal representation is channel interleaved but can be pulled or pushed in either interleaved or deinterleaved formats.

#pragma once


#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkAutoLock.h>

// #define ENABLE_AUDIO_BUFFER_CHECKS 
// #define ENABLE_LOG_RB_STATE

namespace AK
{
	namespace CustomHardware
	{
		class AkAudioRingBuffer
		{
		public:
			AkAudioRingBuffer();
			AKRESULT Init(AK::IAkPluginMemAlloc * in_pAllocator, unsigned int in_uNumChannels, unsigned int in_uFrameCapacity); // Allocate buffers for desired number of channels and capacity
			void Term(); // Free storage
			void Reset( bool in_bFull ); // Reset internal storage status

			unsigned int FramesAvailableForConsumer(); // Determine frames that can be consumed from the ring buffer
			unsigned int FramesRemainingForProducer(); // Determine frames that can be produced to the ring buffer
			unsigned int Capacity(); // Total frame capacity of the ring buffer
			unsigned int NumChannels(); // Number of channels stored

			// Get interleaved data (data is copied to external location). Does not advance read location.
			AKRESULT GetChannels(unsigned int in_uFramesToGet, float * io_pfBuffer, unsigned int in_StartChannel, unsigned int in_uNumChannels);
			// Get deinterleaved data (data is copied to external location). Does not advance read location. Channel buffers are assumed to be contiguous (single pointer for start)
			AKRESULT GetChannelsDeinterleaved(unsigned int in_uFramesToGet, float * io_pfBuffer, unsigned int in_uMaxFrames, unsigned int in_StartChannel, unsigned int in_uNumChannels);
			// Push interleaved data (data is copied to ring buffer). Does not advance write location.
			AKRESULT PushChannels(unsigned int in_uFramesToPush, float * io_pfBuffer, unsigned int in_StartChannel, unsigned int in_uNumChannels);
			AKRESULT PushFrames(unsigned int in_uFramesToPush, float * in_pfBuffer);			// Push interleaved data (data is cached to RB location) and advance write location. 
			void AdvanceReadFrames(unsigned int in_uFramesToAdvance);							// Advance read pointer. Internal data available for writing again after call.
			void AdvanceWriteFrames(unsigned int in_uFramesToAdvance);							// Advance write pointer. Internal data available for reading again after call.
#ifdef ENABLE_LOG_RB_STATE
			void LogStatus(const char * pIDStr);
#endif

		protected:

			unsigned int			m_uWriteIndex;
			unsigned int			m_uReadIndex;
			unsigned int			m_uCapacity;
			unsigned int			m_uFramesInput;
			unsigned int			m_uFramesOutput;
			float *					m_pfRingBuffer;
			unsigned int			m_uNumChannels;
			AK::IAkPluginMemAlloc * m_pAllocator;
			CAkLock					m_Mutex;
#ifdef ENABLE_AUDIO_BUFFER_CHECKS			
			float					m_fLastSampleInput;	
			float					m_fLastSampleOut;
#endif

		};

	} // namespace CustomHardware
} // namespace AK
