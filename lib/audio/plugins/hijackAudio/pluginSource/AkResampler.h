/***********************************************************************
 The content of this file includes source code for the sound engine
 portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
 Two Source Code" as defined in the Source Code Addendum attached
 with this file.  Any use of the Level Two Source Code shall be
 subject to the terms and conditions outlined in the Source Code
 Addendum and the End User License Agreement for Wwise(R).
 
 Version: v2016.2.0  Build: 5972
 Copyright (c) 2006-2016 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkResampler.h
//
// Combines software resampling and pitch shifting opreation in one algorithm using linear interpolation.
// Routines support AkReal32 sample type as well as AkUInt8 and AkInt16
// Assumes same thread will call both SetPitch and Execute (not locking).
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_RESAMPLER_H_
#define _AK_RESAMPLER_H_

//#include "AkInternalPitchState.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AkInternalPitchState Code
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Fixed point interpolation index
#define FPBITS 16
#define FPMUL (1<<FPBITS)
#define FPMASK (FPMUL-1)
#define SINGLEFRAMEDISTANCE (FPMUL)

// Class members used at runtime required by all platforms grouped together
#ifdef AK_XBOX360
__declspec(align(16))
#endif
struct AkInternalPitchState
{
  // Note: Keep this first to guarantee alignement on 16 bytes for XBox360
#ifndef AK_VOICE_MAX_NUM_CHANNELS
  union
  {
    AkReal32	fLastValueStatic[AK_STANDARD_MAX_NUM_CHANNELS];
    AkInt16		iLastValueStatic[AK_STANDARD_MAX_NUM_CHANNELS];
  };
  
  union									// (In/Out) Last buffer values depending on format
  {
    AkReal32 *	fLastValue;
    AkInt16	 *	iLastValue;
  };
#else
  union
  {
    AkReal32	fLastValue[AK_STANDARD_MAX_NUM_CHANNELS];
    AkInt16		iLastValue[AK_STANDARD_MAX_NUM_CHANNELS];
  };
#endif
  
  // Pitch internal state
  AkUInt32 uInFrameOffset;				// (In/Out) Offset in buffer currently being read
  AkUInt32 uOutFrameOffset;				// (In/Out) Offset in buffer currently being filled
  
  AkUInt32 uFloatIndex;					// (In/Out) Fixed point index value
  AkUInt32 uCurrentFrameSkip;				// (In/Out) Current sample frame skip
  AkUInt32 uTargetFrameSkip;				// (In) Target frame skip
  AkUInt32 uInterpolationRampCount;		// (In/Out) Sample count for pitch interpolation (interpolation finished when == PITCHRAMPLENGTH)
  AkUInt32 uInterpolationRampInc;			// (In) increment to ramp count during interpolate pitch transition
  AkUInt32 uRequestedFrames;				// (In) Desired output frames (max frames)
  
#ifdef AK_PS3
  AkUInt32 uInOffset;						// (In) Byte offset to actual input data start (DMA aligned on 16 bytes)
  AkUInt32 uInValidFrames;				// (In/Out) Valid input frames
  void * pOutBuffer;						// (In) Where to DMA output buffer (not aligned on 16 bytes)
  AkUInt32 uOutValidFrames;				// (Out) Number of output frames produced
  AKRESULT eState;						// (Out) The return state of the algorithm
  AkChannelConfig channelConfig;			// (In) Channel config
  AkUInt32 uOutMaxFrames;					// (In) Distance between output channels
#endif // PS3
#ifndef AK_VOICE_MAX_NUM_CHANNELS
  // Platform does not define a strict number of channels.
  bool bLastValuesAllocated;
#endif // !AK_VOICE_MAX_NUM_CHANNELS for flexible number of channels.
  
#if defined(AK_IOS) && !defined (AK_CPU_ARM_NEON)
  void* m_pAudioConverter;		// audio converter reference
#endif
  
} AK_ALIGN_DMA;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AKResampler Code
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Internal pitch state that can be passed on to following sources for seemless sample accurate playback
struct AkSharedPitchState
{
  // Last buffer value (1 per channel). Note fixed point format routines need to convert range when retrieving/storing this value.
  // Fixed point index value
  AkUInt32 uFloatIndex;
  // Pitch interpolation variables
  AkUInt32 uCurrentFrameSkip;					// Current sample frame skip
  AkUInt32 uTargetFrameSkip;					// Target frame skip
  AkUInt32 uInterpolationRampCount;			// Sample count for pitch interpolation (interpolation finished when == PITCHRAMPLENGTH)
  AkReal32 fTargetPitchVal;					// Target pitch value
  AkReal32 fSampleRateConvertRatio;
  bool	 bFirstSetPitch;					// Flags first set pitch received
};

enum PitchOperatingMode
{
  PitchOperatingMode_Bypass = 0,
  PitchOperatingMode_Fixed,
  PitchOperatingMode_Interpolating,
  NumPitchOperatingMode,
};


enum InputDataType
{
  I16_1Chan,
  I16_2Chan,
  I16_1To4Chan,
  I16_5To8Chan,
  Native_1Chan,
  Native_2Chan,
  Native_1To4Chan,
  Native_5To8Chan,
  NumInputDataType,
};

#define ISI16TYPE( __index__ ) (((__index__) == I16_1Chan) || ((__index__) == I16_2Chan) || ((__index__) == I16_1To4Chan) || ((__index__) == I16_5To8Chan))
#define ISNATIVETYPE( __index__ ) (((__index__) == Native_1Chan) || ((__index__) == Native_2Chan) || ((__index__) == Native_1To4Chan) || ((__index__) == Native_5To8Chan))

//#define PERFORMANCE_BENCHMARK

class CAkResampler
{
public:
  CAkResampler();
  ~CAkResampler();
  
#if defined(AK_CPU_X86) && !defined(AK_IOS)
  static void InitDSPFunctTable();
#endif
  
  AKRESULT Init( AkAudioFormat * io_pFormat, AkUInt32 in_uSampleRate );
  void Term();
  
  // Internal pitch state set/get
  void SwitchTo( const AkAudioFormat & in_fmt, AkReal32 in_fPitch, AkAudioBuffer * io_pIOBuffer, AkUInt32 in_uSampleRate );
  
  // Execute
  // Resampling and pitch shifting is performed and input size consumed is returned
  // Returns Ak_DataNeeded if output buffer is not full yet
  // Returns Ak_DataReady if it is full and ready to be passed on
#ifdef AK_PS3
  void ExecutePS3( AkAudioBuffer * io_pInBuffer, AkAudioBuffer * io_pOutBuffer, struct AkVPLState & io_state );
#else
  AKRESULT Execute( AkAudioBuffer * io_pInBuffer, AkAudioBuffer * io_pOutBuffer );
  bool IsPostDeInterleaveRequired();
  AKRESULT DeinterleaveAndSwapOutput( AkAudioBuffer * io_pIOBuffer );
  static AKRESULT InterleaveAndSwapOutput( AkAudioBuffer * io_pIOBuffer );
#endif
  
  // Helper for computing frame skip. Pads against 0 (which would result in div by 0).
  AkUInt32 PitchToFrameSkip(AkReal32 in_fPitchVal);
  
  // Adjust requested time according to pitch parameter.
  
  AkForceInline void TimeInputToOutput( AkUInt32 & io_uFrames )
  {
    io_uFrames = (AkUInt32) ( io_uFrames / ((AkReal32)m_InternalPitchState.uCurrentFrameSkip / FPMUL ) + 0.5f ); // round to nearest
  }
  
  AkForceInline void TimeOutputToInput( AkUInt32 & io_uFrames )
  {
    io_uFrames = (AkUInt32) ( io_uFrames * ((AkReal32)m_InternalPitchState.uCurrentFrameSkip / FPMUL ) + 0.5f ); // round to nearest
  }
  
  void SetPitch( AkReal32 in_fPitchVal, bool in_bInterpolate );
  // SetPitch without triggering interpolation or even setting the right DSP routine (used for time skipping).
  void SetPitchForTimeSkip( AkReal32 in_fPitchVal );
  
  void SetRequestedFrames( AkUInt32 in_uRequestedFrames ){ m_InternalPitchState.uRequestedFrames = in_uRequestedFrames; }
  AkUInt32 GetRequestedFrames() { return m_InternalPitchState.uRequestedFrames; }
  void SetOutputBufferOffset( AkUInt32 in_ulNumFrameOffset ){ m_InternalPitchState.uOutFrameOffset = in_ulNumFrameOffset; }
  void SetInputBufferOffset( AkUInt32 in_ulNumFrameOffset ){ m_InternalPitchState.uInFrameOffset = in_ulNumFrameOffset; }
  AkUInt32 GetInputFrameOffset() { return m_InternalPitchState.uInFrameOffset; }
  void ResetOffsets();
  bool HasOffsets();
  
  AkReal32 GetLastRate() const;
  
#ifndef AK_PS3
private:
#endif
  AkInternalPitchState m_InternalPitchState;	// These only needs to be public on PS3
  PitchOperatingMode	m_PitchOperationMode;	// Bypass, Fixed or Interpolating
  
private:
  void SetLastValues( AkReal32 * in_pfLastValues );
  void GetLastValues( AkReal32 * out_pfLastValues );
  AkUInt8 GetDSPFunctionIndex( const AkAudioFormat & in_fmt ) const;
  
#if defined(AK_IOS) && !defined(AK_CPU_ARM_NEON)
  void SetupAudioConverter(const AkAudioFormat & in_fmt, AkUInt32 in_uSampleRate);
  void DisposeAudioConverter(void* in_pConverter);
#endif
  
  AkReal32			m_fSampleRateConvertRatio;			// Sample rate conversion factor (source/target)
  AkReal32			m_fTargetPitchVal;					// Target pitch value
  AkUInt8				m_DSPFunctionIndex;					// Index of appropriate function given numChannels and data type (same for all 3 operation modes)
  AkUInt8				m_uNumChannels;
  AkUInt8				m_uInputBlockAlign;
  bool				m_bFirstSetPitch;					// Flags first set pitch received
  
#ifdef PERFORMANCE_BENCHMARK
  AkUInt32			m_uNumberCalls;
  AkReal32			m_fTotalTime;
#endif
};

#endif // _AK_RESAMPLER_H_
