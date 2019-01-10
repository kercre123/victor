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
#include "util/container/ringBuffContiguousRead.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/ankiDefines.h"

#if defined(ANKI_PLATFORM_VICOS)
#define POCKETSPHINX_ENABLED 1
#else
#define POCKETSPHINX_ENABLED 0
#endif

#if POCKETSPHINX_ENABLED
#include <pocketsphinx.h>
#endif

#include <algorithm>
#include <array>
#include <locale>
#include <map>
#include <mutex>
#include <string>
#include <sstream>

namespace Anki {
namespace Vector {
  
#define LOG_CHANNEL "SpeechRecognizer"
  
namespace {
  const char* const __attribute((unused)) kWakeWord = "elemental";
  const unsigned int kBuffSize = 1024; // I tried 2048 (slower) and no buffer at all (equiv to 160, wayyyy slower). Nothing else
}
  
struct SpeechRecognizerPocketSphinx::SpeechRecognizerPocketSphinxData
{
  SpeechRecognizerPocketSphinxData();
  
  IndexType                         index = InvalidIndex;
  mutable std::recursive_mutex      recogMutex;
  bool                              disabled = false;
  std::atomic<bool>                 initDone;
  
  Util::RingBuffContiguousRead<int16_t> buff;
  
  #if POCKETSPHINX_ENABLED
  ps_decoder_t* ps = nullptr;
  cmd_ln_t* config = nullptr;
  int32 score;
  uint8 utt_started = false;
  uint8 in_speech;
  #endif
};
  
SpeechRecognizerPocketSphinx::SpeechRecognizerPocketSphinxData::SpeechRecognizerPocketSphinxData()
  : buff{ kBuffSize, kBuffSize }
{
}
  
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
  std::lock_guard<std::recursive_mutex>(_impl->recogMutex);
  _impl->index = index;
}
  
void SpeechRecognizerPocketSphinx::SetRecognizerFollowupIndex(IndexType index)
{

}

SpeechRecognizerPocketSphinx::IndexType SpeechRecognizerPocketSphinx::GetRecognizerIndex() const
{
  std::lock_guard<std::recursive_mutex>(_impl->recogMutex);
  return _impl->index;
}

bool SpeechRecognizerPocketSphinx::Init(const std::string& modelBasePath)
{
  Cleanup();
# if POCKETSPHINX_ENABLED
  
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
  
  _impl->initDone = true;
  
# endif
  
  return true;
}


void SpeechRecognizerPocketSphinx::Cleanup()
{
# if POCKETSPHINX_ENABLED
  std::lock_guard<std::recursive_mutex> lock(_impl->recogMutex);
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
  if( _impl->disabled || !_impl->initDone ) {
    // Don't process audio data in recognizer
    return;
  }
  
  unsigned int idxRemaining;
  if( _impl->buff.Capacity() > _impl->buff.Size() + audioDataLen ) {
    _impl->buff.AddData( audioData, audioDataLen );
    
    return;
  } else {
    const unsigned int numToAdd = static_cast<unsigned int>(_impl->buff.Capacity() - _impl->buff.Size());
    _impl->buff.AddData( audioData, numToAdd );
    idxRemaining = numToAdd;
  }
  
  
  using namespace std;
  clock_t begin = clock();
  
  
  
  const unsigned int buffSize = static_cast<unsigned int>(_impl->buff.GetContiguousSize());
  auto* buff __attribute((unused)) = _impl->buff.ReadData( buffSize );

# if POCKETSPHINX_ENABLED
  
  const char* hyp = nullptr;
  
  ps_process_raw( _impl->ps, buff, buffSize, false, false );
  
  _impl->in_speech = ps_get_in_speech( _impl->ps );
  
  if( _impl->in_speech && !_impl->utt_started ) {
    _impl->utt_started = true;
  }
  
  if( !_impl->in_speech && _impl->utt_started ) {
    ps_end_utt( _impl->ps );
    int32 score = 0;
    hyp = ps_get_hyp( _impl->ps, &score );
    if( hyp != nullptr ) {
      
      // Get results for callback struct
      std::string foundString{ hyp };
      AudioUtil::SpeechRecognizerCallbackInfo info {
        .result       = foundString.c_str(),
        .startTime_ms = 0,
        .endTime_ms   = 0,
        .score        = static_cast<float>(score),
      };

      LOG_INFO("SpeechRecognizerPocketSphinx.Update", "Recognizer -  %s", info.Description().c_str());
      DoCallback(info);
    }
    
    // restart the next utterance
    if( ps_start_utt( _impl->ps ) < 0 ) {
      LOG_ERROR( "SpeechRecognizerPocketSphinx.Update.StartUttFailed", "Failed to start utterance" );
    }
    _impl->utt_started = false;
  }
  
# endif
  
  _impl->buff.AdvanceCursor( buffSize );

  if( idxRemaining < audioDataLen ) {
    _impl->buff.AddData( audioData + idxRemaining, audioDataLen - idxRemaining );
  }
  
  clock_t end = clock();
  double elapsed_secs __attribute((unused)) = double(end - begin) / CLOCKS_PER_SEC;
  //PRINT_NAMED_WARNING("WHATNOW", "Duration = %f ms", elapsed_secs*1000);
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
