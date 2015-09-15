/**
 * File: emotionManager.h
 *
 * Author: Andrew Stein
 * Date:   8/3/15
 *
 *
 * Description: Defines a class to manage Cozmo's emotional state / mood.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_EMOTION_MANAGER_H
#define ANKI_COZMO_EMOTION_MANAGER_H

#include "anki/common/types.h"
#include "json/json.h"

// TODO: This needs to be switched to some kind of _internal_ signals definition
#include "clad/externalInterface/messageEngineToGame.h"

#include <array>

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  class Robot;
  
  class EmotionManager
  {
  public:
    
    // The underlying value for emotions
    using Value = f32;
    
    static const Value MAX_VALUE;
    static const Value MIN_VALUE;
    
    enum Emotion {
      ANXIOUS = 0,
      SCARED,
      SAD,
      LONELY,
      NUM_MOODS
    };
    
    enum class EmotionEvent {
      CloseFace,
    };
    
    EmotionManager(Robot &robot);
    
    Result Init(const Json::Value& config);
    
    // Get the value for a particular emotion
    Value GetEmotion(Emotion emotion) const;
    
    // Directly adjust a particular emotion by a given value
    // Clips to min/max values.
    void AdjustEmotion(Emotion emotion, Value adjustment);
    
    // Set a particular emotion to a specified value
    // Clips to min/max values.
    void SetEmotion(Emotion emotion, Value newValue);
    
    // Update the current emotions as a function of time
    Result Update(double currentTime_sec);
    
    void HandleEmotionalMoment(EmotionEvent event);
    
  private:
    
    // Event handlers
    void HandleRobotObservedFace(const ExternalInterface::RobotObservedFace& msg);
    
    Robot& _robot;
    Json::Value _config;
    
    std::array<Value, Emotion::NUM_MOODS>  _emoState;
    
    // Function prototype for how emotion values change over time, given current
    // value and current time in seconds.
    using EmotionEvolutionFunction = std::function<Value(Value,f64)>;
    std::map<Emotion, EmotionEvolutionFunction> _evolutionFunctions;
    
    void SetEvolutionFunction(Emotion emotion, EmotionEvolutionFunction fcn);
    
    std::map<Emotion, std::map<EmotionEvent, Value> > _emotionEventImpacts;
  }; // class EmotionManager
  
  
  //
  // Inlined Implementations
  //
  
  inline EmotionManager::Value EmotionManager::GetEmotion(Emotion emotion) const {
    return _emoState[emotion];
  }
  
  inline void EmotionManager::AdjustEmotion(Emotion emotion, Value adjustment) {
    SetEmotion(emotion, _emoState[emotion] + adjustment); // pass through to handle clipping
  }
  
  inline void EmotionManager::SetEmotion(Emotion emotion, Value newValue) {
    _emoState[emotion] = std::max(MIN_VALUE, std::min(MAX_VALUE, newValue));
  }
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_EMOTION_MANAGER_H