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
#include "anki/cozmo/basestation/animationGroup/animationGroupContainer.h"
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
    
    const std::string& AnimationGroup::GetAnimationName(const MoodManager& moodManager, AnimationGroupContainer& animationGroupContainer) const {
      return GetAnimationName(moodManager.GetSimpleMood(), moodManager.GetLastUpdateTime(), animationGroupContainer);
    }
    
    const std::string& AnimationGroup::GetAnimationName(SimpleMoodType mood, float currentTime_s, AnimationGroupContainer& animationGroupContainer) const {
      
      Util::RandomGenerator rng; // [MarkW:TODO] We should share these (1 per robot or subsystem maybe?) for replay determinism
      
      float totalWeight = 0.0f;
      
      std::vector<const AnimationGroupEntry*> availableAnimations;
      
      for (auto entry = _animations.begin(); entry != _animations.end(); entry++)
      {
        if(entry->GetMood() == mood && !animationGroupContainer.IsAnimationOnCooldown(entry->GetName(),currentTime_s))
        {
          totalWeight += entry->GetWeight();
          availableAnimations.emplace_back(&(*entry));
        }
      }
      
      float weightedSelection = rng.RandDbl(totalWeight);
      
      const AnimationGroupEntry* lastEntry = nullptr;
      
      for (auto entry : availableAnimations)
      {
        if(entry->GetMood() == mood) {
          
          if(weightedSelection < entry->GetWeight()) {
            animationGroupContainer.SetAnimationCooldown(entry->GetName(), currentTime_s + entry->GetCooldown());
            return entry->GetName();
          }
          else {
            lastEntry = &(*entry);
            weightedSelection -= entry->GetWeight();
          }
        }
      }
      
      // Possible that if weightedSelection == totalWeight, we wouldn't
      // select any, so return the last one if its not the end
      if(lastEntry != nullptr) {
        animationGroupContainer.SetAnimationCooldown(lastEntry->GetName(), currentTime_s + lastEntry->GetCooldown());
        return lastEntry->GetName();
      }
      
      if(mood == SimpleMoodType::Default) {
        static const std::string empty = "";
        return empty;
      }
      else {
        return GetAnimationName(SimpleMoodType::Default, currentTime_s, animationGroupContainer);
      }
    }
    
  } // namespace Cozmo
} // namespace Anki
