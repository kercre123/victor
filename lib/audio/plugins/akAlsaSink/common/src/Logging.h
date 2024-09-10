//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Logging for debugging/tracing purposes

#pragma once

#include <stdio.h>
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkAutoLock.h>

#define DEBUG_LOG 1
#define AK_ALSA_SINK_LOGS 1

/// Audiokinetic namespace
namespace AK
{
	/// Audiokinetic custom hardware namespace
	namespace CustomHardware
	{

		typedef enum LogLevel
		{
			LogLevelDebug = 0,
			LogLevelTrace,
			LogLevelInfo,
			LogLevelWarn,
			LogLevelError,
		} LogLevel;

		// Meyers' Singleton
		class Logging
		{
		public:
			static Logging & Instance()
			{
				static Logging Logger;
				return Logger;
			}

			void SetLogLevel(LogLevel in_level);
			// TODO: Bring back file logging if necessary (currently just a screen logger)
			void Log(LogLevel in_level, const char * levelStr, const char *file, int line, const char *msg, ...);

		private:

			Logging(); // hidden (singleton)
			~Logging(); // hidden (singleton)
			Logging(const Logging&); // hidden (singleton)
			Logging& operator=(const Logging&); // hidden (singleton)

			// Helper
			static int GetTimeStamp(char * in_pTimeStampBuffer, unsigned int in_uBuffersize);

			LogLevel m_eLogLevel;
		};

#define LOG(level, logleveltext, file, line, message, ...) Logging::Instance().Log(level,logleveltext, file, line, message, ##__VA_ARGS__)

#if (!defined(AK_OPTIMIZED) || defined(DEBUG_LOG)) && defined(AK_ALSA_SINK_LOGS)
#define AK_LOG_DEBUG(m, ...)              LOG(LogLevelDebug, "DEBUG", __FILE__, __LINE__, m, ##__VA_ARGS__)
#define AK_LOG_TRACE(m, ...)              LOG(LogLevelTrace, "TRACE", __FILE__, __LINE__, m, ##__VA_ARGS__)
#define AK_LOG_INFO(m, ...)               LOG(LogLevelInfo, "INFO", __FILE__, __LINE__, m, ##__VA_ARGS__)
#define AK_LOG_WARN(m, ...)               LOG(LogLevelWarn, "WARNING", __FILE__, __LINE__, m, ##__VA_ARGS__)
#define AK_LOG_ERROR(m, ...)              LOG(LogLevelError, "ERROR", __FILE__, __LINE__, m, ##__VA_ARGS__)
#else
#define LOG_DEBUG(m, ...)
#define LOG_TRACE(m, ...)
#define LOG_INFO(m, ...)
#define LOG_WARN(m, ...)
#define LOG_ERROR(m, ...)
#endif


	}
}
