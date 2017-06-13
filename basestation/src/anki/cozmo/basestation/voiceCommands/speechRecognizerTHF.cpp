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

#include "anki/cozmo/basestation/voiceCommands/phraseData.h"
#include "anki/cozmo/basestation/voiceCommands/speechRecognizerTHF.h"
#include "anki/cozmo/basestation/voiceCommands/speechRecognizerTHFTypes.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"

#include "trulyhandsfree.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <locale>
#include <map>
#include <mutex>
#include <string>
#include <sstream>

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

namespace {
  // THF keyword for "none of the above" that can be used in non-phrasespotted grammars and search lists
  const std::string kNotaString = "*nota";
  
  // Defaults based on the THF documentation for phrasespotting params A and B
  const int kPhraseSpotParamADefault = -1200;
  const int kPhraseSpotParamBDefault = 500;
}

// Local definition of data used internally for more strict encapsulation
struct SpeechRecognizerTHF::SpeechRecognizerTHFData
{
  thf_t*      _thfSession = nullptr;
  pronuns_t*  _thfPronun = nullptr;
  
  IndexType                         _thfCurrentRecog = InvalidIndex;
  IndexType                         _thfFollowupRecog = InvalidIndex;
  std::map<IndexType, RecogDataSP>  _thfAllRecogs;
  std::mutex                        _recogMutex;
  const recog_t*                    _lastUsedRecognizer = nullptr;
  
  const RecogDataSP RetrieveDataForIndex(IndexType index) const;
  searchs_t* CreatePhraseSpottedSearch(recog_t* createdRecognizer,
                                       const std::vector<VoiceCommand::PhraseDataSharedPtr>& phraseList) const;
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
  std::lock_guard<std::mutex>(_impl->_recogMutex);
  _impl->_thfCurrentRecog = index;
}
  
void SpeechRecognizerTHF::SetRecognizerFollowupIndex(IndexType index)
{
  std::lock_guard<std::mutex>(_impl->_recogMutex);
  _impl->_thfFollowupRecog = index;
}

SpeechRecognizerTHF::IndexType SpeechRecognizerTHF::GetRecognizerIndex() const
{
  std::lock_guard<std::mutex>(_impl->_recogMutex);
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

bool SpeechRecognizerTHF::Init(const std::string& pronunPath)
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
  pronuns_t* createdPronunciation = thfPronunCreateFromFile(_impl->_thfSession, pronunPath.c_str(), 0);
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

bool SpeechRecognizerTHF::AddRecognitionDataAutoGen(IndexType index,
                                                    const std::string& nnFilePath,
                                                    const std::vector<VoiceCommand::PhraseDataSharedPtr>& phraseList,
                                                    bool isPhraseSpotted, bool allowsFollowupRecog)
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
  if (isPhraseSpotted)
  {
    createdSearch = _impl->CreatePhraseSpottedSearch(createdRecognizer, phraseList);
  }
  else
  {
    std::vector<const char*> recogList;
    for (const auto& listItem : phraseList)
    {
      recogList.push_back(listItem->GetPhrase().c_str());
    }
    // Non-phrasespotted lists do better with a *nota entry to capture sound that doesn't match:
    recogList.push_back(kNotaString.c_str());
    
    const auto& itemList = static_cast<const char**>(recogList.data());
    const auto& numItems = Util::numeric_cast<unsigned short>(recogList.size());
    constexpr unsigned short numBestResultsToReturn = 1;
    createdSearch = thfSearchCreateFromList(_impl->_thfSession, createdRecognizer, _impl->_thfPronun,
                                            itemList, NULL, numItems,
                                            numBestResultsToReturn);
  }
  if(!createdSearch)
  {
    const char *err = thfGetLastError(_impl->_thfSession);
    std::string searchMethod = isPhraseSpotted ? "thfPhrasespotCreateFromList " : "thfSearchCreateFromList ";
    std::string errorMessage = std::string("ERROR ") + searchMethod + err;
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

searchs_t* SpeechRecognizerTHF::SpeechRecognizerTHFData::CreatePhraseSpottedSearch(recog_t* createdRecognizer,
                                                                                   const std::vector<VoiceCommand::PhraseDataSharedPtr>& phraseList) const
{
  if (nullptr == createdRecognizer)
  {
    return nullptr;
  }

  // Go through list of phrases, pull out all unique words, and grab their pronunciation
  std::map<std::string, std::string> wordToPronunMap;
  std::vector<const char*> wordList;
  std::vector<const char*> pronunList;
  for (const auto& phrase : phraseList)
  {
    std::istringstream wordStream = std::istringstream(phrase->GetPhrase());
    for(std::string word; wordStream >> word; )
    {
      const auto& iter = wordToPronunMap.find(word);
      if (iter == wordToPronunMap.end())
      {
        const auto& numResultsDesired = static_cast<unsigned short>( 1 );
        const char* pronun = thfPronunCompute(_thfSession, _thfPronun, word.c_str(), numResultsDesired, nullptr, nullptr);
        wordToPronunMap[word] = pronun;
        
        auto newIter = wordToPronunMap.find(word);
        wordList.push_back(newIter->first.c_str());
        pronunList.push_back(newIter->second.c_str());
      }
    }
  }
  
  // Compose the grammar for this list
  
  // Define each of the phrases as its own var in the grammar
  std::stringstream grammarStream;
  for (int i=0; i<phraseList.size(); ++i)
  {
    grammarStream << std::to_string(i) << " = " << phraseList[i]->GetPhrase() << "; ";
  }
  
  // Add in the final grammar definition ORing all the vars together
  grammarStream << "g = ";
  for (int i=0; i<phraseList.size(); ++i)
  {
    grammarStream << "$" << std::to_string(i);
    if (i < (phraseList.size() - 1))
    {
      grammarStream << " | ";
    }
  }
  grammarStream << "; ";
  
  // Set the params for each phrase var
  for (int i=0; i<phraseList.size(); ++i)
  {
    auto paramAValue = kPhraseSpotParamADefault;
    if (phraseList[i]->HasDataValueSet(VoiceCommand::PhraseData::DataValueType::ParamA))
    {
      paramAValue = phraseList[i]->GetParamA();
    }
    
    auto paramBValue = kPhraseSpotParamBDefault;
    if (phraseList[i]->HasDataValueSet(VoiceCommand::PhraseData::DataValueType::ParamB))
    {
      paramBValue = phraseList[i]->GetParamB();
    }
    
    // We always want to set params A and B
    grammarStream << "paramA:$" << std::to_string(i) << " " << std::to_string(paramAValue) << "; ";
    grammarStream << "paramB:$" << std::to_string(i) << " " << std::to_string(paramBValue) << "; ";
    
    // Special case for paramC: only add it to the grammar if it was set in the phrase data
    if (phraseList[i]->HasDataValueSet(VoiceCommand::PhraseData::DataValueType::ParamC))
    {
      grammarStream << "paramC:$" << std::to_string(i) << " " << std::to_string(phraseList[i]->GetParamC()) << "; ";
    }
  }
  
  std::string grammarString = grammarStream.str();
  const auto& numItems = Util::numeric_cast<unsigned short>(wordToPronunMap.size());
  const auto& numResultsDesired = static_cast<unsigned short>( 1 );
  
  // Use the grammar, wordlist, and pronunlist to set up the phrasespotted search
  return thfSearchCreateFromGrammar(_thfSession, createdRecognizer, _thfPronun, grammarString.c_str(), wordList.data(), pronunList.data(), numItems, numResultsDesired, PHRASESPOTTING);
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
  const RecogDataSP currentRecogSP = _impl->RetrieveDataForIndex(GetRecognizerIndex());
  if (!currentRecogSP)
  {
    return;
  }
  
  // If the recognizer has changed since last update, we need to potentially reset and store it again
  auto* const currentRecognizer = currentRecogSP->GetRecognizer();
  if (currentRecognizer != _impl->_lastUsedRecognizer)
  {
    // If we actually had a last recognizer set, then we need to reset
    if (_impl->_lastUsedRecognizer && !thfRecogReset(_impl->_thfSession, currentRecognizer))
    {
      PRINT_NAMED_ERROR("SpeechRecognizerTHF.Update.thfRecogReset.Fail", "%s", thfGetLastError(_impl->_thfSession));
    }
    
    _impl->_lastUsedRecognizer = currentRecognizer;
  }
  
  auto recogPipeMode = currentRecogSP->IsPhraseSpotted() ? RECOG_ONLY : SDET_RECOG;
  unsigned short status = RECOG_SILENCE;
  if(!thfRecogPipe(_impl->_thfSession, currentRecognizer, audioDataLen, (short*)audioData, recogPipeMode, &status))
  {
    PRINT_NAMED_ERROR("SpeechRecognizerTHF.Update.thfRecogPipe.Fail", "%s", thfGetLastError(_impl->_thfSession));
    return;
  }
  
  if (!sPhraseForceHeard.empty() || RecogStatusIsEndCondition(status))
  {
    float score = 0;
    const char* foundStringRaw = nullptr;
    if (sPhraseForceHeard.empty())
    {
      if (!thfRecogResult(_impl->_thfSession, currentRecognizer, &score, &foundStringRaw, NULL, NULL, NULL, NULL, NULL, NULL))
      {
        PRINT_NAMED_ERROR("SpeechRecognizerTHF.Update.thfRecogResult.Fail", "%s", thfGetLastError(_impl->_thfSession));
      }
    }
    else
    {
      foundStringRaw = sPhraseForceHeard.c_str();
      score = -1.0f;
      status = RECOG_DONE;
    }
    
    if (foundStringRaw != nullptr && foundStringRaw[0] != '\0' && kNotaString.compare(foundStringRaw) != 0)
    {
      std::string foundString{foundStringRaw};
      std::replace(foundString.begin(), foundString.end(), '_', ' ');
      DoCallback(foundString.c_str(), score);
      PRINT_CH_INFO("VoiceCommands", "SpeechRecognizerTHF.Update", "Recognizer score %f %s", score, foundString.c_str());
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
          std::lock_guard<std::mutex>(_impl->_recogMutex);
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
  
} // end namespace Cozmo
} // end namespace Anki
