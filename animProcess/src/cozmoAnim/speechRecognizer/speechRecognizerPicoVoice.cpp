/**
* File: speechRecognizerPicoVoice.cpp
*
* Author: chapados
* Created: 01/10/2019
*
* Description: SpeechRecognizer implementation for PicoVoice Porcupine. 
*
* Copyright: Anki, Inc. 2019
*
*/

#include "speechRecognizerPicoVoice.h"

#include "audioUtil/speechRecognizer.h"
#include "util/logging/logging.h"
#include "util/container/ringBuffContiguousRead.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/ankiDefines.h"

#if defined(ANKI_PLATFORM_VICOS)
#define PICOVOICE_ENABLED 1
#else
#define PICOVOICE_ENABLED 0
#endif

#if PICOVOICE_ENABLED
#include <pv_porcupine.h>
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
  
struct SpeechRecognizerPicoVoice::SpeechRecognizerPicoVoiceData
{
  SpeechRecognizerPicoVoiceData();
  
  IndexType                         index = InvalidIndex;
  mutable std::recursive_mutex      recogMutex;
  bool                              disabled = false;
  std::atomic<bool>                 initDone;
  
  Util::RingBuffContiguousRead<int16_t> buff;
  
  #if PICOVOICE_ENABLED
  pv_porcupine_object_t* pv = nullptr;
  #endif

  int frameLen = 0;
  float sensitivity = 0.5f;
  bool recognized = false;
};
  
SpeechRecognizerPicoVoice::SpeechRecognizerPicoVoiceData::SpeechRecognizerPicoVoiceData()
  : buff{ kBuffSize, kBuffSize }
{
}
  
SpeechRecognizerPicoVoice::SpeechRecognizerPicoVoice()
: _impl(new SpeechRecognizerPicoVoiceData())
{
  
}

SpeechRecognizerPicoVoice::~SpeechRecognizerPicoVoice()
{
  Cleanup();
}

SpeechRecognizerPicoVoice::SpeechRecognizerPicoVoice(SpeechRecognizerPicoVoice&& other)
: SpeechRecognizer(std::move(other))
{
  SwapAllData(other);
}

SpeechRecognizerPicoVoice& SpeechRecognizerPicoVoice::operator=(SpeechRecognizerPicoVoice&& other)
{
  SpeechRecognizer::operator=(std::move(other));
  SwapAllData(other);
  return *this;
}

void SpeechRecognizerPicoVoice::SwapAllData(SpeechRecognizerPicoVoice& other)
{
  auto temp = std::move(other._impl);
  other._impl = std::move(this->_impl);
  this->_impl = std::move(temp);
}

void SpeechRecognizerPicoVoice::SetRecognizerIndex(IndexType index)
{
  std::lock_guard<std::recursive_mutex>(_impl->recogMutex);
  _impl->index = index;
}
  
void SpeechRecognizerPicoVoice::SetRecognizerFollowupIndex(IndexType index)
{

}

SpeechRecognizerPicoVoice::IndexType SpeechRecognizerPicoVoice::GetRecognizerIndex() const
{
  std::lock_guard<std::recursive_mutex>(_impl->recogMutex);
  return _impl->index;
}

bool SpeechRecognizerPicoVoice::Init(const std::string& modelBasePath)
{
  Cleanup();
# if PICOVOICE_ENABLED  
  
  const std::string modelFile = 
    Util::FileUtils::AddTrailingFileSeparator(modelBasePath) + "porcupine_tiny_params.pv";
  const std::string keywordFile =
    Util::FileUtils::AddTrailingFileSeparator(modelBasePath) + "elemental_raspberrypi_tiny.ppn";
  
  const pv_status_t status = pv_porcupine_init_softfp(modelFile.c_str(),
                                                      keywordFile.c_str(),
                                                      &_impl->sensitivity,
                                                      &_impl->pv);

  if (status != PV_STATUS_SUCCESS) {
    LOG_ERROR("SpeechRecognizerPicoVoice.Init.BadConfig",
              "Failed to create pv handle [error %d]",
              (int)status);
    return false;
  }
  
  _impl->frameLen = pv_porcupine_frame_length();
  _impl->initDone = true;
# endif
  
  return true;
}


void SpeechRecognizerPicoVoice::Cleanup()
{
# if PICOVOICE_ENABLED
  std::lock_guard<std::recursive_mutex> lock(_impl->recogMutex);
  if (_impl->pv) {
    pv_porcupine_delete(_impl->pv);
    _impl->pv = nullptr;
  }
  _impl->frameLen = 0;
  _impl->initDone = false;
# endif
}

void SpeechRecognizerPicoVoice::Update( const AudioUtil::AudioSample* audioData, unsigned int audioDataLen )
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

  int offset = 0;

# if PICOVOICE_ENABLED

  const int frameLen = _impl->frameLen;
  int remaining = buffSize;
  bool recognized = false;

  // process audio data in frameLen chunks until we recognize something
  while (!recognized && (remaining > frameLen)) {
    const int16_t* pcm = static_cast<const int16_t*>(buff) + offset;
    pv_status_t status = pv_porcupine_process(_impl->pv, pcm, &recognized);
    if (status != PV_STATUS_SUCCESS) {
      break;
    }
    offset += frameLen;
    remaining -= frameLen;
  }

  if (recognized) {
      // Get results for callback struct
      AudioUtil::SpeechRecognizerCallbackInfo info {
        .result       = kWakeWord,
        .startTime_ms = 0,
        .endTime_ms   = 0,
        .score        = 1.0f,
      };

      LOG_INFO("SpeechRecognizerPicoVoice.Update", "Recognizer -  %s", info.Description().c_str());
      DoCallback(info);
  }
  
# endif
  
  _impl->buff.AdvanceCursor( offset );

  if( idxRemaining < audioDataLen ) {
    _impl->buff.AddData( audioData + idxRemaining, audioDataLen - idxRemaining );
  }
  
  clock_t end = clock();
  double elapsed_secs __attribute((unused)) = double(end - begin) / CLOCKS_PER_SEC;
  //PRINT_NAMED_WARNING("WHATNOW", "Duration = %f ms", elapsed_secs*1000);
}
  
void SpeechRecognizerPicoVoice::StartInternal()
{
  _impl->disabled = false;
}

void SpeechRecognizerPicoVoice::StopInternal()
{
  _impl->disabled = true;
}


} // end namespace Vector
} // end namespace Anki
