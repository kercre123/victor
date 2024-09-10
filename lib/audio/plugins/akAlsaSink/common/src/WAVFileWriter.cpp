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

#include "WAVFileWriter.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

namespace AK
{
	namespace CustomHardware
	{

		WAVFileWriter::WAVFileWriter()
			: m_pCaptureFile(NULL)
			, m_uFramesWritten(0)
			, m_uCaptureStreamDataSize(0)
		{
			AKPLATFORM::AkMemSet(&m_WAVHeader, 0, sizeof(m_WAVHeader));
		}

		WAVFileWriter::~WAVFileWriter()
		{
			if (m_pCaptureFile != NULL)
			{
				fclose(m_pCaptureFile);
				m_pCaptureFile = NULL;
			}
		}

		AKRESULT WAVFileWriter::Start(const char * in_pCaptureFileName, unsigned int in_uNumChannels, unsigned int in_uSampleRate)
		{
			if (m_pCaptureFile)
				return AK_Success;

			m_pCaptureFile = fopen(in_pCaptureFileName, "wb");
			if (m_pCaptureFile == NULL)
				return AK_Fail;

			AkUInt32 uBlockAlign = in_uNumChannels * sizeof(float);
			m_WAVHeader.RIFF.ChunkId = RIFFChunkId;
			m_WAVHeader.RIFF.dwChunkSize = 0XFFFFFFFF;//sizeof(AkWAVEFileHeader) + in_uDataSize - 8;
			m_WAVHeader.uWAVE = WAVEChunkId;
			m_WAVHeader.fmt.ChunkId = fmtChunkId;
			m_WAVHeader.fmt.dwChunkSize = sizeof(WaveFormatEx);
			m_WAVHeader.fmtHeader.wFormatTag = AK_WAVE_FORMAT_IEEE_FLOAT;
			m_WAVHeader.fmtHeader.nChannels = (AkUInt16) in_uNumChannels;
			m_WAVHeader.fmtHeader.nSamplesPerSec = in_uSampleRate;
			m_WAVHeader.fmtHeader.nAvgBytesPerSec = in_uSampleRate * uBlockAlign;
			m_WAVHeader.fmtHeader.nBlockAlign = (AkUInt16) uBlockAlign;
			m_WAVHeader.fmtHeader.wBitsPerSample = (AkUInt16) 32;
			m_WAVHeader.fmtHeader.cbSize = 0;
			m_WAVHeader.data.ChunkId = dataChunkId;
			m_WAVHeader.data.dwChunkSize = 0XFFFFFFFF;//in_uDataSize; 

			// write dummy WAVE header block out 
			// we will be going back to re-write the block when sizes have been resolved
			AkUInt32 uOutSize = fwrite(&m_WAVHeader, 1, sizeof(AkWAVEFileHeader), m_pCaptureFile);
			if (uOutSize != sizeof(AkWAVEFileHeader))
			{
				fclose(m_pCaptureFile);
				m_pCaptureFile = NULL;
				return AK_Fail;
			}

			return AK_Success;
		}

		AKRESULT WAVFileWriter::Stop()
		{
			if (!m_pCaptureFile)
				return AK_Fail;

			// Update header with size written
			unsigned int uDataSize = m_uFramesWritten * m_WAVHeader.fmtHeader.nChannels * sizeof(float);
			m_WAVHeader.RIFF.dwChunkSize = sizeof(AkWAVEFileHeader) + uDataSize - 8;
			m_WAVHeader.data.dwChunkSize = uDataSize;

			// Seek back to start of stream 
			int iSeekResult = fseek(m_pCaptureFile, 0, SEEK_SET);
			if (iSeekResult == 0)
			{
				AkUInt32 uOutSize = fwrite(&m_WAVHeader, 1, sizeof(AkWAVEFileHeader), m_pCaptureFile);
			}

			// Kill stream, whether or not we succeeded to write updated header
			fclose(m_pCaptureFile);
			m_pCaptureFile = NULL;
			return AK_Success;
		}

		AKRESULT WAVFileWriter::WriteBuffer(float * in_pfData, AkUInt32 in_uNumFrames)
		{
			if (!m_pCaptureFile)
				return AK_Fail;

			unsigned int uInSize = in_uNumFrames*m_WAVHeader.fmtHeader.nChannels*sizeof(float);
			AkUInt32 uOutSize = fwrite(in_pfData, 1, uInSize, m_pCaptureFile);
			if ( uOutSize != uInSize)
				return AK_Fail;

			m_uFramesWritten += in_uNumFrames;
			return AK_Success;
		}
	} // namespace CustomHardware
} // namespace AK
