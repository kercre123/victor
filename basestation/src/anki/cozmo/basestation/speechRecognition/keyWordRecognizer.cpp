/**
* File: recognizer
*
* Author: damjan stulic
* Created: 9/3/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/

#include "anki/cozmo/basestation/speechRecognition/keyWordRecognizer.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/logging/logging.h"
#include <stdio.h>
#include <string>
#include <assert.h>
#include <sys/select.h>
#include <sphinxbase/err.h>

namespace Anki {
namespace Cozmo {
namespace SpeechRecognition {

KeyWordRecognizer::KeyWordRecognizer(IExternalInterface* externalInterface)
: _externalInterface(externalInterface)
, _started(false)
, ps(nullptr)
, config(nullptr)
, ad(nullptr)
, adbuf{0}
, utt_started(false)
, in_speech(0)
, k(0)
{
  _translationPairs = {
    // ordered list
    { "watch me", KeyWord::WatchMe },
    { "fetch", KeyWord::Fetch },
    { "hello", KeyWord::Hello },
    { "goodbye", KeyWord::GoodBye },
    { "come here", KeyWord::ComeHere },
    { "pick up", KeyWord::PickUp },
    { "drop", KeyWord::Drop },
    { "stack", KeyWord::Stack }
  };
}

KeyWordRecognizer::~KeyWordRecognizer()
{
  CleanUp();
}

void sphinxLogger(void* user_data, err_lvl_t level, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  switch (level){
    case ERR_DEBUG:
      //Util::sDebugV("KeywordRecognizer.sphinx", {{}}, format, args);
      break;
    case ERR_INFO:
      //Util::sInfoV("KeywordRecognizer.sphinx", {{}}, format, args);
      break;
    case ERR_INFOCONT:
      //Util::sInfoV("KeywordRecognizer.sphinx", {{}}, format, args);
      break;
    case ERR_WARN:
      Util::sWarningV("KeywordRecognizer.sphinx", {{}}, format, args);
      break;
    case ERR_ERROR:
    case ERR_FATAL:
    case ERR_MAX:
      Util::sErrorV("KeywordRecognizer.sphinx", {{}}, format, args);
      break;
  }
  va_end(args);
}

void KeyWordRecognizer::Init(const std::string& hmmFile, const std::string& file, const std::string& dictFile)
{
  err_set_logfp(nullptr);
  err_set_callback(sphinxLogger, nullptr);

  config =
    cmd_ln_init(nullptr, ps_args(), true, "-hmm", hmmFile.c_str(),
      "-kws", file.c_str(),
      "-dict", dictFile.c_str(), nullptr);

  ps_default_search_args(config);
  ps = ps_init(config);
  if (ps == nullptr) {
    PRINT_NAMED_ERROR("KeywordRecognizer", "init failed");
    cmd_ln_free_r(config);
    return;
  }

  _signalHandles.push_back(
    _externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::KeyWordRecognitionEnabled,
    [this](const AnkiEvent<ExternalInterface::MessageGameToEngine>& event){
      this->Start();
    })
  );

  _signalHandles.push_back(
    _externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::KeyWordRecognitionDisabled,
      [this](const AnkiEvent<ExternalInterface::MessageGameToEngine>& event){
        this->Stop();
      })
  );

}

void KeyWordRecognizer::CleanUp()
{
  ps_free(ps);
  cmd_ln_free_r(config);
}

void KeyWordRecognizer::Start()
{
  if (ps == nullptr) {
    return;
  }
  if (_started) {
    return;
  }
  if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
    (int) cmd_ln_float32_r(config, "-samprate"))) == nullptr) {
    PRINT_NAMED_ERROR("KeywordRecognizer", "Failed to open audio device");
  }
  if (ad_start_rec(ad) < 0) {
    PRINT_NAMED_ERROR("KeywordRecognizer", "Failed to start recording");
  }

  if (ps_start_utt(ps) < 0) {
    PRINT_NAMED_ERROR("KeywordRecognizer", "Failed to start utterance");
  }
  utt_started = false;
  _started = true;
}

void KeyWordRecognizer::Update(unsigned int millisecondsPassed)
{
  if (ps == nullptr) {
    return;
  }
  if (!_started) {
    return;
  }
  if ((k = ad_read(ad, adbuf, adbuffSize)) < 0) {
    PRINT_NAMED_ERROR("KeywordRecognizer", "Failed to read audio");
  }

  ps_process_raw(ps, adbuf, (size_t)k, false, false);
  in_speech = ps_get_in_speech(ps);
  if (in_speech && !utt_started) {
    utt_started = true;
  }
  if (!in_speech && utt_started) {
    /* speech -> silence transition, time to start new utterance  */
    ps_end_utt(ps);
    int32 score;
    const char* hyp = ps_get_hyp(ps, &score );
    if (hyp != nullptr) {
      TranslateHypothesisToEvent(hyp, score);
    }

    if (ps_start_utt(ps) < 0) {
      E_FATAL("Failed to start utterance\n");
    }
    utt_started = false;
  }
}

void KeyWordRecognizer::Stop()
{
  if (ps == nullptr) {
    return;
  }
  if (!_started) {
    return;
  }
  ps_end_utt(ps);
  ad_close(ad);
  _started = false;
}

void KeyWordRecognizer::TranslateHypothesisToEvent(const char* hypothesis, int32_t score)
{
  std::string hyp(hypothesis);
  for (const auto& translationPair : _translationPairs) {
    if (hyp.find(translationPair.first) != std::string::npos) {
      PRINT_NAMED_INFO("KeywordRecognizer", "keyword [%s], recognized in hypothesis [%s]", translationPair.first.c_str(), hypothesis);
      _externalInterface->Broadcast(
        ExternalInterface::MessageEngineToGame(ExternalInterface::KeyWordRecognized(translationPair.second, score))
      );
      return;
    }
  }
}

} // end namespace SpeechRecognition
} // end namespace Cozmo
} // end namespace Anki

