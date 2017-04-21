/**
* File: speechRecognizerTHFTypes.h
*
* Author: Lee Crippen
* Created: 04/20/2017
*
* Description: SpeechRecognizer Sensory TrulyHandsFree type definitions
*
* Copyright: Anki, Inc. 2017
*
*/
#ifndef __Cozmo_Basestation_VoiceCommands_SpeechRecognizerTHFTypes_H_
#define __Cozmo_Basestation_VoiceCommands_SpeechRecognizerTHFTypes_H_
#if VOICE_RECOG_PROVIDER == VOICE_RECOG_THF

#include "trulyhandsfree.h"

#include <memory>
#include <utility>

namespace Anki {
namespace Cozmo {

class RecogData
{
public:
  RecogData(recog_t* recog, searchs_t* search, bool isPhraseSpotted, bool allowsFollowupRecog);
  ~RecogData();
  
  RecogData(RecogData&& other);
  RecogData& operator=(RecogData&& other);
  
  RecogData(const RecogData& other) = delete;
  RecogData& operator=(const RecogData& other) = delete;
  
  recog_t* GetRecognizer() const { return _recognizer; }
  searchs_t* GetSearch() const { return _search; }
  bool IsPhraseSpotted() const { return _isPhraseSpotted; }
  bool AllowsFollowupRecog() const { return _allowsFollowupRecog; }
  
  static void DestroyData(recog_t*& recognizer, searchs_t*& search);
  
private:
  recog_t*          _recognizer = nullptr;
  searchs_t*        _search = nullptr;
  bool              _isPhraseSpotted = false;
  bool              _allowsFollowupRecog = false;
  
  void DestroyData();
};
  
using RecogDataSP = std::shared_ptr<RecogData>;
  
template <typename ...Args>
static RecogDataSP MakeRecogDataSP(Args&& ...args)
{
  return std::shared_ptr<RecogData>(new RecogData(std::forward<Args>(args)...));
}

} // end namespace Cozmo
} // end namespace Anki

#endif // VOICE_RECOG_PROVIDER == VOICE_RECOG_THF
#endif // __Cozmo_Basestation_VoiceCommands_SpeechRecognizerTHFTypes_H_
