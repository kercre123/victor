/**
* File: speechRecognizerTHFTypes.cpp
*
* Author: Lee Crippen
* Created: 04/20/2017
*
* Description: SpeechRecognizer Sensory TrulyHandsFree type definitions
*
* Copyright: Anki, Inc. 2017
*
*/

#include "anki/cozmo/basestation/voiceCommands/speechRecognizerTHFTypes.h"

namespace Anki {
namespace Cozmo {

RecogData::RecogData(recog_t* recog, searchs_t* search, bool isPhraseSpotted, bool allowsFollowupRecog)
: _recognizer(recog)
, _search(search)
, _isPhraseSpotted(isPhraseSpotted)
, _allowsFollowupRecog(allowsFollowupRecog)
{ }

RecogData::~RecogData()
{
  DestroyData(_recognizer, _search);
}

RecogData::RecogData(RecogData&& other)
: _recognizer(other._recognizer)
, _search(other._search)
{
  other._recognizer = nullptr;
  other._search = nullptr;
}

RecogData& RecogData::operator=(RecogData&& other) // move assignment
{
  if(this != &other) // prevent self-move
  {
    DestroyData(_recognizer, _search);
    _recognizer = other._recognizer;
    _search = other._search;
    other._recognizer = nullptr;
    other._search = nullptr;
  }
  return *this;
}

void RecogData::DestroyData(recog_t*& recognizer, searchs_t*& search)
{
  if (recognizer)
  {
    thfRecogDestroy(recognizer);
    recognizer = nullptr;
  }
  
  if (search)
  {
    thfSearchDestroy(search);
    search = nullptr;
  }
}
  
} // end namespace Cozmo
} // end namespace Anki
