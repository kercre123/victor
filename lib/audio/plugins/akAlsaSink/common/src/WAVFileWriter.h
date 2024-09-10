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

#include <AK/SoundEngine/Common/AkTypes.h>
#include <stdio.h>

namespace AK
{
	namespace CustomHardware
	{
		// Currently assumes format to be 32-bit float

		static const AkFourcc RIFXChunkId = AkmmioFOURCC('R', 'I', 'F', 'X');
		static const AkFourcc RIFFChunkId = AkmmioFOURCC('R', 'I', 'F', 'F');
		static const AkFourcc WAVEChunkId = AkmmioFOURCC('W', 'A', 'V', 'E');
		static const AkFourcc fmtChunkId = AkmmioFOURCC('f', 'm', 't', ' ');
		static const AkFourcc dataChunkId = AkmmioFOURCC('d', 'a', 't', 'a');

#define AK_WAVE_FORMAT_IEEE_FLOAT	3
#define MAXPATH_WAVFILENAME			256

#pragma pack(1)
		struct AkChunkHeader
		{
			AkFourcc	ChunkId;
			AkUInt32	dwChunkSize;
		};

		// This is a copy of WAVEFORMATEX
		struct WaveFormatEx
		{
			AkUInt16  	wFormatTag;
			AkUInt16  	nChannels;
			AkUInt32  	nSamplesPerSec;
			AkUInt32  	nAvgBytesPerSec;
			AkUInt16  	nBlockAlign;
			AkUInt16  	wBitsPerSample;
			AkUInt16    cbSize;	// size of extra chunk of data, after end of this struct
		};

		struct AkWAVEFileHeader
		{
			AkChunkHeader		RIFF;		// AkFourcc	ChunkId: FOURCC('RIFF')
			// AkUInt32	dwChunkSize: Size of file (in bytes) - 8
			AkFourcc			uWAVE;		// FOURCC('WAVE')
			AkChunkHeader		fmt;		// AkFourcc	ChunkId: FOURCC('fmt ')
			// AkUInt32	dwChunkSize: Total size (in bytes) of fmt chunk's content
			WaveFormatEx		fmtHeader;	// AkUInt16	wFormatTag: 0x0001 for linear PCM etc.
			// AkUInt16	nChannels: Number of channels (1=mono, 2=stereo etc.)
			// AkUInt32	nSamplesPerSec: Sample rate (e.g. 44100)
			// AkUInt32	nAvgBytesPerSec: nSamplesPerSec * nBlockAlign
			// AkUInt16 nBlockAlign: nChannels * nBitsPerSample / 8 for PCM
			// AkUInt16 wBitsPerSample: 8, 16, 24 or 32
			// AkUInt16 cbSize:	Size of extra chunk of data, after end of this struct
			AkChunkHeader		data;		// AkFourcc	ChunkId: FOURCC('data')
			// AkUInt32	dwChunkSize: Total size (in bytes) of data chunk's content
			// Sample data goes after this..
		};

#pragma pack()

		class WAVFileWriter
		{
		public:
			WAVFileWriter();
			~WAVFileWriter();

			AKRESULT Start(const char* in_CaptureFileName, unsigned int in_uNumChannels, unsigned int in_uSampleRate);
			AKRESULT Stop();
			AKRESULT WriteBuffer(float * in_pfData, AkUInt32 in_uNumFrames);

		private:
			AkUInt32			m_uCaptureStreamDataSize;			// Data size capture counter (bytes)
			FILE *				m_pCaptureFile;						
			AkWAVEFileHeader	m_WAVHeader;
			unsigned int		m_uFramesWritten;
		};
	} // namespace CustomHardware
} // namespace AK
