//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Logging implementation for debugging purposes

#include "Logging.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#ifdef AK_LINUX
#include <sys/time.h>
#include <stdarg.h>
#endif
#if defined(VICOS)
#include "util/logging/logging.h"
#endif

#if defined(AK_WIN)
#define SafeSprintf(__buffer__, __sizeOfBuffer__, __format__, ...) sprintf_s(__buffer__, __sizeOfBuffer__, __format__,__VA_ARGS__ );
#endif

/// Audiokinetic namespace
namespace AK
{
	/// Audiokinetic custom hardware namespace
	namespace CustomHardware
	{
#define MSGBUFFERSIZE 16384

		// We cannot use LOG_ERROR() to log error occuring in the logger.
#define INTERNAL_LOG(msg, ...) printf("%s:%d:" msg "\n", __FUNCTION__,__LINE__, ##__VA_ARGS__);

		Logging::Logging()
		{
#ifndef AK_OPTIMIZED
			m_eLogLevel = LogLevelInfo;
#else
			m_eLogLevel = LogLevelWarn;
#endif
		}

		Logging::~Logging()
		{
		}

		int Logging::GetTimeStamp(char *ptimestampbuffer, unsigned int buffersize)
		{
#if defined(AK_WIN)
			SYSTEMTIME time;
			char *format = "%04d-%02d-%02d %02d:%02d:%02d.%03d";
			GetLocalTime(&time);
			return sprintf_s(ptimestampbuffer, buffersize, format, time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
#elif defined(AK_LINUX)
			timeval curTime;
			gettimeofday(&curTime, NULL);
			int milli = curTime.tv_usec / 1000;
			char buffer [80];
			strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&curTime.tv_sec));
			return sprintf(ptimestampbuffer, "%s.%d", buffer, milli);
#endif
		}

		void Logging::SetLogLevel(LogLevel in_level)
		{
			m_eLogLevel = in_level;
		}

		void Logging::Log(LogLevel in_level, const char * levelStr, const char *file, int line, const char *msg, ...)
		{
			//Not need to do a mutex lock here, all below function are threadsafe and buffer are on the stack
			//To ensure a sequence of log in a mutithread process please do a flockfile(3)/funlockfile(3) at the application level
			//surrounding the logs call.

			if (in_level < m_eLogLevel)
			{
				return;
			}

			va_list arglist;
			va_start(arglist, msg);
			char msgBuffer[MSGBUFFERSIZE];
#if defined(AK_WIN)
			vsnprintf_s(msgBuffer, MSGBUFFERSIZE, MSGBUFFERSIZE, msg, arglist);
#else
			vsnprintf(msgBuffer, MSGBUFFERSIZE, msg, arglist);
#endif
			va_end(arglist);

			AkThreadID threadId = AKPLATFORM::CurrentThread();

			char timestamp[256];
			GetTimeStamp(timestamp, 256);
			const char * filenameShort = NULL;
			filenameShort = strrchr(file, '\\');
			if (filenameShort == NULL)
			{
				filenameShort = file; // Not found use whole file
			}
			else
			{
				filenameShort += 1;   // Found skip current character
			}

#if defined(VICOS)
			switch(in_level)
			{
				case LogLevelDebug:
				case LogLevelTrace:
					PRINT_CH_DEBUG("Audio", "AkAlsaSink", "TId:%x L:%d %s", (int)threadId, line, msgBuffer);
					break;
				case LogLevelInfo:
					PRINT_CH_INFO("Audio", "AkAlsaSink", "TId:%x L:%d %s", (int)threadId, line, msgBuffer);
					break;
				case LogLevelWarn:
					PRINT_NAMED_WARNING("AkAlsaSink", "TId:%x L:%d %s", (int)threadId, line, msgBuffer);
					break;
				case LogLevelError:
					PRINT_NAMED_ERROR("AkAlsaSink", "TId:%x L:%d %s", (int)threadId, line, msgBuffer);
					break;
			}
#else
			printf("%s %x %s:%d %s:%s\n", timestamp, (int)threadId, filenameShort, line, levelStr, msgBuffer);
#endif
#if !defined(AK_OPTIMIZED) && !defined(AK_LINUX)
			char LogBuffer[MSGBUFFERSIZE];
			SafeSprintf(LogBuffer, MSGBUFFERSIZE, "%s %x %s:%d %s:%s\n", timestamp, (int)threadId, filenameShort, line, levelStr, msgBuffer);

			AKPLATFORM::OutputDebugMsg(LogBuffer);
#endif
		}

	} // namespace CustomHardware
} // // Audiokinetic namespace
