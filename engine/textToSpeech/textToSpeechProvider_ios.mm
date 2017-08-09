/**
 * File: textToSpeechProvider_ios.mm
 *
 * Description: Platform-specific details of text-to-speech implementation
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#include "util/helpers/ankiDefines.h"

#if defined(ANKI_PLATFORM_IOS)

#include "engine/textToSpeech/textToSpeechProvider_ios.h"
#include "engine/textToSpeech/textToSpeechProvider_acapela.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/data/dataScope.h"
#include "engine/cozmoContext.h"

#include "util/console/consoleInterface.h"
#include "util/environment/locale.h"
#include "util/logging/logging.h"

#include <thread>

// Log options
#define LOG_CHANNEL    "TextToSpeech"
#define LOG_ERROR      PRINT_NAMED_ERROR
#define LOG_WARNING    PRINT_NAMED_WARNING
#define LOG_INFO(...)  PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)
#define LOG_DEBUG(...) PRINT_CH_DEBUG(LOG_CHANNEL, ##__VA_ARGS__)

#define LOG_TRACE_ENABLED 0

#if LOG_TRACE_ENABLED
#define LOG_TRACE LOG_DEBUG
#else
#define LOG_TRACE(...) { }
#endif


// Acapela configuration
#import "AcapelaSpeech.h"
#import "acattsioslicense.h"
#import "acattsioslicense.m"

#if REMOTE_CONSOLE_ENABLED

// Debug sliders
#define CONSOLE_GROUP "TextToSpeech.VoiceParameters"

namespace {
  CONSOLE_VAR_RANGED(s32, kVoiceSpeed, CONSOLE_GROUP, 100, 30, 300);
  CONSOLE_VAR_RANGED(s32, kVoiceShaping, CONSOLE_GROUP, 100, 70, 140);
  CONSOLE_VAR_RANGED(s32, kVoicePitch, CONSOLE_GROUP, 100, 70, 160);
}

#endif

// Max samples per read
#define TTS_BLOCKSIZE (16*1024)

// Fixed parameters
#define TTS_LEADINGSILENCE_MS  50 // Minimum allowed by Acapela TTS SDK
#define TTS_TRAILINGSILENCE_MS 50 // Minimum allowed by Acapela TTS SDK

//
// This interface is used to provide a pure-C interface on top of Acapela's Objective-C SDK.
//
@interface AcapelaProviderImpl : NSObject
// Class methods to provide singleton instance
+ (id)shared;

// Instance methods to implement SDK operations
- (Anki::Result)loadVoice:(const char*)voice;
- (Anki::Result)generateAudioFile:(const char*)str
    path:(const char*)path
    speed:(int)speed
  shaping:(int)shaping
    pitch:(int)pitch;

// Instance methods to implement AcapelaSpeechDelegate callback interface.
// These delegate methods appear to be required for correct operation of the SDK,
// even though they do nothing but log diagnostics.
- (void)speechSynthesizer:(AcapelaSpeech *)sender didFinishSpeaking:(BOOL)finishedSpeaking;
- (void)speechSynthesizer:(AcapelaSpeech *)sender didFinishSpeaking:(BOOL)finishedSpeaking textIndex:(int)index;
- (void)speechSynthesizer:(AcapelaSpeech *)sender willSpeakWord:(NSRange)characterRange ofString:(NSString *)string;
- (void)speechSynthesizer:(AcapelaSpeech *)sender willSpeakViseme:(short)visemeCode;
- (void)speechSynthesizer:(AcapelaSpeech *)sender didEncounterSyncMessage:(NSString *)errorMessage;

@end

@implementation AcapelaProviderImpl
{
  // Instance members
  AcapelaSpeech * _acaTTS;
}

+ (id)shared
{
  // Dispatch block is run at most once to ensure unique instance
  static dispatch_once_t once;
  static id instance;
  dispatch_once(&once, ^{
      instance = [[self alloc] init];
  });
  return instance;
}

- (Anki::Result)loadVoice:(const char*)voice
{
  @autoreleasepool
  {
  
    // Scan for new voices
    [AcapelaSpeech refreshVoiceList];

    // Initialize SDK object
    _acaTTS = [[AcapelaSpeech alloc] init];

    // Initialize Acapela license info. Note Acapela SDK provides license info as unobscured string,
    // but they seem to be OK with that.
    NSString* nsLicense = [acattsioslicense license];
    NSInteger nsUserid = [acattsioslicense userid];
    NSInteger nsPassword = [acattsioslicense password];
    NSString* nsMode = @"";
  
    //
    // Which voice do we load? Allocate a temp string for objective C interface.
    // Xcode leak checker reports that this object is leaked. It appears that AcapelaSpeech interface
    // is leaking a reference.
    //
    NSString* nsVoice = [NSString stringWithCString:voice encoding:NSASCIIStringEncoding];

    int result = [_acaTTS loadVoice:nsVoice
                            license:nsLicense
                             userid:nsUserid
                           password:nsPassword
                               mode:nsMode];
    if (0 != result) {
      LOG_ERROR("TextToSpeechProvider.LoadVoice", "Unable to load voice (result %d)", result);
      _acaTTS = nullptr;
      return Anki::RESULT_FAIL;
    }
  
    [_acaTTS setTTSSetting:@"LEADINGSILENCE" settingvalue:TTS_LEADINGSILENCE_MS];
    [_acaTTS setTTSSetting:@"TRAILINGSILENCE" settingvalue:TTS_TRAILINGSILENCE_MS];
    
    //
    // Set SDK delegate for diagnostics.  At present, delegate methods don't do anything else,
    // but TTS SDK crashes at random if delegate is not provided. :(
    //
    [_acaTTS setDelegate:self];

    return Anki::RESULT_OK;
  }
}

- (Anki::Result)generateAudioFile:(const char *)str
   path:(const char*)path
  speed:(int)speed
shaping:(int)shaping
  pitch:(int)pitch
{
  @autoreleasepool
  {
    DEV_ASSERT(nullptr != _acaTTS, "TextToSpeechProviderImpl.NoSpeechProvider");
    DEV_ASSERT(nullptr != str, "TextToSpeechProviderImpl.NoString");
    DEV_ASSERT(nullptr != path, "TextToSpeechProviderImpl.NoPath");
  
    LOG_DEBUG("TextToSpeechProviderImpl.GenerateAudioFile", "str=%s path=%s speed=%d shaping=%d pitch=%d",
              Anki::Util::HidePersonallyIdentifiableInfo(str), path,
              speed, shaping, pitch);
  
    NSString* nsstr = [NSString stringWithUTF8String:str];
    NSURL* nsurl = [NSURL fileURLWithFileSystemRepresentation:path isDirectory:NO relativeToURL:nil];
    NSString* nstype = @"pcm";
  
    [_acaTTS setRate:speed];
    [_acaTTS setVoiceShaping:shaping];
    [_acaTTS setTTSSetting:@"SEL_PITCH" settingvalue:pitch];

  
    BOOL ok = [_acaTTS generateAudioFile:nsstr toURL:nsurl type:nstype sync:YES];
    if (!ok) {
      // This method returns 0 (false) even when a file is generated.  If Acapela fixes this,
      // we should check for errors here.
      // LOG_ERROR("TextToSpeechProviderImpl.GenerateAudioFile", "Unable to generate audio file");
      // return Anki::RESULT_FAIL;
    }
  
    return Anki::RESULT_OK;
  }
}

- (void)speechSynthesizer:(AcapelaSpeech *)sender didFinishSpeaking:(BOOL)finishedSpeaking
{
  LOG_TRACE("TextToSpeechProviderImpl.DidFinishSpeaking", "%d", finishedSpeaking);
}

- (void)speechSynthesizer:(AcapelaSpeech *)sender didFinishSpeaking:(BOOL)finishedSpeaking textIndex:(int)index;
{
  LOG_TRACE("TextToSpeechProviderImpl.DidFinishSpeaking", "%d at %d", finishedSpeaking, index);
}

- (void)speechSynthesizer:(AcapelaSpeech *)sender willSpeakWord:(NSRange)characterRange ofString:(NSString *)string;
{
  LOG_TRACE("TextToSpeechProviderImpl.WillSpeakWord",
            "%lu,%lu of %s",
            (unsigned long)characterRange.location,
            (unsigned long)characterRange.length,
            [string cString]);
}

- (void)speechSynthesizer:(AcapelaSpeech *)sender willSpeakViseme:(short)visemeCode;
{
  LOG_TRACE("TextToSpeechProviderImpl.WillSpeakViseme", "%d", visemeCode);
}

- (void)speechSynthesizer:(AcapelaSpeech *)sender didEncounterSyncMessage:(NSString *)errorMessage;
{
  LOG_TRACE("TextToSpeechProviderImpl.DidEncounterSyncMessage", "%s", [errorMessage cString]);
}

@end

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {

TextToSpeechProviderImpl::TextToSpeechProviderImpl(const CozmoContext* context, const Json::Value& tts_platform_config)
{
  using Locale = Anki::Util::Locale;
  using DataPlatform = Anki::Util::Data::DataPlatform;
  using Scope = Anki::Util::Data::Scope;
  
  @autoreleasepool
  {
    // Check for valid data platform before we do any work
    const DataPlatform * dataPlatform = context->GetDataPlatform();
    if (nullptr == dataPlatform) {
      // This may happen during unit tests
      LOG_WARNING("TextToSpeechProvider.Initialize.NoDataPlatform", "Unable to initialize TTS provider");
      return;
    }
    
    // Check for valid locale before we do any work
    const Locale * locale = context->GetLocale();
    if (nullptr == locale) {
      // This may happen during unit tests
      LOG_WARNING("TextToSpeechProvider.Initialize.NoLocale", "Unable to initialize TTS provider");
      return;
    }
    
    // Set up default parameters
    _tts_language = locale->GetLanguageString();
    _tts_voice = "enu_ryan_22k_co.fl";
    _tts_speed = 100;
    _tts_shaping = 100;
    _tts_pitch = 100;

    //
    // Allow language configuration to override defaults.  Note pitch is not
    // supported on all platforms, so it not supported as a config parameter
    // at this time.
    //
    Json::Value tts_language_config = tts_platform_config[_tts_language.c_str()];
    
    JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kVoiceKey, _tts_voice);
    JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kSpeedKey, _tts_speed);
    JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kShapingKey, _tts_shaping);
    //JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kPitchKey, _tts_pitch);
    
    LOG_INFO("TextToSpeechProvider.Initialize",
             "language=%s voice=%s speed=%d shaping=%d pitch=%d",
             _tts_language.c_str(), _tts_voice.c_str(), _tts_speed,
             _tts_shaping, _tts_pitch);

#if REMOTE_CONSOLE_ENABLED
    // Initialize debug sliders to match voice params
    kVoiceSpeed = _tts_speed;
    kVoiceShaping = _tts_shaping;
    kVoicePitch = _tts_pitch;
#endif

    // Initialize SDK and load voice
    AcapelaProviderImpl * impl = [AcapelaProviderImpl shared];
  
    Result result = [impl loadVoice:_tts_voice.c_str()];

    if (RESULT_OK != result) {
      LOG_ERROR("TextToSpeechProvider.LoadVoice",
                "Unable to load voice=%s (result %d)",
                _tts_voice.c_str(), result);
    }
  
    // Generate a path for temporary data
    _path = dataPlatform->pathToResource(Scope::Cache, "cozmo.pcm");
  }
}
  
TextToSpeechProviderImpl::~TextToSpeechProviderImpl()
{
  // Release temp file
  unlink(_path.c_str());
}

Result TextToSpeechProviderImpl::CreateAudioData(const std::string& text,
                                                 float durationScalar,
                                                 TextToSpeechProviderData& data)
{
  @autoreleasepool
  {
    const auto before = std::chrono::steady_clock::now();

    // Get reference to shared instance
    AcapelaProviderImpl * impl = [AcapelaProviderImpl shared];

#if REMOTE_CONSOLE_ENABLED
    // Update voice params to match debug sliders
    _tts_speed = kVoiceSpeed;
    _tts_shaping = kVoiceShaping;
    _tts_pitch = kVoicePitch;
#endif

    // Adjust base speed by scalar, then clamp to allowed range
    const float speed = AcapelaTTS::GetSpeechRate(_tts_speed, durationScalar);
  
    Result result = [impl generateAudioFile:text.c_str()
                                       path:_path.c_str()
                                      speed:speed
                                    shaping:_tts_shaping
                                      pitch:_tts_pitch];
    if (RESULT_OK != result) {
      LOG_ERROR("TextToSpeechProvider.CreateAudioData.GenerateAudioFile",
                "Unable to generate audio file (error %d)",
                result);
      return result;
    }
  
    // Initialize results
    data.Init(AcapelaTTS::GetSampleRate(), AcapelaTTS::GetNumChannels());
  
    AudioUtil::AudioChunk& chunk = data.GetChunk();
    int fd = open(_path.c_str(), O_RDONLY);
    if (fd < 0) {
      LOG_ERROR("TextToSpeechProvider.CreateAudioData.OpenFailed",
                "Unable to open %s (errno %d)",
                _path.c_str(), errno);
      return RESULT_FAIL;
    }
    
    while (1) {
      short buf[TTS_BLOCKSIZE];
      const ssize_t nbytes = read(fd, buf, sizeof(buf));
      if (nbytes < 0) {
        LOG_ERROR("TextToSpeechProvider.CreateAudioData.ReadError",
                  "Unable to read %s (errno %d)",
                  _path.c_str(), errno);
        break;
      }
      if (nbytes == 0) {
        LOG_DEBUG("TextToSpeechProvider.CreateAudioData.ReadEOF", "EOF");
        break;
      }
      const ssize_t nshorts = (nbytes/sizeof(short));
      LOG_TRACE("TextToSpeechProvider.CreateAudioData.Read",
                "%zd bytes, %zd shorts",
                nbytes, nshorts);
      for (ssize_t n = 0; n < nshorts; ++n) {
        chunk.push_back(buf[n]);
      }
    }
    
    close(fd);
  
    const auto after = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(after - before);
    
    LOG_DEBUG("TextToSpeechProvider.CreateAudioData.Result",
              "Returning %zu samples after %lld ms",
              data.GetNumSamples(), elapsed.count());
  
    return RESULT_OK;
  }
}

} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

#endif // ANKI_PLATFORM_IOS


