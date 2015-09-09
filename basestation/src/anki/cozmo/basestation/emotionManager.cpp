/**
 * File: emotionManager.cpp
 *
 * Author: Andrew Stein
 * Date:   8/3/15
 *
 *
 * Description: Implements a class to manage Cozmo's emotional state / mood.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/emotionManager.h"

#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

  const EmotionManager::Value EmotionManager::MAX_VALUE = 1.f;
  const EmotionManager::Value EmotionManager::MIN_VALUE = 0.f;
  
  EmotionManager::Value DefaultEmotionEvolution(EmotionManager::Value currentValue,
                                                f64 currentTime_sec,
                                                f32 factor)
  {
    // this just ignores time and blindly tends things towards zero...
    return currentValue * factor;
  }
  
  EmotionManager::EmotionManager(Robot& robot)
  : _robot(robot)
  , _emoState{} // init with all zeros
  {
    
  }
  
  Result EmotionManager::Init(const Json::Value& config)
  {
    // TODO: Set evolution functions based on config information
    auto sadDecayRateName = "SadnessDecayRate";
    if (config.isMember(sadDecayRateName))
      {
      auto sadnessEvolveFcn = [&](Value value, f64 t) {
        return DefaultEmotionEvolution(value,t,config[sadDecayRateName].asFloat());
      };
      SetEvolutionFunction(SAD, sadnessEvolveFcn);
    }
    else
    {
      PRINT_NAMED_ERROR("EmotionManager.Init.Sad", "Missing config key %s", sadDecayRateName);
    }
    
    auto scaredName = "scared";
    auto resetDurationName = "reset_duration";
    if (config.isMember(scaredName) && config[scaredName].isMember(resetDurationName))
    {
      auto scaredResetTime = config[scaredName][resetDurationName].asFloat();
      auto scaredEvolveFcn = [scaredResetTime] (Value value, f64 t) {
        static auto lastReset = t;
        if (t - lastReset >= scaredResetTime)
        {
          lastReset = t;
          return Value(0);
        }
        return value;
      };
      
      SetEvolutionFunction(SCARED, scaredEvolveFcn);
    }
    else
    {
      PRINT_NAMED_ERROR("EmotionManager.Init.Scared", "Missing config key %s.%s", scaredName, resetDurationName);
    }
    
    return Result::RESULT_OK;
  }
  
  Result EmotionManager::Update(double currentTime_sec)
  {
    // Update emotions according to any evolution functions that are set
    for(auto & fcnPair : _evolutionFunctions)
    {
      Emotion whichEmotion = fcnPair.first;
      Value currentValue = GetEmotion(whichEmotion);
      EmotionEvolutionFunction& fcn = fcnPair.second;
      
      SetEmotion(whichEmotion, fcn(currentValue, currentTime_sec));
    }
    
    // TODO: Remove this placeholder -- just here to prevent error that "_robot" is unused.
    _robot.GetID();
    
    return RESULT_OK;
  }
  
  
  void EmotionManager::HandleRobotObservedFace(const ExternalInterface::RobotObservedFace &msg)
  {
    // TODO: Get extra happy if we _recognize_ the face?
    
    // TODO: Use config file to determine adjustment factors:
    //AdjustEmotion(LONELY, _config[AnkiUtil::kP_EMO_MGR_SEE_FACE_LONELY_ADJUSTMENT]);
    AdjustEmotion(LONELY, 0.5f);
  }
  
  
  void EmotionManager::SetEvolutionFunction(Emotion emotion, EmotionEvolutionFunction fcn)
  {
    _evolutionFunctions[emotion] = fcn;
  }
  
} // namespace Cozmo
} // namespace Anki