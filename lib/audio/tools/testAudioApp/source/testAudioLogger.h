/**
 * File: testAudioLogger.h
 *
 * Author: Jordan Rivas
 * Created: 1/16/18
 *
 * Description: Logger for standalone command line app to test Anki's Audio Library.
 *
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Anki_TestAudioLogger_H__
#define __Anki_TestAudioLogger_H__


namespace TestAudioLogger {
 static bool EnableDebugLogs = false;
}

#define LOG_CHANNEL "TestAudio"

#define LOG_DEBUG(msg, ...)  if(TestAudioLogger::EnableDebugLogs) { printf((LOG_CHANNEL " -D- " msg "\n"), ##__VA_ARGS__); }
#define LOG_INFO(msg, ...)  printf((LOG_CHANNEL " -I- " msg "\n"), ##__VA_ARGS__)
#define LOG_ERROR(msg, ...)  printf((LOG_CHANNEL " -E- " msg "\n"), ##__VA_ARGS__)


#endif /* __Anki_TestAudioLogger_H__ */
