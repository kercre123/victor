/**
* File: speechRecognizerTHFSimple_v6.cpp
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

#include "speechRecognizerTHFSimple_v6.h"
#include "speechRecognizerTHFTypesSimple_v6.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"

//#include "coretech/common/engine/utils/data/dataPlatform.h"
//#include "util/fileUtils/fileUtils.h"

#include <algorithm>
#include <array>
#include <locale>
#include <map>
#include <mutex>
#include <string>
#include <sstream>

#pragma GCC diagnostic ignored "-Wunused-function"

namespace Anki {
namespace Vector {
  
// Anonymous namespace to use with debug console vars for manipulating input
namespace {
  std::string sPhraseForceHeard = "";
}

void SpeechRecognizerTHF_v6::SetForceHeardPhrase(const char* phrase)
{
  sPhraseForceHeard = (phrase == nullptr) ? "" : phrase;
}

namespace {
  // THF keyword for "none of the above" that can be used in non-phrasespotted grammars and search lists
  const std::string kNotaString = "*nota";
  
  // Defaults based on the THF documentation for phrasespotting params A and B
//  const int kPhraseSpotParamADefault = -1200;
//  const int kPhraseSpotParamBDefault = 500;
}

// Local definition of data used internally for more strict encapsulation
struct SpeechRecognizerTHF_v6::SpeechRecognizerTHFData
{
  SnsrSession* _snsrSession = nullptr;
//  thf_t*      _thfSession = nullptr;
  
  // We intentionally don't store off and reuse the pronun object. Attempting to do so during testing resulted in
  // crashes when calling into thfSearchCreateFromGrammar and passing in a common pronun object. The safe way to use
  // the pronun object appears to be creating, using, and then destroying it each time a search object is to be created.
  std::string _thfPronunPath;
  
  IndexType                         _thfCurrentRecog = InvalidIndex;
  IndexType                         _thfFollowupRecog = InvalidIndex;
  std::map<IndexType, RecogDataSP>  _thfAllRecogs;
  mutable std::recursive_mutex      _recogMutex;
//  const recog_t*                    _lastUsedRecognizer = nullptr;
  
  const RecogDataSP RetrieveDataForIndex(IndexType index) const;
};
  
SpeechRecognizerTHF_v6::SpeechRecognizerTHF_v6()
: _impl(new SpeechRecognizerTHFData())
{
  
}

SpeechRecognizerTHF_v6::~SpeechRecognizerTHF_v6()
{
  Cleanup();
}

SpeechRecognizerTHF_v6::SpeechRecognizerTHF_v6(SpeechRecognizerTHF_v6&& other)
: SpeechRecognizer(std::move(other))
{
  SwapAllData(other);
}

SpeechRecognizerTHF_v6& SpeechRecognizerTHF_v6::operator=(SpeechRecognizerTHF_v6&& other)
{
  SpeechRecognizer::operator=(std::move(other));
  SwapAllData(other);
  return *this;
}

void SpeechRecognizerTHF_v6::SwapAllData(SpeechRecognizerTHF_v6& other)
{
  auto temp = std::move(other._impl);
  other._impl = std::move(this->_impl);
  this->_impl = std::move(temp);
}

const RecogDataSP SpeechRecognizerTHF_v6::SpeechRecognizerTHFData::RetrieveDataForIndex(IndexType index) const
{
  std::lock_guard<std::recursive_mutex> lock(_recogMutex);
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

void SpeechRecognizerTHF_v6::SetRecognizerIndex(IndexType index)
{
  std::lock_guard<std::recursive_mutex>(_impl->_recogMutex);
  _impl->_thfCurrentRecog = index;
}
  
void SpeechRecognizerTHF_v6::SetRecognizerFollowupIndex(IndexType index)
{
  std::lock_guard<std::recursive_mutex>(_impl->_recogMutex);
  _impl->_thfFollowupRecog = index;
}

SpeechRecognizerTHF_v6::IndexType SpeechRecognizerTHF_v6::GetRecognizerIndex() const
{
  std::lock_guard<std::recursive_mutex>(_impl->_recogMutex);
  return _impl->_thfCurrentRecog;
}

void SpeechRecognizerTHF_v6::RemoveRecognitionData(IndexType index)
{
  std::lock_guard<std::recursive_mutex> lock(_impl->_recogMutex);
  auto indexIter = _impl->_thfAllRecogs.find(index);
  if (indexIter != _impl->_thfAllRecogs.end())
  {
    _impl->_thfAllRecogs.erase(indexIter);
  }
}
  
void SpeechRecognizerTHF_v6::PerformCallBack(const char* callbackArg, float score)
{
  DoCallback(callbackArg, score);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/* VAD endpoint event callback function.
 * Print the segmentation found, and reutrn SNSR_RC_STOP to exit the main loop.
 */
static SnsrRC
endEvent(SnsrSession s, const char *key, void *privateData)
{
  double begin, end;
  snsrGetDouble(s, SNSR_RES_BEGIN_MS, &begin);
  snsrGetDouble(s, SNSR_RES_END_MS, &end);
  PRINT_NAMED_WARNING("SpeechRecognizerTHF_v6",
                      "VAD found audio from %.0f ms to %.0f ms.\n", begin, end);
  return SNSR_RC_STOP;
}


/* VAD detected silence, print a message and continue.
 */
static SnsrRC
silenceEvent(SnsrSession s, const char *key, void *privateData)
{
  PRINT_NAMED_WARNING("SpeechRecognizerTHF_v6",
                      "VAD detected silence. Listening for trigger.\n");
  return SNSR_RC_OK;
}


/* Result callback function, see snsrSetHandler() in main() below.
 * Print the result text and the start and end sample indices of
 * the first spotted phrase.
 */
static SnsrRC
resultEvent(SnsrSession s, const char *key, void *privateData)
{
  SnsrRC r;
  const char *phrase;
  double begin, end;
  
  /* Retrieve the phrase text and alignments from the session handle */
  snsrGetDouble(s, SNSR_RES_BEGIN_SAMPLE, &begin);
  snsrGetDouble(s, SNSR_RES_END_SAMPLE, &end);
  r = snsrGetString(s, SNSR_RES_TEXT, &phrase);
  
  /* Quit early if an error occurred. */
  if (r != SNSR_RC_OK) return r;
  PRINT_NAMED_WARNING("SpeechRecognizerTHF_v6",
                      "Spotted \"%s\" from sample %.0f to sample %.0f.\n",
                      phrase, begin, end);
  
  SpeechRecognizerTHF_v6* inst = (SpeechRecognizerTHF_v6*)privateData;
  inst->PerformCallBack(phrase, 0.0);
  
  return SNSR_RC_OK;
}

/* Push one audio buffer through the recognition pipeline.
 */
SnsrRC
processAudioChunk(SnsrSession s, short *buffer, size_t samples)
{
  /* Create a SnsrStream wrapper for the audio segment and set
   * it as the SnsrSession input audio source. */
  snsrSetStream(s, SNSR_SOURCE_AUDIO_PCM,
                snsrStreamFromMemory(buffer, sizeof(*buffer) * samples,
                                     SNSR_ST_MODE_READ));
  /* Run the pipeline. */
  if (snsrRun(s) == SNSR_RC_STREAM_END) {
    /* All the samples in m were processed. Clear return code. */
    snsrClearRC(s);
  }
  /* Report all other return codes. */
  return snsrRC(s);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  

bool SpeechRecognizerTHF_v6::Init(const std::string& pronunPath)
{
  Cleanup();
  
  PRINT_NAMED_WARNING("SpeechRecognizerTHF_v6.Init", "Enter");
  
  /* Create SDK session */
  SnsrRC result;
  SnsrSession* session = new SnsrSession;
  
  result = snsrNew(session);
  if (result != SnsrRC::SNSR_RC_OK) {
    PRINT_NAMED_ERROR("SpeechRecognizerTHF_v6.Init", "Create session Error '%s'", snsrErrorDetail(*session));
    return false;
  }
//  extern char *optarg;
//  const std::string& triggerDataDir = dataPlatform->pathToResource(Util::Data::Scope::Resources, "assets");
//  Util::FileUtils::FullFilePath({});
//  const std::string filePath; // = Util::fu
  result = snsrLoad(*session, snsrStreamFromFileName(pronunPath.c_str(), "r"));
  
  if (result != SnsrRC::SNSR_RC_OK) {
    PRINT_NAMED_ERROR("SpeechRecognizerTHF_v6.Init", "Load session Error '%s'", snsrErrorDetail(*session));
  }
  
  if (snsrRequire(*session, SNSR_TASK_TYPE, SNSR_PHRASESPOT) != SNSR_RC_OK) {
    PRINT_NAMED_WARNING("SpeechRecognizerTHF_v6.Init", "Enter - snsrRequire");
    /* If this is a phrasespot-vad task, wire up an output buffer */
//    snsrClearRC(s);
//    snsrRequire(s, SNSR_TASK_TYPE, SNSR_PHRASESPOT_VAD);
//    out = snsrStreamFromBuffer(RING_BUFFER_SIZE, RING_BUFFER_SIZE);
//    snsrRetain(out);
//    snsrSetStream(s, SNSR_SINK_AUDIO_PCM, out);
    /* Register VAD endpoint callbacks. */
    snsrSetHandler(*session, SNSR_END_EVENT, snsrCallback(endEvent, NULL, NULL));
    snsrSetHandler(*session, SNSR_LIMIT_EVENT, snsrCallback(endEvent, NULL, NULL));
    snsrSetHandler(*session, SNSR_SILENCE_EVENT,
                   snsrCallback(silenceEvent, NULL, NULL));
  }
  
  
  /* Register a result callback. Private data handle is not used. */
  snsrSetHandler(*session, SNSR_RESULT_EVENT, snsrCallback(resultEvent, NULL, this));
  /* Turn off automatic pipeline flushing that happens when the end of the
   * input stream is reached. */
  snsrSetInt(*session, SNSR_AUTO_FLUSH, 0);
  
  
  
  _impl->_snsrSession = session;
  
  PRINT_NAMED_WARNING("SpeechRecognizerTHF_v6.Init", "EXIT SUCCESS");
  
//  thf_t* createdSession = thfSessionCreate();
//  if(nullptr == createdSession)
//  {
//    /* as of SDK 3.0.9 thfGetLastError(NULL) will return a valid string */
//    const char *err=thfGetLastError(NULL) ? thfGetLastError(NULL) : "could not find dll or out of memory";
//    std::string failMessage = std::string("ERROR thfSessionCreate ") + err;
//    HandleInitFail(failMessage);
//    return false;
//  }
//  _impl->_thfSession = createdSession;
//
//  // Store the pronunciation file path
//  _impl->_thfPronunPath = pronunPath;
  
  return true;
}

void SpeechRecognizerTHF_v6::HandleInitFail(const std::string& failMessage)
{
  PRINT_NAMED_ERROR("SpeechRecognizerTHF_v6.Init.Fail", "%s", failMessage.c_str());
  Cleanup();
}

bool SpeechRecognizerTHF_v6::AddRecognitionDataFromFile(IndexType index,
                                                     const std::string& nnFilePath, const std::string& searchFilePath,
                                                     bool isPhraseSpotted, bool allowsFollowupRecog)
{
//  std::lock_guard<std::recursive_mutex> lock(_impl->_recogMutex);
//  recog_t* createdRecognizer = nullptr;
//  searchs_t* createdSearch = nullptr;
//
//  auto cleanupAfterFailure = [&] (const std::string& failMessage)
//  {
//    PRINT_NAMED_ERROR("SpeechRecognizerTHF_v6.AddRecognitionDataFromFile.Fail", "%s %s", failMessage.c_str(), thfGetLastError(_impl->_thfSession));
//    RecogData_v6::DestroyData(createdRecognizer, createdSearch);
//  };
//
//  if (InvalidIndex == index)
//  {
//    cleanupAfterFailure(std::string("Specified index matches InvalidIndex and cannot be used: ") + std::to_string(index));
//    return false;
//  }
//
//  // First check whether this spot is already taken
//  auto indexIter = _impl->_thfAllRecogs.find(index);
//  if (indexIter != _impl->_thfAllRecogs.end())
//  {
//    cleanupAfterFailure(std::string("Recognizer already added at index ") + std::to_string(index));
//    return false;
//  }
//
//  // The code examples had a buffer size as double the standard chunk size, so we'll do the same
//  auto bufferSizeInSamples = Util::numeric_cast<unsigned short>(AudioUtil::kSamplesPerChunk * 2);
//
//  /* Create recognizer */
//  auto doSpeechDetect = isPhraseSpotted ? NO_SDET : SDET;
//  createdRecognizer = thfRecogCreateFromFile(_impl->_thfSession, nnFilePath.c_str(), bufferSizeInSamples, -1, doSpeechDetect);
//  if(nullptr == createdRecognizer)
//  {
//    cleanupAfterFailure("ERROR thfRecogCreateFromFile");
//    return false;
//  }
//
//  /* Create search */
//  constexpr unsigned short numBestResultsToReturn = 1;
//  createdSearch = thfSearchCreateFromFile(_impl->_thfSession, createdRecognizer, searchFilePath.c_str(), numBestResultsToReturn);
//  if(nullptr == createdSearch)
//  {
//    const char *err = thfGetLastError(_impl->_thfSession);
//    std::string errorMessage = std::string("ERROR thfSearchCreateFromFile ") + err;
//    cleanupAfterFailure(errorMessage);
//    return false;
//  }
//
//  /* Initialize recognizer */
//  if(!thfRecogInit(_impl->_thfSession, createdRecognizer, createdSearch, RECOG_KEEP_NONE))
//  {
//    cleanupAfterFailure("ERROR thfRecogInit");
//    return false;
//  }
//
//  if (allowsFollowupRecog)
//  {
//    if (!isPhraseSpotted)
//    {
//      cleanupAfterFailure("Tried to set up phrase following with non-phrasespotting recognizers, which is not allowed.");
//      return false;
//    }
//
//    constexpr float overlapTime_ms = 1000.f;
//    if (!thfPhrasespotConfigSet(_impl->_thfSession, createdRecognizer, createdSearch, PS_SEQ_BUFFER, overlapTime_ms))
//    {
//      cleanupAfterFailure("ERROR thfPhrasespotConfigSet PS_SEQ_BUFFER");
//      return false;
//    }
//  }
//
//  // This delay is recommended when using more complex command sets, but ours is simple for the moment. Keeping this
//  // here for now just for reference.
//  // if (isPhraseSpotted)
//  // {
//  //   if (!thfPhrasespotConfigSet(_impl->_thfSession, createdRecognizer, createdSearch, PS_DELAY, 90))
//  //   {
//  //     cleanupAfterFailure("ERROR thfPhrasespotConfigSet PS_DELAY");
//  //     return false;
//  //   }
//  // }
//
//  // Everything should be happily added, so store off this recognizer
//  _impl->_thfAllRecogs[index] = MakeRecogDataSP(createdRecognizer, createdSearch, isPhraseSpotted, allowsFollowupRecog);
  
  return true;
}

void SpeechRecognizerTHF_v6::Cleanup()
{
  std::lock_guard<std::recursive_mutex> lock(_impl->_recogMutex);
  
  if (_impl->_snsrSession != nullptr) {
    snsrRelease(_impl->_snsrSession);
    _impl->_snsrSession = nullptr;
  }
  
//  _impl->_thfAllRecogs.clear();
//
//  if (_impl->_thfSession)
//  {
//    thfSessionDestroy(_impl->_thfSession);
//    _impl->_thfSession = nullptr;
//  }
}

bool SpeechRecognizerTHF_v6::RecogStatusIsEndCondition(uint16_t status)
{
//  switch (status)
//  {
//    case RECOG_SILENCE:   //Timed out waiting for start of speech (end condition).
//    case RECOG_DONE:      //End of utterance detected (end condition).
//    case RECOG_MAXREC:    //Timed out waiting for end of utterance (end condition).
//    case RECOG_IGNORE:    //Speech detector triggered but failed the minduration test. Probably not speech (end condition).
//    case RECOG_NODATA:    //The input audio buffer was empty (error condition).
//      return true;
//    default:
//      return false;
//  }
  return false;
}

void SpeechRecognizerTHF_v6::Update(const AudioUtil::AudioSample * audioData, unsigned int audioDataLen)
{
  
  /* Process one block of audio. */
  auto r = processAudioChunk(*_impl->_snsrSession, (short*)audioData, audioDataLen);
  /* The result callback returns SNSR_RC_STOP when a phrase is spotted. */
  if (r == SNSR_RC_STOP) {
    
  }
  if (r != SNSR_RC_OK) {
//    fprintf(stderr, "ERROR: %s\n", snsrErrorDetail(s));
//    exit(3);
  }
  
  
//  // Intentionally make a local copy of the shared ptr with the current recog data
//  RecogDataSP currentRecogSP;
//  {
//    std::lock_guard<std::recursive_mutex> lock(_impl->_recogMutex);
//    currentRecogSP = _impl->RetrieveDataForIndex(GetRecognizerIndex());
//  }
//
//  if (nullptr == currentRecogSP)
//  {
//    return;
//  }
//
//  // If the recognizer has changed since last update, we need to potentially reset and store it again
//  auto* const currentRecognizer = currentRecogSP->GetRecognizer();
//  if (currentRecognizer != _impl->_lastUsedRecognizer)
//  {
//    // If we actually had a last recognizer set, then we need to reset
//    if (_impl->_lastUsedRecognizer && !thfRecogReset(_impl->_thfSession, currentRecognizer))
//    {
//      PRINT_NAMED_ERROR("SpeechRecognizerTHF_v6.Update.thfRecogReset.Fail", "%s", thfGetLastError(_impl->_thfSession));
//    }
//
//    _impl->_lastUsedRecognizer = currentRecognizer;
//  }
//
//  auto recogPipeMode = currentRecogSP->IsPhraseSpotted() ? RECOG_ONLY : SDET_RECOG;
//  unsigned short status = RECOG_SILENCE;
//  if(!thfRecogPipe(_impl->_thfSession, currentRecognizer, audioDataLen, (short*)audioData, recogPipeMode, &status))
//  {
//    PRINT_NAMED_ERROR("SpeechRecognizerTHF_v6.Update.thfRecogPipe.Fail", "%s", thfGetLastError(_impl->_thfSession));
//    return;
//  }
//
//  if (!sPhraseForceHeard.empty() || RecogStatusIsEndCondition(status))
//  {
//    float score = 0;
//    const char* foundStringRaw = nullptr;
//    if (sPhraseForceHeard.empty())
//    {
//      if (!thfRecogResult(_impl->_thfSession, currentRecognizer, &score, &foundStringRaw, NULL, NULL, NULL, NULL, NULL, NULL))
//      {
//        PRINT_NAMED_ERROR("SpeechRecognizerTHF_v6.Update.thfRecogResult.Fail", "%s", thfGetLastError(_impl->_thfSession));
//      }
//    }
//    else
//    {
//      foundStringRaw = sPhraseForceHeard.c_str();
//      score = -1.0f;
//      status = RECOG_DONE;
//    }
//
//    if (foundStringRaw != nullptr && foundStringRaw[0] != '\0' && kNotaString.compare(foundStringRaw) != 0)
//    {
//      std::string foundString{foundStringRaw};
//      std::replace(foundString.begin(), foundString.end(), '_', ' ');
//      DoCallback(foundString.c_str(), score);
//      PRINT_CH_INFO("VoiceCommands", "SpeechRecognizerTHF_v6.Update", "Recognizer score %f %s", score, foundString.c_str());
//    }
//
//    // If the current recognizer allows a followup recognizer to immediately take over
//    if (status == RECOG_DONE && currentRecogSP->AllowsFollowupRecog())
//    {
//      // Verify whether we actually have a followup recognizer set
//      const RecogDataSP nextRecogSP = _impl->RetrieveDataForIndex(_impl->_thfFollowupRecog);
//      if (nextRecogSP)
//      {
//        // Actually do the switch over to the new recognizer (as long as this phrase wasn't forced), which copies some buffered audio data
//        if (!sPhraseForceHeard.empty() ||
//            thfRecogPrepSeq(_impl->_thfSession, nextRecogSP->GetRecognizer(), currentRecognizer))
//        {
//          std::lock_guard<std::recursive_mutex>(_impl->_recogMutex);
//          PRINT_CH_INFO("VoiceCommands",
//                        "SpeechRecognizerTHF_v6.Update",
//                        "Switching current recog from %d to %d",
//                        _impl->_thfCurrentRecog, _impl->_thfFollowupRecog);
//          _impl->_thfCurrentRecog = _impl->_thfFollowupRecog;
//          _impl->_thfFollowupRecog = InvalidIndex;
//        }
//        else
//        {
//          PRINT_NAMED_ERROR("SpeechRecognizerTHF_v6.Update.thfRecogPrepSeq.Fail", "%s", thfGetLastError(_impl->_thfSession));
//        }
//      }
//    }
//
//    if (!thfRecogReset(_impl->_thfSession, currentRecognizer))
//    {
//      PRINT_NAMED_ERROR("SpeechRecognizerTHF_v6.Update.thfRecogReset.Fail", "%s", thfGetLastError(_impl->_thfSession));
//    }
//
//    sPhraseForceHeard = "";
//  }
}
  
} // end namespace Vector
} // end namespace Anki
