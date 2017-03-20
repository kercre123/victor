/**
* File: speechRecognizerTHF.cpp
*
* Author: Lee Crippen
* Created: 12/12/2016
*
* Description: SpeechRecognizer implementation for Sensory TrulyHandsFree. The cpp
* defines the impl struct that is only declared in the header, in order to encapsulate
* accessing outside headers to only be in this file.
*
* Copyright: Anki, Inc. 2016
*
*/

#include "anki/cozmo/basestation/voiceCommands/speechRecognizerTHF.h"
#include "util/logging/logging.h"

#if VOICE_RECOG_PROVIDER == VOICE_RECOG_THF
#include "trulyhandsfree.h"
#include "sensoryStaticData.h"
#endif // VOICE_RECOG_PROVIDER == VOICE_RECOG_THF

#include <iostream>


#define AUDIO_BUFFERSZ 250


namespace Anki {
namespace Cozmo {
  
struct SpeechRecognizerTHFData
{
#if VOICE_RECOG_PROVIDER == VOICE_RECOG_THF
  thf_t*      _thfSession = nullptr;
  recog_t*    _thfRecognizer = nullptr;
  searchs_t*  _thfSearch = nullptr;
  pronuns_t*  _thfPronun = nullptr;
  
  // Grab references to the static data used for recognition
  // Not bothering to make these const since the API expects them NON const for some reason
  sdata_t* const precog_data = &thf_static_recog_data;
  sdata_t* const ppronun_data = &thf_static_pronun_data;
#endif // VOICE_RECOG_PROVIDER == VOICE_RECOG_THF
  
};
  
SpeechRecognizerTHF::SpeechRecognizerTHF()
: _impl(new SpeechRecognizerTHFData())
{
  
}

SpeechRecognizerTHF::~SpeechRecognizerTHF()
{
  Cleanup();
}

SpeechRecognizerTHF::SpeechRecognizerTHF(SpeechRecognizerTHF&& other)
: SpeechRecognizer(std::move(other))
{
  SwapAllData(other);
}

SpeechRecognizerTHF& SpeechRecognizerTHF::operator=(SpeechRecognizerTHF&& other)
{
  SpeechRecognizer::operator=(std::move(other));
  SwapAllData(other);
  return *this;
}

void SpeechRecognizerTHF::SwapAllData(SpeechRecognizerTHF& other)
{
  auto temp = std::move(other._impl);
  other._impl = std::move(this->_impl);
  this->_impl = std::move(temp);
}

#if VOICE_RECOG_PROVIDER == VOICE_RECOG_THF

bool SpeechRecognizerTHF::Init(const char* const * phraseList, unsigned int numPhrases)
{
  Cleanup();
  
  /* Create SDK session */
  thf_t* createdSession = thfSessionCreate();
  if(!createdSession)
  {
    /* as of SDK 3.0.9 thfGetLastError(NULL) will return a valid string */
    const char *err=thfGetLastError(NULL) ? thfGetLastError(NULL) :
    "could not find dll or out of memory";
    std::string failMessage = std::string("ERROR thfSessionCreate ") + err;
    HandleInitFail(failMessage);
    return false;
  }
  _impl->_thfSession = createdSession;
  
  /* Create recognizer */
  recog_t* createdRecognizer = thfRecogCreateFromStatic(_impl->_thfSession, _impl->precog_data, 0, (unsigned short)(AUDIO_BUFFERSZ/1000.f*AudioUtil::kSampleRate_hz), -1, SDET);
  if(!createdRecognizer)
  {
    HandleInitFail("ERROR thfRecogCreateFromFile");
    return false;
  }
  _impl->_thfRecognizer = createdRecognizer;
  
  /* Create pronunciation */
  pronuns_t* createdPronunciation = thfPronunCreateFromStatic(_impl->_thfSession, _impl->ppronun_data, 0, 0);
  if(!createdPronunciation)
  {
    HandleInitFail("ERROR thfPronunCreateFromFile");
    return false;
  }
  _impl->_thfPronun = createdPronunciation;
  
  /* Create search */
  searchs_t* createdSearch = thfPhrasespotCreateFromList(_impl->_thfSession, _impl->_thfRecognizer, _impl->_thfPronun, (const char**)phraseList, (uint16_t)numPhrases, NULL, NULL, 0, PHRASESPOT_LISTEN_SHORT);
  if(!createdSearch)
  {
    const char *err = thfGetLastError(_impl->_thfSession);
    std::string errorMessage = std::string("ERROR thfSearchCreateFromList ") + err;
    HandleInitFail(errorMessage);
    return false;
  }
  _impl->_thfSearch = createdSearch;
  
  
  // To add some better configuration to the phrase spotting we're doing, try calling thfPhrasespotConfigSet with appropriate options.
  // In particular, look at the PS_SEQ_BUFFER option in conjunction with the call to thfRecogPrepSeq (check out the documentation), which
  // allows for switching to a secondary recognizer after a primary keyphrase has been recognized
  // file:///Users/leecrippen/dev/TrulyHandsfreeSDK/4.4.4/doc/thfPhrasespotConfigSet.html
  // file:///Users/leecrippen/dev/TrulyHandsfreeSDK/4.4.4/doc/thfRecogPrepSeq.html
  
  /* Initialize recognizer */
  if(!thfRecogInit(_impl->_thfSession, _impl->_thfRecognizer, _impl->_thfSearch, RECOG_KEEP_NONE))
  {
    HandleInitFail("ERROR thfRecogInit");
    return false;
  }
  
  return true;
}

void SpeechRecognizerTHF::HandleInitFail(const std::string& failMessage)
{
  PRINT_NAMED_ERROR("SpeechRecognizerTHF.Init.Fail", "%s", failMessage.c_str());
  Cleanup();
}

void SpeechRecognizerTHF::Cleanup()
{
  if (_impl->_thfRecognizer)
  {
    thfRecogDestroy(_impl->_thfRecognizer);
    _impl->_thfRecognizer = nullptr;
  }
  
  if (_impl->_thfSearch)
  {
    thfSearchDestroy(_impl->_thfSearch);
    _impl->_thfSearch = nullptr;
  }
  
  if (_impl->_thfPronun)
  {
    thfPronunDestroy(_impl->_thfPronun);
    _impl->_thfPronun = nullptr;
  }
  
  if (_impl->_thfSession)
  {
    thfSessionDestroy(_impl->_thfSession);
    _impl->_thfSession = nullptr;
  }
}

bool SpeechRecognizerTHF::RecogStatusIsEndCondition(uint16_t status)
{
  switch (status)
  {
    case RECOG_SILENCE:   //Timed out waiting for start of speech (end condition).
    case RECOG_DONE:      //End of utterance detected (end condition).
    case RECOG_MAXREC:    //Timed out waiting for end of utterance (end condition).
    case RECOG_IGNORE:    //Speech detector triggered but failed the minduration test. Probably not speech (end condition).
    case RECOG_NODATA:    //The input audio buffer was empty (error condition).
      return true;
    default:
      return false;
  }
}

void SpeechRecognizerTHF::Update(const AudioUtil::AudioSample * audioData, unsigned int audioDataLen)
{
  unsigned short status = RECOG_SILENCE;
  if(!thfRecogPipe(_impl->_thfSession, _impl->_thfRecognizer, audioDataLen, (short*)audioData, RECOG_ONLY, &status))
  {
    PRINT_NAMED_ERROR("SpeechRecognizerTHF.Update.thfRecogPipe.Fail", "");
    return;
  }
  
  if (RecogStatusIsEndCondition(status))
  {
    float score = 0;
    const char* foundString = NULL;
    if (!thfRecogResult(_impl->_thfSession, _impl->_thfRecognizer, &score, &foundString, NULL, NULL, NULL, NULL, NULL, NULL))
    {
      PRINT_NAMED_ERROR("SpeechRecognizerTHF.Update.thfRecogResult.Fail", "");
    }
    
    if (foundString != NULL)
    {
      DoCallback(foundString);
    }
    
    if (!thfRecogReset(_impl->_thfSession, _impl->_thfRecognizer))
    {
      PRINT_NAMED_ERROR("SpeechRecognizerTHF.Update.thfRecogReset.Fail", "");
    }
  }
}
#else // VOICE_RECOG_PROVIDER == VOICE_RECOG_THF
  
bool SpeechRecognizerTHF::Init(const char* const * phraseList, unsigned int numPhrases) { return true; }
void SpeechRecognizerTHF::HandleInitFail(const std::string& failMessage) { }
void SpeechRecognizerTHF::Cleanup() { }
bool SpeechRecognizerTHF::RecogStatusIsEndCondition(uint16_t status) { return false; }
void SpeechRecognizerTHF::Update(const AudioUtil::AudioSample * audioData, unsigned int audioDataLen) { }
  
#endif
  
} // end namespace Cozmo
} // end namespace Anki
