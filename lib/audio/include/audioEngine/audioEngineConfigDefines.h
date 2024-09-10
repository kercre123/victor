/*
 * File: audioEngineConfigDefines.h
 *
 * Author: Jordan Rivas
 * Created: 7/7/16
 *
 * Description: Setup Audio Engine Types
 *              - Types mirror those of Wwise, therefore don't change without confirming
 *
 * Copyright: Anki, Inc. 2016
 *
 */

#ifndef __Audio_AudioEngineConfigDefines_h
#define __Audio_AudioEngineConfigDefines_h
// Turn off unused-function to allow exported helper functions
#pragma clang diagnostic ignored "-Wunused-function"

#include "audioEngine/audioExport.h"
#include "audioEngine/audioTypes.h"
#include <functional>
#include <cassert>
#include <string>
#include <vector>
#include <unordered_map>


namespace Anki {
namespace AudioEngine {

using AudioFilePathResolverFunc = std::function<bool(const std::string& in_name, std::string& out_path)>;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum class AUDIOENGINE_EXPORT AudioLocaleType : uint8_t
{
  EnglishUS = 0,
  German,
  FrenchFrance,
  Japanese
};
  
AUDIOENGINE_EXPORT static AudioLocaleType AudioLocaleTypeFromString(const std::string& str)
{
  static const std::unordered_map<std::string, AudioLocaleType> stringToEnumMap = {
    {"EnglishUS",         AudioLocaleType::EnglishUS},
    {"German",            AudioLocaleType::German},
    {"FrenchFrance",      AudioLocaleType::FrenchFrance},
    {"Japanese",          AudioLocaleType::Japanese},
  };
  auto it = stringToEnumMap.find(str);
  if(it == stringToEnumMap.end()) {
    assert(false && "string must be a valid AudioLocaleType value");
    return AudioLocaleType::EnglishUS;
  }
  return it->second;
}

AUDIOENGINE_EXPORT static char const * AudioLocalTypeToString(const AudioLocaleType localeType)
{
  switch(localeType) {
    case AudioLocaleType::EnglishUS:
      return "EnglishUS";
    case AudioLocaleType::German:
      return "German";
    case AudioLocaleType::FrenchFrance:
      return "FrenchFrance";
    case AudioLocaleType::Japanese:
      return "Japanese";
    default: return nullptr;
  }
  return nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct AUDIOENGINE_EXPORT SetupConfig
{
  // File Details
  std::string assetFilePath;
  AudioFilePathResolverFunc pathResolver = nullptr;
  std::vector<std::string> pathToZipFiles;
  std::string writeFilePath;
  // Android Details
  void* assetManager = nullptr;
  std::string assetManagerBasePath;
  void* javaVm = nullptr;
  void* javaActivity = nullptr;
  
  // Engine Memory Details
  // Note: These are conservative settings they need to be tuned for the apps needs
  // Default values are the same as Wwise's default settings
  enum { kInvalidSampleRate = 0 };
  enum { kInvalidBufferSize = 0 };
  enum { kInvalidBufferRefillsInVoice = 0 };
  
  AudioUInt32 sampleRate = kInvalidSampleRate;
  AudioUInt32 bufferSize = kInvalidBufferSize;
  AudioUInt16 bufferRefillsInVoice = kInvalidBufferRefillsInVoice;

  AudioUInt32 defaultMemoryPoolSize       = ( 16 * 1024 * 1024 );   // 16 MB
  AudioUInt32 defaultLEMemoryPoolSize     = ( 16 * 1024 * 1024 );   // 16 MB
  AudioUInt32 streamingMangMemorySize     = ( 64 * 1024 );          // 64 KB
  AudioUInt32 ioMemorySize                = ( 2 * 1024 * 1024 );    //  2 MB
  AudioUInt32 ioMemoryGranularitySize     = ( 16 * 1024 );          // 16 KB
  AudioUInt32 defaultPoolBlockSize        = 1024;
  AudioUInt32 defaultMaxNumPools          = 15;
  AudioUInt32 defaultPlaybackLookAhead    = 1;
  bool        enableGameSyncPreparation   = false;
  bool        enableStreamCache           = false;
  bool        enableMusicEngine           = true;

  // Main output audio device shareset name
  std::string mainOutputSharesetName;

  // Thread config
  struct ThreadProperties
  {
    enum { kInvalidPriority = UINT_MAX };
    enum { kInvalidAffinityMask  = 0 };

    int         priority      = kInvalidPriority;
    AudioUInt32 affinityMask  = kInvalidAffinityMask;

    void SetAffinityMaskCpuId(AudioUInt16 cpuId)
    {
      // Acceptable Cpu Id [0, 31]
      assert( cpuId < (sizeof(affinityMask) * 8) );
      affinityMask |= 1 << cpuId;
    }
    void ClearAffinityMaskCpuId() { affinityMask = kInvalidAffinityMask; }
  };

  ThreadProperties threadBankManager;
  ThreadProperties threadLowEngine;
  ThreadProperties threadMonitor;
  ThreadProperties threadScheduler;

  // Engine Settings
  AudioLocaleType audioLocale = AudioLocaleType::EnglishUS;
  using RandomSeed_t = uint16_t;
  enum { kNoRandomSeed = 0 };
  RandomSeed_t randomSeed = kNoRandomSeed;
};
  
enum class AUDIOENGINE_EXPORT ErrorLevel : uint32_t
{
  None    = 0,
  Message = 1 << 0,
  Error   = 1 << 1,  
  All     = Message | Error
};
  
using LogCallbackFunc = std::function<void( uint32_t akErrorCode,
                                            const char* errorMessage,
                                            ErrorLevel errorLevel,
                                            AudioPlayingId playingId,
                                            AudioGameObject gameObjectId )>;

} // AudioEngine
} // Anki


#endif /* __Audio_AudioEngineConfigDefines_h */
