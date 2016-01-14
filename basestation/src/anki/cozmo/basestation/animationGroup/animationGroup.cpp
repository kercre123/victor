/**
 * File: animationGroup.cpp
 *
 * Authors: Trevor Dasch
 * Created: 2016-01-11
 *
 * Description:
 *    Class for storing a group of animations,
 *    from which an animation can be selected
 *    for a given set of moods
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/animationGroup/animationGroup.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "util/logging/logging.h"
#include "util/random/randomGenerator.h"
#include "anki/common/basestation/jsonTools.h"

namespace Anki {
  namespace Cozmo {
    
    static const char* kAnimationsKeyName = "Animations";
    
    AnimationGroup::AnimationGroup(const std::string& name)
    : _name(name)
    {
      
    }
    
    Result AnimationGroup::DefineFromJson(const std::string& name, const Json::Value &jsonRoot)
    {
      _name = name;
      
      const Json::Value& jsonAnimations = jsonRoot[kAnimationsKeyName];

      if(!jsonAnimations.isArray()) {
        
        PRINT_NAMED_ERROR("AnimationGroup.DefineFromJson.NoAnimations",
                          "Missing '%s' field for animation group.", kAnimationsKeyName);
        return RESULT_FAIL;
      }

      _animations.clear();
      
      const s32 numEntries = jsonAnimations.size();
      
      _animations.reserve(numEntries);
      
      for(s32 iEntry = 0; iEntry < numEntries; ++iEntry)
      {
        const Json::Value& jsonEntry = jsonAnimations[iEntry];
        
        _animations.emplace_back();
        
        auto addResult = _animations.back().DefineFromJson(jsonEntry);
        
        if(addResult != RESULT_OK) {
          PRINT_NAMED_ERROR("AnimationGroup.DefineFromJson.AddEntryFailure",
                            "Adding animation %d failed.",
                            iEntry);
          return addResult;
        }
        
        
      } // for each Entry
      
      return RESULT_OK;
    }
    
    bool AnimationGroup::IsEmpty() const
    {
      return _animations.empty();
    }
    
    const std::string& AnimationGroup::GetAnimationName(const MoodManager& moodManager) const {
      const float kRandomFactor = 0.1f;
      
      if(IsEmpty()) {
        static const std::string empty = "";
        return empty;
      }
      
      Util::RandomGenerator rng; // [MarkW:TODO] We should share these (1 per robot or subsystem maybe?) for replay determinism
      
      auto bestEntry = _animations.end();
      float bestScore = -FLT_MAX;
      for (auto entry = _animations.begin(); entry != _animations.end(); entry++)
      {
        auto entryScore = entry->EvaluateScore(moodManager);
        auto totalScore    = entryScore;
        
        totalScore += rng.RandDbl(kRandomFactor);
        
        if (totalScore > bestScore)
        {
          bestEntry = entry;
          bestScore = totalScore;
        }
      }
      
      return bestEntry->GetName();
    }
    
  } // namespace Cozmo
} // namespace Anki
