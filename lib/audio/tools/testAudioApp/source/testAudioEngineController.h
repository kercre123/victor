/**
 * File: testAudioEngineController.h
 *
 * Author: Jordan Rivas
 * Created: 1/16/18
 *
 * Description: Test Audio App interface to Audio Engine
 *              - Subclass of AudioEngine::AudioEngineController
 *              - Implement helper functions to load sound banks and perform audio actions
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Anki_TestAudioEngineController_H__
#define __Anki_TestAudioEngineController_H__

#include "audioEngine/audioEngineController.h"
#include <functional>
#include <string>


class TestAudioEngineController : public Anki::AudioEngine::AudioEngineController {
public:
  
  TestAudioEngineController(const std::string& basePath,
                            const std::string& writePath,
                            const std::string& localeTitle);
  
  bool LoadAudioScene(const std::string& audioScenePath);
  
  void PostNextAction(std::function<void()> completedCallbackFunc);
  
  bool AllActionsCompleted() const;
  
  void RestActionIdx() { _actionIdx = 0; }
  
  
private:
  
  Anki::AudioEngine::SetupConfig _config;
  
  Anki::AudioEngine::AudioGameObject _gameObj = Anki::AudioEngine::kInvalidAudioGameObject;
  
  const Anki::AudioEngine::AudioScene* _activeScene = nullptr;
  
  size_t _actionIdx = 0;
  
};


#endif /* __Anki_TestAudioEngineController_H__ */
