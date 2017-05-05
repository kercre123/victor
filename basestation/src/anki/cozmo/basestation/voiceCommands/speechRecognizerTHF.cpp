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
#include "anki/cozmo/basestation/voiceCommands/speechRecognizerTHFTypes.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"

#if VOICE_RECOG_PROVIDER == VOICE_RECOG_THF
#include "trulyhandsfree.h"
#include "sensoryStaticData.h"
#endif // VOICE_RECOG_PROVIDER == VOICE_RECOG_THF

#include <array>
#include <iostream>
#include <map>

namespace Anki {
namespace Cozmo {
  
// Anonymous namespace to use with debug console vars for manipulating input
namespace {
  std::string sPhraseForceHeard = "";
}

CONSOLE_VAR(bool, kIgnoreMicInput, "VoiceCommand", false);

void SpeechRecognizerTHF::SetForceHeardPhrase(const char* phrase)
{
  sPhraseForceHeard = (phrase == nullptr) ? "" : phrase;
}

// Local definition of data used internally for more strict encapsulation
struct SpeechRecognizerTHF::SpeechRecognizerTHFData
{
#if VOICE_RECOG_PROVIDER == VOICE_RECOG_THF
  thf_t*      _thfSession = nullptr;
  pronuns_t*  _thfPronun = nullptr;
  
  // Grab references to the static data used for recognition
  // Not bothering to make these const since the API expects them NON const for some reason
  sdata_t* const precog_data = &thf_static_recog_data;
  sdata_t* const ppronun_data = &thf_static_pronun_data;
#endif // VOICE_RECOG_PROVIDER == VOICE_RECOG_THF
  
  IndexType                          _thfCurrentRecog = InvalidIndex;
  IndexType                          _thfFollowupRecog = InvalidIndex;
  std::map<IndexType, RecogDataSP>   _thfAllRecogs;
  
  const RecogDataSP RetrieveDataForIndex(IndexType index) const;
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

const RecogDataSP SpeechRecognizerTHF::SpeechRecognizerTHFData::RetrieveDataForIndex(IndexType index) const
{
  if (index == InvalidIndex)
  {
    return RecogDataSP();
  }
  
  // We can only use recognizers that actually exist
  auto indexIter = _thfAllRecogs.find(index);
  if (indexIter == _thfAllRecogs.end())
  {
    return RecogDataSP();
  }
  
  // Intentionally make a local copy of the shared ptr with the current recog data
  return indexIter->second;
}

void SpeechRecognizerTHF::SetRecognizerIndex(IndexType index)
{
  _impl->_thfCurrentRecog = index;
}
  
void SpeechRecognizerTHF::SetRecognizerFollowupIndex(IndexType index)
{
  _impl->_thfFollowupRecog = index;
}

SpeechRecognizerTHF::IndexType SpeechRecognizerTHF::GetRecognizerIndex() const
{
  return _impl->_thfCurrentRecog;
}

void SpeechRecognizerTHF::RemoveRecognitionData(IndexType index)
{
  auto indexIter = _impl->_thfAllRecogs.find(index);
  if (indexIter != _impl->_thfAllRecogs.end())
  {
    _impl->_thfAllRecogs.erase(indexIter);
  }
}

#if VOICE_RECOG_PROVIDER == VOICE_RECOG_THF

bool SpeechRecognizerTHF::Init()
{
  Cleanup();
  
  /* Create SDK session */
  thf_t* createdSession = thfSessionCreate();
  if(!createdSession)
  {
    /* as of SDK 3.0.9 thfGetLastError(NULL) will return a valid string */
    const char *err=thfGetLastError(NULL) ? thfGetLastError(NULL) : "could not find dll or out of memory";
    std::string failMessage = std::string("ERROR thfSessionCreate ") + err;
    HandleInitFail(failMessage);
    return false;
  }
  _impl->_thfSession = createdSession;
  
  /* Create pronunciation */
  pronuns_t* createdPronunciation = thfPronunCreateFromStatic(_impl->_thfSession, _impl->ppronun_data, 0, 0);
  if(!createdPronunciation)
  {
    HandleInitFail(std::string("ERROR thfPronunCreateFromFile ") + thfGetLastError(_impl->_thfSession));
    return false;
  }
  _impl->_thfPronun = createdPronunciation;
  
  return true;
}

void SpeechRecognizerTHF::HandleInitFail(const std::string& failMessage)
{
  PRINT_NAMED_ERROR("SpeechRecognizerTHF.Init.Fail", "%s", failMessage.c_str());
  Cleanup();
}

bool SpeechRecognizerTHF::AddRecognitionDataAutoGen(IndexType index, const char* const * phraseList, unsigned int numPhrases)
{
  recog_t* createdRecognizer = nullptr;
  searchs_t* createdSearch = nullptr;
  
  auto cleanupAfterFailure = [&] (const std::string& failMessage)
  {
    PRINT_NAMED_ERROR("SpeechRecognizerTHF.AddRecognitionDataAutoGen.Fail", "%s %s", failMessage.c_str(), thfGetLastError(_impl->_thfSession));
    RecogData::DestroyData(createdRecognizer, createdSearch);
  };
  
  if (InvalidIndex == index)
  {
    cleanupAfterFailure(std::string("Specified index matches InvalidIndex and cannot be used: ") + std::to_string(index));
    return false;
  }
  
  // Check whether this spot is already taken
  auto indexIter = _impl->_thfAllRecogs.find(index);
  if (indexIter != _impl->_thfAllRecogs.end())
  {
    cleanupAfterFailure(std::string("Recognizer already added at index ") + std::to_string(index));
    return false;
  }
  
  // The code examples had a buffer size as double the standard chunk size, so we'll do the same
  auto bufferSizeInSamples = Util::numeric_cast<unsigned short>(AudioUtil::kSamplesPerChunk * 2);
  
  /* Create recognizer */
  createdRecognizer = thfRecogCreateFromStatic(_impl->_thfSession, _impl->precog_data, 0, bufferSizeInSamples, -1, SDET);
  if(!createdRecognizer)
  {
    cleanupAfterFailure("ERROR thfRecogCreateFromStatic");
    return false;
  }
  
  /* Create search */
  // Using const_cast because the THF API apparently wants to be able to modify strings?
  auto phraseListAPIType = const_cast<const char**>(phraseList);
  createdSearch = thfPhrasespotCreateFromList(_impl->_thfSession, createdRecognizer, _impl->_thfPronun,
                                              phraseListAPIType, Util::numeric_cast<uint16_t>(numPhrases),
                                              NULL, NULL, 0, PHRASESPOT_LISTEN_SHORT);
  if(!createdSearch)
  {
    const char *err = thfGetLastError(_impl->_thfSession);
    std::string errorMessage = std::string("ERROR thfPhrasespotCreateFromList ") + err;
    cleanupAfterFailure(errorMessage);
    return false;
  }
  
  /* Initialize recognizer */
  if(!thfRecogInit(_impl->_thfSession, createdRecognizer, createdSearch, RECOG_KEEP_NONE))
  {
    cleanupAfterFailure("ERROR thfRecogInit");
    return false;
  }
  
  // Everything should be happily added, so store off this recognizer
  constexpr bool isPhraseSpotted = true;
  constexpr bool allowsFollowupRecog = false;
  _impl->_thfAllRecogs[index] = MakeRecogDataSP(createdRecognizer, createdSearch, isPhraseSpotted, allowsFollowupRecog);
  
  return true;
}

bool SpeechRecognizerTHF::AddRecognitionDataFromFile(IndexType index,
                                                     const std::string& nnFilePath, const std::string& searchFilePath,
                                                     bool isPhraseSpotted, bool allowsFollowupRecog)
{
  recog_t* createdRecognizer = nullptr;
  searchs_t* createdSearch = nullptr;
  
  auto cleanupAfterFailure = [&] (const std::string& failMessage)
  {
    PRINT_NAMED_ERROR("SpeechRecognizerTHF.AddRecognitionDataFromFile.Fail", "%s %s", failMessage.c_str(), thfGetLastError(_impl->_thfSession));
    RecogData::DestroyData(createdRecognizer, createdSearch);
  };
  
  if (InvalidIndex == index)
  {
    cleanupAfterFailure(std::string("Specified index matches InvalidIndex and cannot be used: ") + std::to_string(index));
    return false;
  }
  
  // First check whether this spot is already taken
  auto indexIter = _impl->_thfAllRecogs.find(index);
  if (indexIter != _impl->_thfAllRecogs.end())
  {
    cleanupAfterFailure(std::string("Recognizer already added at index ") + std::to_string(index));
    return false;
  }
  
  // The code examples had a buffer size as double the standard chunk size, so we'll do the same
  auto bufferSizeInSamples = Util::numeric_cast<unsigned short>(AudioUtil::kSamplesPerChunk * 2);
  
  /* Create recognizer */
  auto doSpeechDetect = isPhraseSpotted ? NO_SDET : SDET;
  createdRecognizer = thfRecogCreateFromFile(_impl->_thfSession, nnFilePath.c_str(), bufferSizeInSamples, -1, doSpeechDetect);
  if(!createdRecognizer)
  {
    cleanupAfterFailure("ERROR thfRecogCreateFromFile");
    return false;
  }
  
  /* Create search */
  constexpr unsigned short numBestResultsToReturn = 1;
  createdSearch = thfSearchCreateFromFile(_impl->_thfSession, createdRecognizer, searchFilePath.c_str(), numBestResultsToReturn);
  if(!createdSearch)
  {
    const char *err = thfGetLastError(_impl->_thfSession);
    std::string errorMessage = std::string("ERROR thfSearchCreateFromFile ") + err;
    cleanupAfterFailure(errorMessage);
    return false;
  }
  
  /* Initialize recognizer */
  if(!thfRecogInit(_impl->_thfSession, createdRecognizer, createdSearch, RECOG_KEEP_NONE))
  {
    cleanupAfterFailure("ERROR thfRecogInit");
    return false;
  }
  
  if (allowsFollowupRecog)
  {
    if (!isPhraseSpotted)
    {
      cleanupAfterFailure("Tried to set up phrase following with non-phrasespotting recognizers, which is not allowed.");
      return false;
    }
    
    constexpr float overlapTime_ms = 1000.f;
    if (!thfPhrasespotConfigSet(_impl->_thfSession, createdRecognizer, createdSearch, PS_SEQ_BUFFER, overlapTime_ms))
    {
      cleanupAfterFailure("ERROR thfPhrasespotConfigSet PS_SEQ_BUFFER");
      return false;
    }
  }
  
  if (isPhraseSpotted)
  {
    if (!thfPhrasespotConfigSet(_impl->_thfSession, createdRecognizer, createdSearch, PS_DELAY, 90))
    {
      cleanupAfterFailure("ERROR thfPhrasespotConfigSet PS_DELAY");
      return false;
    }
  }
  
  // Everything should be happily added, so store off this recognizer
  _impl->_thfAllRecogs[index] = MakeRecogDataSP(createdRecognizer, createdSearch, isPhraseSpotted, allowsFollowupRecog);
  
  return true;
}

void SpeechRecognizerTHF::Cleanup()
{
  _impl->_thfAllRecogs.clear();
  
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
  // If we're ignoring mic input, swap out the real mic data with silence
  if (kIgnoreMicInput)
  {
    static const std::array<AudioUtil::AudioSample, AudioUtil::kSamplesPerChunk> kSilenceSample{};
    audioData = kSilenceSample.data();
    audioDataLen = Util::numeric_cast<unsigned int>(kSilenceSample.size());
  }
  
  // Intentionally make a local copy of the shared ptr with the current recog data
  const RecogDataSP currentRecogSP = _impl->RetrieveDataForIndex(_impl->_thfCurrentRecog);
  if (!currentRecogSP)
  {
    return;
  }
  
  auto recogPipeMode = currentRecogSP->IsPhraseSpotted() ? RECOG_ONLY : SDET_RECOG;
  auto* const currentRecognizer = currentRecogSP->GetRecognizer();
  
  unsigned short status = RECOG_SILENCE;
  if(!thfRecogPipe(_impl->_thfSession, currentRecognizer, audioDataLen, (short*)audioData, recogPipeMode, &status))
  {
    PRINT_NAMED_ERROR("SpeechRecognizerTHF.Update.thfRecogPipe.Fail", "%s", thfGetLastError(_impl->_thfSession));
    return;
  }
  
  if (!sPhraseForceHeard.empty() || RecogStatusIsEndCondition(status))
  {
    float score = 0;
    const char* foundString = nullptr;
    if (sPhraseForceHeard.empty())
    {
      if (!thfRecogResult(_impl->_thfSession, currentRecognizer, &score, &foundString, NULL, NULL, NULL, NULL, NULL, NULL))
      {
        PRINT_NAMED_ERROR("SpeechRecognizerTHF.Update.thfRecogResult.Fail", "%s", thfGetLastError(_impl->_thfSession));
      }
    }
    else
    {
      foundString = sPhraseForceHeard.c_str();
      status = RECOG_DONE;
    }
    
    if (foundString != nullptr && foundString[0] != '\0')
    {
      DoCallback(foundString, score);
      PRINT_CH_INFO("VoiceCommands", "SpeechRecognizerTHF.Update", "Recognizer score %f %s", score, foundString);
    }
    
    // If the current recognizer allows a followup recognizer to immediately take over
    if (status == RECOG_DONE && currentRecogSP->AllowsFollowupRecog())
    {
      // Verify whether we actually have a followup recognizer set
      const RecogDataSP nextRecogSP = _impl->RetrieveDataForIndex(_impl->_thfFollowupRecog);
      if (nextRecogSP)
      {
        // Actually do the switch over to the new recognizer (as long as this phrase wasn't forced), which copies some buffered audio data
        if (!sPhraseForceHeard.empty() ||
            thfRecogPrepSeq(_impl->_thfSession, nextRecogSP->GetRecognizer(), currentRecognizer))
        {
          PRINT_CH_INFO("VoiceCommands",
                        "SpeechRecognizerTHF.Update",
                        "Switching current recog from %d to %d",
                        _impl->_thfCurrentRecog, _impl->_thfFollowupRecog);
          _impl->_thfCurrentRecog = _impl->_thfFollowupRecog;
          _impl->_thfFollowupRecog = InvalidIndex;
        }
        else
        {
          PRINT_NAMED_ERROR("SpeechRecognizerTHF.Update.thfRecogPrepSeq.Fail", "%s", thfGetLastError(_impl->_thfSession));
        }
      }
    }
    
    if (!thfRecogReset(_impl->_thfSession, currentRecognizer))
    {
      PRINT_NAMED_ERROR("SpeechRecognizerTHF.Update.thfRecogReset.Fail", "%s", thfGetLastError(_impl->_thfSession));
    }
    
    sPhraseForceHeard = "";
  }
}

#else // VOICE_RECOG_PROVIDER == VOICE_RECOG_THF
  
bool SpeechRecognizerTHF::Init() { return true; }
bool SpeechRecognizerTHF::AddRecognitionDataAutoGen(IndexType index,
                                                    const char* const * phraseList,
                                                    unsigned int numPhrases) { return true; }
bool SpeechRecognizerTHF::AddRecognitionDataFromFile(IndexType index,
                                                     const std::string& nnFilePath,
                                                     const std::string& searchFilePath,
                                                     bool isPhraseSpotted,
                                                     bool allowsFollowupRecog)
{
  _impl->_thfAllRecogs[index] = MakeRecogDataSP(isPhraseSpotted, allowsFollowupRecog);
  return true;
}
void SpeechRecognizerTHF::HandleInitFail(const std::string& failMessage) { }
void SpeechRecognizerTHF::Cleanup() { }
bool SpeechRecognizerTHF::RecogStatusIsEndCondition(uint16_t status) { return false; }

void SpeechRecognizerTHF::Update(const AudioUtil::AudioSample * audioData, unsigned int audioDataLen)
{
  // Intentionally make a local copy of the shared ptr with the current recog data
  const RecogDataSP currentRecogSP = _impl->RetrieveDataForIndex(_impl->_thfCurrentRecog);
  if (!currentRecogSP)
  {
    return;
  }
  
  if (!sPhraseForceHeard.empty())
  {
    float score = 0;
    const char* foundString = sPhraseForceHeard.c_str();
    
    if (foundString != nullptr && foundString[0] != '\0')
    {
      DoCallback(foundString, score);
      PRINT_CH_INFO("VoiceCommands", "SpeechRecognizerTHF.Update", "Recognizer score %f %s", score, foundString);
    }
    
    // If the current recognizer allows a followup recognizer to immediately take over
    if (currentRecogSP->AllowsFollowupRecog())
    {
      // Verify whether we actually have a followup recognizer set
      const RecogDataSP nextRecogSP = _impl->RetrieveDataForIndex(_impl->_thfFollowupRecog);
      if (nextRecogSP)
      {
        _impl->_thfCurrentRecog = _impl->_thfFollowupRecog;
        _impl->_thfFollowupRecog = InvalidIndex;
      }
    }
    
    sPhraseForceHeard = "";
  }
}
  
#endif
  
} // end namespace Cozmo
} // end namespace Anki
