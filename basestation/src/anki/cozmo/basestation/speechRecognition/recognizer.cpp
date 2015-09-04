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

#include "anki/cozmo/basestation/speechRecognition/recognizer.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/logging/logging.h"
#include <stdio.h>
#include <string>
#include <assert.h>
#include <sys/select.h>
#include <sphinxbase/err.h>

namespace Anki {
namespace Cozmo {


Recognizer::Recognizer(IExternalInterface* externalInterface)
: _externalInterface(externalInterface)
, ps(nullptr)
, config(nullptr)
, ad(nullptr)
, adbuf{0}
, utt_started(false)
, in_speech(0)
, k(0)
{

}

Recognizer::~Recognizer()
{
  CleanUp();
}

void Recognizer::Init(const std::string& hmmFile, const std::string& file, const std::string& dictFile)
{
  config =
    cmd_ln_init(nullptr, ps_args(), true, "-hmm", hmmFile.c_str(),
      "-kws", file.c_str(),
      "-dict", dictFile.c_str(), nullptr);

  ps_default_search_args(config);
  ps = ps_init(config);
  if (ps == nullptr) {
    PRINT_NAMED_ERROR("KeywordRecognizer","init failed");
    cmd_ln_free_r(config);
    return;
  }
}

void Recognizer::CleanUp()
{
  ps_free(ps);
  cmd_ln_free_r(config);
}

void Recognizer::Start()
{
  if (ps == nullptr) {
    return;
  }
  //privateCallback = callback;
  if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
    (int) cmd_ln_float32_r(config, "-samprate"))) == nullptr) {
    E_FATAL("Failed to open audio device\n");
  }
  if (ad_start_rec(ad) < 0) {
    E_FATAL("Failed to start recording\n");
  }

  if (ps_start_utt(ps) < 0) {
    E_FATAL("Failed to start utterance\n");
  }
  utt_started = false;
}

void Recognizer::Update(unsigned int millisecondsPassed)
{
  if (ps == nullptr) {
    return;
  }
  if ((k = ad_read(ad, adbuf, adbuffSize)) < 0) {
    E_FATAL("Failed to read audio\n");
  }

  ps_process_raw(ps, adbuf, k, false, false);
  in_speech = ps_get_in_speech(ps);
  if (in_speech && !utt_started) {
    utt_started = true;
    printf("Listening...\n");
  }
  if (!in_speech && utt_started) {
    /* speech -> silence transition, time to start new utterance  */
    ps_end_utt(ps);
    int32 score;
    const char* hyp = ps_get_hyp(ps, &score );
    if (hyp != nullptr) {
      _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::Ping()));
      //printf("-=-=-=-=-=-=-=-=-  %s - %d\n", hyp, score);
      //if (privateCallback != NULL) {
      //  privateCallback(hyp);
      //}
    }

    if (ps_start_utt(ps) < 0) {
      E_FATAL("Failed to start utterance\n");
    }
    utt_started = false;
//    printf("READY....\n");
  }
}

void Recognizer::Stop()
{
  if (ps == nullptr) {
    return;
  }
  ps_end_utt(ps);
  ad_close(ad);
  //privateCallback = nullptr;
}


} // end namespace Cozmo
} // end namespace Anki

