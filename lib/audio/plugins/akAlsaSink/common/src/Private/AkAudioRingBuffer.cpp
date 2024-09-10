//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkAudioRingBuffer.h"
#include <string.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "Logging.h"
#include <math.h>

#ifdef ENABLE_LOG_RB_STATE
#define LOG_RB_STATE(__ID__) LogStatus(__ID__);
#else
#define LOG_RB_STATE(__ID__)
#endif

namespace AK
{
	namespace CustomHardware
	{
#ifdef ENABLE_AUDIO_BUFFER_CHECKS
		static unsigned int CheckOutOfRangeSamples(float * in_pfBuffer, unsigned int in_uFramesInBuffer, unsigned int in_uNumChannels)
		{
			const unsigned int uNumSamples = in_uFramesInBuffer * in_uNumChannels;
			unsigned int uCount = 0;
			for (unsigned int i = 0; i < uNumSamples; i++)
			{
				float fSample = in_pfBuffer[i];
				if (abs(fSample) > 1.f)
					uCount++;
				if (fpclassify(fSample) == FP_SUBNORMAL)
					uCount++;
			}
			return uCount;
		}
#endif // ENABLE_AUDIO_BUFFER_CHECKS

		AkAudioRingBuffer::AkAudioRingBuffer() 
			: m_pfRingBuffer(NULL)
			, m_uCapacity(0)
			, m_uReadIndex(0)
			, m_uWriteIndex(0)
			, m_uFramesInput(0)
			, m_uFramesOutput(0)
			, m_uNumChannels(0)
			, m_pAllocator(NULL)
		{
		}

#ifdef ENABLE_LOG_RB_STATE
		void AkAudioRingBuffer::LogStatus(const char * pIDStr)
		{
			//AkAutoLock<CAkLock> AccessLock(m_Mutex);

			unsigned int uFillCount = FramesAvailableForConsumer();
			LOG_DEBUG("%s -> m_uCapacity: %d m_uNumChannels: %d m_pfRingBuffer: 0x%p m_uReadIndex: %d m_uWriteIndex: %d m_uFramesInput: %d m_uFramesOutput: %d uFillCount %d"
				, pIDStr, m_uCapacity, m_uNumChannels, m_pfRingBuffer, m_uReadIndex, m_uWriteIndex, m_uFramesInput, m_uFramesOutput, uFillCount);
		}
#endif

		// Allocate buffers for desired number of channels and capacity
		AKRESULT AkAudioRingBuffer::Init(AK::IAkPluginMemAlloc * in_pAllocator, unsigned int in_uNumChannels, unsigned int in_uFrameCapacity)
		{
			m_pAllocator = in_pAllocator;

			// Keep one storage slot unallocated to distinguish between empty/full conditions
			m_pfRingBuffer = (float*) AK_PLUGIN_ALLOC(m_pAllocator, in_uNumChannels*in_uFrameCapacity*sizeof(float));
			if (m_pfRingBuffer == NULL)
			{
				LOG_ERROR("Could not allocate audio ring buffer");
				return AK_InsufficientMemory;
			}
			m_uCapacity = in_uFrameCapacity;
			m_uNumChannels = in_uNumChannels;

			Reset(true); // Start full by default

			return AK_Success;
		}

		// Free storage
		void AkAudioRingBuffer::Term()
		{
			if (m_pfRingBuffer != NULL)
			{
				AK_PLUGIN_FREE(m_pAllocator, m_pfRingBuffer);
				m_pfRingBuffer = NULL;
			}
		}

		// Reset internal storage status
		void AkAudioRingBuffer::Reset(bool in_bFull)
		{
			if (m_pfRingBuffer != NULL)
			{
				memset(m_pfRingBuffer, 0, m_uCapacity*m_uNumChannels*sizeof(float));
			}
			if (in_bFull)
			{
				m_uFramesInput = m_uCapacity;
			}
			else
			{
				m_uFramesInput = 0;
			}	

			m_uWriteIndex = 0;
			m_uReadIndex = 0;
			m_uFramesOutput = 0;
#ifdef ENABLE_AUDIO_BUFFER_CHECKS
			m_fLastSampleInput = 0.f;
			m_fLastSampleOut = 0.f;
#endif
		}

		// Determine frames that can be consumed from the ring buffer
		unsigned int AkAudioRingBuffer::FramesAvailableForConsumer()
		{
			AkAutoLock<CAkLock> AccessLock(m_Mutex);

			unsigned int uFramesAvailable;
			if (m_uFramesInput >= m_uFramesOutput)
			{
				uFramesAvailable = m_uFramesInput - m_uFramesOutput;
			}
			else
			{
				// counter wrap around situation
				uFramesAvailable = (UINT_MAX - m_uFramesOutput + 1) + m_uFramesInput;
			}
			return uFramesAvailable;
		}

		// Determine frames that can be produced to the ring buffer
		unsigned int AkAudioRingBuffer::FramesRemainingForProducer()
		{
			AkAutoLock<CAkLock> AccessLock(m_Mutex);

			unsigned int uFramesAvailableForProducer;
			if (m_uFramesInput >= m_uFramesOutput)
			{
				uFramesAvailableForProducer = m_uCapacity - (m_uFramesInput - m_uFramesOutput);
			}
			else
			{
				// counter wrap around situation
				uFramesAvailableForProducer = m_uCapacity - ((UINT_MAX - m_uFramesOutput + 1) + m_uFramesInput);
			} 
			return uFramesAvailableForProducer;
		}

		// Total frame capacity of the ring buffer
		unsigned int AkAudioRingBuffer::Capacity()
		{
			return m_uCapacity;
		}

		// Number of channels stored
		unsigned int AkAudioRingBuffer::NumChannels()
		{
			return m_uNumChannels;
		}

		// Get interleaved data (data is copied to external location). 
		AKRESULT AkAudioRingBuffer::GetChannels(unsigned int in_uFramesToGet, float * io_pfBuffer, unsigned int in_StartChannel, unsigned int in_uNumChannels)
		{
			if (in_StartChannel + in_uNumChannels > m_uNumChannels)
			{
				LOG_ERROR("Channel indexing out of range");
				return AK_Fail;
			}
			unsigned int uFramesAvailable = FramesAvailableForConsumer();
			if (in_uFramesToGet > uFramesAvailable)
			{
				LOG_DEBUG("Audio ring buffer request %d not fullfilled with only %d frames available", in_uFramesToGet, uFramesAvailable);
				return AK_NoDataReady;
			}

			LOG_RB_STATE("GetChannel-Start");

			unsigned int uFramesBeforeWrap = m_uCapacity - m_uReadIndex;
			const unsigned int uFramesToCopy = AkMin(in_uFramesToGet, uFramesBeforeWrap);
			const unsigned int uNumChannels = m_uNumChannels;
			const unsigned int uNumOutChannels = in_uNumChannels;
			unsigned int uReadIndex = m_uReadIndex; // Real read index is advanced by AdvanceFrames()
			float * const AK_RESTRICT pfChannelDest = io_pfBuffer;
			const float * const AK_RESTRICT pfRingBufferSource = &m_pfRingBuffer[uReadIndex*m_uNumChannels];

			// Optimization note: the dominant use case (to optimize) is uNumOutChannels == uNumChannels and start channel is 0 (all channel being copied), this step falls down to memcpy
			// SIMD note: both source and destination guaranteed to be SIMD aligned by use of plugin allocator
			// SIMD note: channel interleaved format makes SIMD difficult in unknown numchannel context

			if (uNumOutChannels == uNumChannels && in_StartChannel == 0)
			{
				unsigned int uCopySize = uFramesToCopy*uNumOutChannels*sizeof(float);
				AKPLATFORM::AkMemCpy(pfChannelDest, pfRingBufferSource, uCopySize);
			}
			else
			{
				// Internal data format is interleaved
				for (unsigned int i = 0; i < uFramesToCopy; i++)
				{
					for (unsigned int j = 0; j < uNumOutChannels; j++)
					{
						pfChannelDest[i*uNumOutChannels + j] = pfRingBufferSource[i*uNumChannels + j + in_StartChannel];
					}
				}
			}
			// Increment and wrap
			uReadIndex += uFramesToCopy;
			if (uReadIndex >= m_uCapacity)
			{
				uReadIndex = 0;
			}
			unsigned int uFramesLeft = in_uFramesToGet - uFramesToCopy;
			if (uFramesLeft)
			{
				float * const AK_RESTRICT pfRingBufferSource = &m_pfRingBuffer[uReadIndex*m_uNumChannels];
				// Optimization note: the dominant use case (to optimize) is uNumOutChannels == uNumChannels and start channel is 0 (all channel being copied), this step falls down to memcpy
				if (uNumOutChannels == uNumChannels && in_StartChannel == 0)
				{
					unsigned int uCopySize = uFramesLeft*uNumOutChannels*sizeof(float);
					AKPLATFORM::AkMemCpy(pfChannelDest + uFramesToCopy*uNumOutChannels, pfRingBufferSource, uCopySize);
				}
				else
				{
					// Internal data format is interleaved
					for (unsigned int i = 0; i < uFramesLeft; i++)
					{
						for (unsigned int j = 0; j < uNumOutChannels; j++)
						{
							pfChannelDest[(i + uFramesToCopy)*uNumOutChannels + j] = pfRingBufferSource[i*uNumChannels + j + in_StartChannel];
						}
					}
				}
			}

#ifdef ENABLE_AUDIO_BUFFER_CHECKS
			float fFirstSampleOut = io_pfBuffer[0];
			float fSampleDiff = abs(m_fLastSampleOut - fFirstSampleOut);
			LOG_DEBUG("RB m_uFrameOut: %d fSampleDiff %f", m_uFramesOutput, fSampleDiff);
			m_fLastSampleOut = io_pfBuffer[(in_uFramesToGet - 1)*in_uNumChannels];

			unsigned int uOutOfRangeSamplesCount = CheckOutOfRangeSamples(io_pfBuffer, in_uFramesToGet, in_uNumChannels);
			if (uOutOfRangeSamplesCount)
			{
				LOG_WARN("Buffer contains %d out of range samples", uOutOfRangeSamplesCount);
			}
#endif // ENABLE_AUDIO_BUFFER_CHECKS

			// Persisted read index advanced by advance frames to allow multiple gets of the same data (different channels)
			LOG_RB_STATE("GetChannel-End");

			return AK_DataReady;
		}

		// Get deinterleaved data (data is copied to external location). Does not advance read location. Channel buffers are assumed to be contiguous (single pointer for start)
		AKRESULT AkAudioRingBuffer::GetChannelsDeinterleaved(unsigned int in_uFramesToGet, float * io_pfBuffer, unsigned int in_uMaxFrames, unsigned int in_StartChannel, unsigned int in_uNumChannels)
		{
			if (in_StartChannel + in_uNumChannels > m_uNumChannels)
			{
				LOG_ERROR("Channel indexing out of range");
				return AK_Fail;
			}
			unsigned int uFramesAvailable = FramesAvailableForConsumer();
			if (in_uFramesToGet > uFramesAvailable)
			{
				LOG_DEBUG("Audio ring buffer request %d not fullfilled with only %d frames available", in_uFramesToGet, uFramesAvailable);
				return AK_NoDataReady;
			}

			LOG_RB_STATE("GetChannel-Start");

			unsigned int uFramesBeforeWrap = m_uCapacity - m_uReadIndex;
			const unsigned int uFramesToCopy = AkMin(in_uFramesToGet, uFramesBeforeWrap);
			const unsigned int uNumChannels = m_uNumChannels;
			const unsigned int uNumOutChannels = in_uNumChannels;
			unsigned int uReadIndex = m_uReadIndex; // Real read index is advanced by AdvanceFrames()
			const float * const AK_RESTRICT pfRingBufferSource = &m_pfRingBuffer[uReadIndex*m_uNumChannels];

			// SIMD note: both source and destination guaranteed to be SIMD aligned by use of plugin allocator
			// SIMD note: channel interleaved format makes SIMD difficult in unknown numchannel context

			// Internal data format is interleaved
			for (unsigned int j = 0; j < uNumOutChannels; j++)
			{
				float * const AK_RESTRICT pfChannelDest = io_pfBuffer + j*in_uMaxFrames;
				for (unsigned int i = 0; i < uFramesToCopy; i++)
				{
					AkReal32 fSourceVal = pfRingBufferSource[i*uNumChannels + j + in_StartChannel];
					pfChannelDest[i] = fSourceVal;
				}
			}

			// Increment and wrap
			uReadIndex += uFramesToCopy;
			if (uReadIndex >= m_uCapacity)
			{
				uReadIndex = 0;
			}
			unsigned int uFramesLeft = in_uFramesToGet - uFramesToCopy;
			if (uFramesLeft)
			{
				float * const AK_RESTRICT pfRingBufferSource = &m_pfRingBuffer[uReadIndex*m_uNumChannels];

				for (unsigned int j = 0; j < uNumOutChannels; j++)
				{
					float * const AK_RESTRICT pfChannelDest = io_pfBuffer + j*in_uMaxFrames + uFramesToCopy;
					for (unsigned int i = 0; i < uFramesLeft; i++)
					{
						AkReal32 fSourceVal = pfRingBufferSource[i*uNumChannels + j + in_StartChannel];
						pfChannelDest[i] = fSourceVal;
					}
				}
			}

#ifdef ENABLE_AUDIO_BUFFER_CHECKS
			unsigned int uOutOfRangeSamplesCount = CheckOutOfRangeSamples(io_pfBuffer, in_uFramesToGet, in_uNumChannels);
			if (uOutOfRangeSamplesCount)
			{
				LOG_WARN("Buffer contains %d out of range samples", uOutOfRangeSamplesCount);
			}
#endif // ENABLE_AUDIO_BUFFER_CHECKS

			// Persisted read index advanced by advance frames to allow multiple gets of the same data (different channels)
			LOG_RB_STATE("GetChannel-End");

			return AK_DataReady;
		}


		// Push interleaved data (data is copied to RB). 
		AKRESULT AkAudioRingBuffer::PushChannels(unsigned int in_uFramesToPush, float * io_pfBuffer, unsigned int in_StartChannel, unsigned int in_uNumChannels)
		{
			if (in_StartChannel + in_uNumChannels > m_uNumChannels)
			{
				LOG_ERROR("Channel indexing out of range");
				return AK_Fail;
			}

			// The assumption is that the caller verifies space available before trying to push (otherwise fail)
			unsigned int uFramesFree = FramesRemainingForProducer();
			if (in_uFramesToPush > uFramesFree)
			{
				LOG_ERROR("Attempt to push more frames than possible into RB");
				return AK_Fail;
			}

			LOG_RB_STATE("PushChannel-Start");

			unsigned int uFramesBeforeWrap = m_uCapacity - m_uWriteIndex;
			const unsigned int uFramesToCopy = AkMin(in_uFramesToPush, uFramesBeforeWrap);
			const unsigned int uNumOutChannels = m_uNumChannels;
			const unsigned int uNumChannels = in_uNumChannels;
			unsigned int uWriteIndex = m_uWriteIndex; // Real write index is advanced by AdvanceWriteFrames()
			const float * const AK_RESTRICT pfChannelSource = io_pfBuffer;
			float * AK_RESTRICT pfRingBufferDest = &m_pfRingBuffer[uWriteIndex*m_uNumChannels];
			// Internal data format is interleaved
			for (unsigned int i = 0; i < uFramesToCopy; i++)
			{
				for (unsigned int j = 0; j < uNumOutChannels; j++)
				{
					// Other way around???
					pfRingBufferDest[i*uNumOutChannels + j] = pfChannelSource[i*uNumChannels + j + in_StartChannel];
				}
			}
			// Increment and wrap
			uWriteIndex += uFramesToCopy;
			if (uWriteIndex >= m_uCapacity)
			{
				uWriteIndex = 0;
			}
			unsigned int uFramesLeft = in_uFramesToPush - uFramesToCopy;
			if (uFramesLeft)
			{
				float * const AK_RESTRICT pfRingBufferSource = &m_pfRingBuffer[uWriteIndex*m_uNumChannels];
				// Internal data format is interleaved
				for (unsigned int i = 0; i < uFramesLeft; i++)
				{
					for (unsigned int j = 0; j < uNumOutChannels; j++)
					{
						pfRingBufferSource[i*uNumChannels + j + in_StartChannel] = pfChannelSource[(i + uFramesToCopy)*uNumOutChannels + j];
					}
				}
			}

#ifdef ENABLE_AUDIO_BUFFER_CHECKS
			float fFirstSampleOut = io_pfBuffer[0];
			float fSampleDiff = abs(m_fLastSampleOut - fFirstSampleOut);
			LOG_DEBUG("RB m_uFrameOut: %d fSampleDiff %f", m_uFramesOutput, fSampleDiff);
			m_fLastSampleOut = io_pfBuffer[(in_uFramesToPush - 1)*in_uNumChannels];

			unsigned int uOutOfRangeSamplesCount = CheckOutOfRangeSamples(io_pfBuffer, in_uFramesToPush, in_uNumChannels);
			if (uOutOfRangeSamplesCount)
			{
				LOG_WARN("Buffer contains %d out of range samples", uOutOfRangeSamplesCount);
			}
#endif // ENABLE_AUDIO_BUFFER_CHECKS

			// Persisted read index advanced by advance frames to allow multiple push of the same data (different channels)
			LOG_RB_STATE("PushChannel-End");

			return AK_DataReady;
		}

		// Push interleaved data (data is cached to RB location). 
		AKRESULT AkAudioRingBuffer::PushFrames(unsigned int in_uFramesToPush, float * in_pfBuffer)
		{	
			// The assumption is that the caller verifies space available before trying to push (otherwise fail)
			unsigned int uFramesFree = FramesRemainingForProducer();
			if (in_uFramesToPush > uFramesFree)
			{
				LOG_ERROR("Attempt to push more frames than possible into RB");
				return AK_Fail;
			}

			LOG_RB_STATE("PushFrames-Start");

#ifdef ENABLE_AUDIO_BUFFER_CHECKS
			unsigned int uOutOfRangeSamplesCount = CheckOutOfRangeSamples(in_pfBuffer, in_uFramesToPush, m_uNumChannels);
			if (uOutOfRangeSamplesCount)
			{
				LOG_WARN("Buffer contains %d out of range samples", uOutOfRangeSamplesCount);
			}
#endif
			unsigned int uFramesBeforeWrap = m_uCapacity - m_uWriteIndex;
			unsigned int uFramesToCopy = AkMin(in_uFramesToPush, uFramesBeforeWrap);
			AKPLATFORM::AkMemCpy(&m_pfRingBuffer[m_uWriteIndex*m_uNumChannels], in_pfBuffer, uFramesToCopy*m_uNumChannels*sizeof(float));
			// Increment and wrap
			m_uWriteIndex += uFramesToCopy;
			if (m_uWriteIndex >= m_uCapacity)
			{
				m_uWriteIndex = 0;
				LOG_DEBUG("Write index wrap");
			}
			unsigned int uFramesLeft = in_uFramesToPush - uFramesToCopy;
			if (uFramesLeft)
			{
				AKPLATFORM::AkMemCpy(&m_pfRingBuffer[m_uWriteIndex*m_uNumChannels], in_pfBuffer + uFramesToCopy*m_uNumChannels, uFramesLeft*m_uNumChannels*sizeof(float));
				m_uWriteIndex += uFramesLeft;
				if (m_uWriteIndex >= m_uCapacity)
				{
					m_uWriteIndex = 0;
					LOG_DEBUG("Write index wrap");
				}
			}

#ifdef ENABLE_AUDIO_BUFFER_CHECKS 
			float fFirstSampleInput = in_pfBuffer[0];
			float fSampleDiff = abs(m_fLastSampleInput - fFirstSampleInput);
			LOG_DEBUG("RB m_uFramesInput: %d fSampleDiff %f", m_uFramesInput, fSampleDiff);
			m_fLastSampleInput = in_pfBuffer[(in_uFramesToPush - 1)*m_uNumChannels];
#endif
			{
				AkAutoLock<CAkLock> AccessLock(m_Mutex);
				m_uFramesInput += in_uFramesToPush;
			}
			

			LOG_RB_STATE("PushFrames-End");

			return AK_Success;
		}

		// Internal storage available for writing again after call (advance read pointer).
		void AkAudioRingBuffer::AdvanceReadFrames(unsigned int in_uFramesToAdvance)
		{
			AkAutoLock<CAkLock> AccessLock(m_Mutex);

			LOG_RB_STATE("AdvanceReadFrames-Start");

			m_uReadIndex += in_uFramesToAdvance;
			if (m_uReadIndex >= m_uCapacity)
			{
				m_uReadIndex -= m_uCapacity;
				LOG_DEBUG("Read index wrap");
			}
			m_uFramesOutput += in_uFramesToAdvance;

			LOG_RB_STATE("AdvanceReadFrames-End");
		}

		// Internal storage available for reading again after call (advance write pointer).
		void AkAudioRingBuffer::AdvanceWriteFrames(unsigned int in_uFramesToAdvance)
		{
			AkAutoLock<CAkLock> AccessLock(m_Mutex);

			LOG_RB_STATE("AdvanceWriteFrames-Start");

			m_uWriteIndex += in_uFramesToAdvance;
			if (m_uWriteIndex >= m_uCapacity)
			{
				m_uWriteIndex -= m_uCapacity;
				LOG_DEBUG("Write index wrap");
			}
			m_uFramesInput += in_uFramesToAdvance;

			LOG_RB_STATE("AdvanceWriteFrames-End");
		}
	}
}
