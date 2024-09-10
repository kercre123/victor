/***********************************************************************************************************************
 *
 *  AudioDefines
 *  Audio
 *
 *  Created by Jarrod Hatfield on 5/11/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 ***********************************************************************************************************************/

#ifndef __AnkiAudio_AudioDefines_H__
#define __AnkiAudio_AudioDefines_H__

#include <cstdio>

#ifdef ANDROID
#include <android/log.h>
#endif

//#include "util/logging/logging.h"
//#define AUDIO_ASSERT( assertion ) DASAssert( assertion )
//#define AUDIO_ERROR( error, args... ) do{ DASError( "AudioSystem", error, ##args ); assert( false ); } while(0)
//#define AUDIO_WARN( format, args... ) do{ DASWarn( "AudioSystem", format, ##args ); } while(0)
//#define AUDIO_EVENT( format, args... ) do{ DASEvent( "AudioSystem", format, ##args ); } while(0)

//#ifdef DEV_FEATURES_ALLOWED
  #define AUDIO_DEVELOPER_CODE 1
//#else
//  #define AUDIO_DEVELOPER_CODE 0
//#endif

#define AUDIO_DEBUGGING_ENABLED ( AUDIO_DEVELOPER_CODE && 0 )   // Print detailed audio information
#define AUDIO_LOGGING_ENABLED ( AUDIO_DEVELOPER_CODE && 0 )     // Print lightweight audio state information
#define DISABLE_AUDIO_SYSTEM ( AUDIO_DEVELOPER_CODE && 0 )      // Disable the audio system entirely
#define DISABLE_SOUNDS ( AUDIO_DEVELOPER_CODE && 0 )            // Disable the playback of sound events
#define DISABLE_MUSIC ( AUDIO_DEVELOPER_CODE && 0 )             // Disable the playback of music

#if AUDIO_DEBUGGING_ENABLED
  #ifdef ANDROID
    #define AUDIO_DEBUG( message, args... ) __android_log_print( ANDROID_LOG_INFO, "AudioSystem.Debugging", message, ##args )
  #else
    // #define AUDIO_DEBUG( message, args... ) DASInfo( "AudioSystem.Debugging", message, ##args )
    #define AUDIO_DEBUG( message, args... ) std::printf( message, ##args )
  #endif
#else
  #define AUDIO_DEBUG( message, ... )
#endif // AUDIO_LOGGING_ENABLED

#if ( AUDIO_LOGGING_ENABLED | AUDIO_DEBUGGING_ENABLED )
  #ifdef ANDROID
    #define AUDIO_LOG( message, args... ) __android_log_print( ANDROID_LOG_INFO, "AudioSystem.Logging", message, ##args )
  #else
    // #define AUDIO_LOG( message, args... ) DASInfo( "AudioSystem.Logging", message, ##args )
    #define AUDIO_LOG( message, args... ) std::printf( message "\n", ##args )
  #endif
#else
  #define AUDIO_LOG( message, ... )
#endif // AUDIO_LOGGING_ENABLED


#endif // __AnkiAudio_AudioDefines_H__
