//
//  MayaAudioController.h
//  AnkiMayaWWisePlugIn
//
//  Created by Jordan Rivas on 1/09/18.
//  Copyright © 2016 Anki, Inc. All rights reserved.
//

#ifndef __Anki_MayaAudioController_H__
#define __Anki_MayaAudioController_H__

#include "audioActionTypes.h"
#include "audioEngine/audioEngineController.h"
#include "audioEngine/soundbankLoader.h"
#include <memory>


class MayaAudioController : public Anki::AudioEngine::AudioEngineController
{
public:
  
  MayaAudioController( char* soundbanksPath );
  
  bool PostAudioKeyframe( const AudioKeyframe* keyframe );

  bool PlayAudioEvent( const std::string& eventName );

  bool SetParameterValue( const std::string& paramName, const float paramValue );

private:
  
  std::unique_ptr<Anki::AudioEngine::SoundbankLoader> _soundbankLoader;
  
  Anki::AudioEngine::AudioGameObject _gameObj = Anki::AudioEngine::kInvalidAudioGameObject;
  
};


#endif /* __Anki_MayaAudioController_H__ */
