/**
* File: speechRecognizerPocketSphinx.cpp
*
* Author: Lee Crippen
* Created: 12/12/2016
* Updated: 10/29/2017 Simple version rename to differentiate from legacy implementation.
*
* Description: SpeechRecognizer implementation for Sensory TrulyHandsFree. The cpp
* defines the impl struct that is only declared in the header, in order to encapsulate
* accessing outside headers to only be in this file.
*
* Copyright: Anki, Inc. 2016
*
*/

#include "speechRecognizerPocketSphinx.h"

#include "audioUtil/speechRecognizer.h"
#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"

#if defined(VICOS)
#define PRYON_ENABLED 1
#else
#define PRYON_ENABLED 0
#endif

#if PRYON_ENABLED
#include <pocketsphinx.h>
#endif

#include <algorithm>
#include <array>
#include <locale>
#include <map>
#include <mutex>
#include <string>
#include <sstream>
#include <thread>

namespace Anki {
namespace Vector {
  
#define LOG_CHANNEL "SpeechRecognizer"
  
namespace {
  const char* const kWakeWord __attribute((unused)) = "elemental";
  // I tried 2048 (slower) and no buffer at all (equiv to 160, wayyyy slower), then 1024, which worked well.
  // then I switched to double buffering, where logic is easier if its a multiple of 160 (audio input size)
  const unsigned int kBuffSize = 7*160;
}
  
struct SpeechRecognizerPocketSphinx::SpeechRecognizerPocketSphinxData
{
  IndexType                         index = InvalidIndex;
  bool                              disabled = false;
  std::atomic<bool>                 initDone;
  
  std::thread                       detectionThread;
  mutable std::mutex                mutex;
  std::condition_variable           cv;
  bool                              detectionRunning;
  
  int16_t buffA[kBuffSize];
  int16_t buffB[kBuffSize];
  bool writingBuffA = true;
  unsigned int writeIdx = 0;
  int16_t* readBuff = nullptr; // also serves as the flag for the recognizer thread that there's a new readBuff
  std::unique_ptr<AudioUtil::SpeechRecognizerCallbackInfo> callbackInfo;
  
  // check if there's a callback.
  // fully write to buffX. set readBuff to buffX and notify. flip writingBuffA.
  
  
  #if PRYON_ENABLED
  ps_decoder_t* ps = nullptr;
  cmd_ln_t* config = nullptr;
  int32 score;
  uint8 utt_started = false;
  uint8 in_speech;
  #endif
};
  
SpeechRecognizerPocketSphinx::SpeechRecognizerPocketSphinx()
: _impl(new SpeechRecognizerPocketSphinxData())
{
  
}

SpeechRecognizerPocketSphinx::~SpeechRecognizerPocketSphinx()
{
  Cleanup();
}

SpeechRecognizerPocketSphinx::SpeechRecognizerPocketSphinx(SpeechRecognizerPocketSphinx&& other)
: SpeechRecognizer(std::move(other))
{
  SwapAllData(other);
}

SpeechRecognizerPocketSphinx& SpeechRecognizerPocketSphinx::operator=(SpeechRecognizerPocketSphinx&& other)
{
  SpeechRecognizer::operator=(std::move(other));
  SwapAllData(other);
  return *this;
}

void SpeechRecognizerPocketSphinx::SwapAllData(SpeechRecognizerPocketSphinx& other)
{
  auto temp = std::move(other._impl);
  other._impl = std::move(this->_impl);
  this->_impl = std::move(temp);
}

void SpeechRecognizerPocketSphinx::SetRecognizerIndex(IndexType index)
{
  _impl->index = index;
}
  
void SpeechRecognizerPocketSphinx::SetRecognizerFollowupIndex(IndexType index)
{

}

SpeechRecognizerPocketSphinx::IndexType SpeechRecognizerPocketSphinx::GetRecognizerIndex() const
{
  return _impl->index;
}

bool SpeechRecognizerPocketSphinx::Init(const std::string& modelBasePath)
{
  Cleanup();
# if PRYON_ENABLED
  
  // todo: thread. For now, since this only runs during animProcess' init(), we don't have to worry about messages piling up
  
  
  const std::string param_hmm   = Util::FileUtils::AddTrailingFileSeparator(modelBasePath) + "en-us/en-us";
  const std::string param_lm    = Util::FileUtils::AddTrailingFileSeparator(modelBasePath) + "en-us/en-us.lm.bin";
  const std::string param_dict  = Util::FileUtils::AddTrailingFileSeparator(modelBasePath) + "en-us/cmudict-en-us.dict";
  
  _impl->config
    = cmd_ln_init( NULL, ps_args(), TRUE,
                   "-hmm",  param_hmm.c_str(),
                   "-lm",   param_lm.c_str(),
                   "-dict", param_dict.c_str(),
                   // params optimized for speed, from https://cmusphinx.github.io/wiki/pocketsphinxhandhelds/
                   "-ds", "2", "-topn", "2", "-maxwpf", "5", "-maxhmmpf", "3000", "-pl_window", "10",
                   // params optimized for speed, from random internet article http://pe.org.pl/articles/2014/4/42.pdf
                   // (might not be trustworthy but seems to work)
                   "-beam", "1e-40 ", "-pbeam", "1e-40", "-wbeam", "1e-05",
                   NULL );
  
  if( _impl->config == nullptr ) {
    LOG_ERROR("SpeechRecognizerPocketSphinx.Init.BadConfig", "Failed to create config object");
    return false;
  }
  
  _impl->ps = ps_init( _impl->config );
  if( _impl->ps == nullptr ) {
    LOG_ERROR("SpeechRecognizerPocketSphinx.Init.BadPS", "Failed to create recognizer");
    return false;
  }
  
  ps_set_keyphrase( _impl->ps, "keyphrase_search", kWakeWord);
  ps_set_search( _impl->ps, "keyphrase_search" );
  ps_start_utt( _impl->ps );
  
  _impl->detectionRunning = true;
  _impl->detectionThread = std::thread( &SpeechRecognizerPocketSphinx::RecognizerThread, this );
  
  _impl->initDone = true;
  
# endif
  
  return true;
}


void SpeechRecognizerPocketSphinx::Cleanup()
{
# if PRYON_ENABLED
  
  if( _impl->detectionThread.joinable() ) {
    _impl->detectionRunning = false;
    _impl->detectionThread.join();
  }
  
  if( _impl->ps ) {
    ps_end_utt( _impl->ps );
    ps_free( _impl->ps );
    _impl->ps = nullptr;
  }
  if( _impl->config ) {
    cmd_ln_free_r( _impl->config );
    _impl->config = nullptr;
  }
  _impl->initDone = false;
# endif
}

void SpeechRecognizerPocketSphinx::Update( const AudioUtil::AudioSample* audioData, unsigned int audioDataLen )
{
  // needs mutex
  auto checkCallbackUnsafe = [&]() {
    //PRINT_NAMED_WARNING("WHATNOW", "checking for callback");
    if( _impl->callbackInfo ) {
      DoCallback(*_impl->callbackInfo.get());
      LOG_INFO("SpeechRecognizerPocketSphinx.Update", "Recognizer -  %s", _impl->callbackInfo->Description().c_str());
      _impl->callbackInfo.reset();
    }
  };
  
  if( _impl->disabled || !_impl->initDone ) {
    // Don't process audio data in recognizer
    return;
  }
  
  auto* writeBuff = _impl->writingBuffA ? _impl->buffA : _impl->buffB;
  if( !ANKI_VERIFY( _impl->writeIdx + audioDataLen <= kBuffSize, "", "" ) ) {
    return;
  }
  
  {
    // check if there's a callback
    std::unique_lock<std::mutex> lk(_impl->mutex, std::try_to_lock);
    if( lk.owns_lock() ) {
      checkCallbackUnsafe();
    }
  }
  
  for( int i=0; i<audioDataLen; ++i ) {
    writeBuff[_impl->writeIdx++] = audioData[i];
  }
  //PRINT_NAMED_WARNING("WHATNOW", "_impl->writeIdx=%d", _impl->writeIdx);
  
  static_assert( kBuffSize % 160 == 0, "");
  if( _impl->writeIdx == kBuffSize )
  {
    {
      //PRINT_NAMED_WARNING("WHATNOW", "trying to get lock to swap buffers");
      // get a lock on the mutex shared with the read op
      std::lock_guard<std::mutex> lk{ _impl->mutex };
      //PRINT_NAMED_WARNING("WHATNOW", "swapping buffers");
      ANKI_VERIFY( _impl->readBuff == nullptr, "", "" );
      
      checkCallbackUnsafe();
      
      _impl->readBuff = writeBuff;
      _impl->writingBuffA = !_impl->writingBuffA;
      _impl->writeIdx = 0;
    }
    //PRINT_NAMED_WARNING("WHATNOW", "buffers avaialble");
    _impl->cv.notify_one();
  }
  
  
}
  
void SpeechRecognizerPocketSphinx::RecognizerThread()
{
#if defined(ANKI_PLATFORM_VICOS)
  // Setup the thread's affinity mask
  // We don't want to run this on the same cores as mic processor
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(0, &cpu_set);
  CPU_SET(3, &cpu_set);
  int error = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
  if (error != 0) {
    LOG_ERROR("AlexaMediaPlayer.play", "SetAffinityMaskError %d", error);
  }
#endif
  
  while( _impl->detectionRunning ) {
    
    // wait for a new readBuff
    std::unique_lock<std::mutex> lk{ _impl->mutex };
    _impl->cv.wait( lk, [this]{ return (_impl->readBuff != nullptr); } );
    
    //PRINT_NAMED_WARNING("WHATNOW", "processing readBuff");
    // we own readBuff now
    
    using namespace std;
    clock_t begin = clock();
    
#   if PRYON_ENABLED
    
    
    
    const char* hyp = nullptr;
    
    ps_process_raw( _impl->ps, _impl->readBuff, kBuffSize, false, false );
    
    _impl->in_speech = ps_get_in_speech( _impl->ps );
    
    if( _impl->in_speech && !_impl->utt_started ) {
      _impl->utt_started = true;
    }
    
    if( !_impl->in_speech && _impl->utt_started ) {
      ps_end_utt( _impl->ps );
      int32 score = 0;
      hyp = ps_get_hyp( _impl->ps, &score );
      if( hyp != nullptr ) {
        PRINT_NAMED_WARNING("SpeechRecognizerPocketSphinx.RecogThread.Detected", "Detected %s", hyp);
        
        // Get results for callback struct
        std::string foundString{ hyp };
        _impl->callbackInfo = std::make_unique<AudioUtil::SpeechRecognizerCallbackInfo>();
        _impl->callbackInfo->result       = foundString.c_str();
        _impl->callbackInfo->startTime_ms = 0;
        _impl->callbackInfo->endTime_ms   = 0;
        _impl->callbackInfo->score        = static_cast<float>(score);
      }
      
      // restart the next utterance
      if( ps_start_utt( _impl->ps ) < 0 ) {
        LOG_ERROR( "SpeechRecognizerPocketSphinx.Update.StartUttFailed", "Failed to start utterance" );
      }
      _impl->utt_started = false;
    }
    
#   endif
    
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    PRINT_NAMED_WARNING("WHATNOW", "Duration = %f ms", elapsed_secs*1000);
    
    // done with the readbuff
    _impl->readBuff = nullptr;
    //PRINT_NAMED_WARNING("WHATNOW", "done processing readBuff");
  }
}
  
void SpeechRecognizerPocketSphinx::StartInternal()
{
  _impl->disabled = false;
}

void SpeechRecognizerPocketSphinx::StopInternal()
{
  _impl->disabled = true;
}


} // end namespace Vector
} // end namespace Anki
