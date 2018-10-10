/**
* File: speechRecognizerTHFTypesSimple.cpp
*
* Author: Lee Crippen
* Created: 04/20/2017
* Updated: 10/29/2017 Simple version rename to differentiate from legacy implementation.
*
* Description: SpeechRecognizer Sensory TrulyHandsFree type definitions
*
* Copyright: Anki, Inc. 2017
*
*/

#include "speechRecognizerTHFTypesSimple_v6.h"

namespace Anki {
namespace Vector {

RecogData_v6::RecogData_v6(recog_t* recog, searchs_t* search, bool isPhraseSpotted, bool allowsFollowupRecog)
: _recognizer(recog)
, _search(search)
, _isPhraseSpotted(isPhraseSpotted)
, _allowsFollowupRecog(allowsFollowupRecog)
{ }

RecogData_v6::~RecogData_v6()
{
  DestroyData(_recognizer, _search);
}

RecogData_v6::RecogData_v6(RecogData_v6&& other)
: _recognizer(other._recognizer)
, _search(other._search)
{
  other._recognizer = nullptr;
  other._search = nullptr;
}

RecogData_v6& RecogData_v6::operator=(RecogData_v6&& other) // move assignment
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

void RecogData_v6::DestroyData(recog_t*& recognizer, searchs_t*& search)
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
  
} // end namespace Vector
} // end namespace Anki
