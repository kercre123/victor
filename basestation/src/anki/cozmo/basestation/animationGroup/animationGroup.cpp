/**
 * File: animationGroup.h
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
//#include <cassert>
#include <vector>

namespace Anki {
  namespace Cozmo {
    
    AnimationGroup::AnimationGroup(const std::string& name)
    : _name(name)
    {
      
    }
    
    Result AnimationGroup::DefineFromJson(const std::string& name, Json::Value &jsonRoot)
    {
      /*
       if(!jsonRoot.isMember("Name")) {
       PRINT_NAMED_ERROR("Animation.DefineFromJson.NoName",
       "Missing 'Name' field for animation.");
       return RESULT_FAIL;
       }
       */
      
      _name = name; //jsonRoot["Name"].asString();
      
      if(!jsonRoot.isMember("Animations")) {
        
        PRINT_NAMED_ERROR("AnimationGroup.DefineFromJson.NoAnimations",
                          "Missing 'Animations' field for animation group.");
      }
      
      Json::Value& jsonAnimations = jsonRoot["Animations"];
      
      
      const s32 numEntries = jsonAnimations.size();
      for(s32 iEntry = 0; iEntry < numEntries; ++iEntry)
      {
        Json::Value& jsonEntry = jsonAnimations[iEntry];
        
        _animations.push_back(AnimationGroupEntry());
        
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
    
    const std::string& AnimationGroup::GetAnimation(const MoodManager& moodManager) const {
      const float kRandomFactor = 0.1f;
      
      Util::RandomGenerator rng; // [MarkW:TODO] We should share these (1 per robot or subsystem maybe?) for replay determinism
      
      auto bestEntry = _animations.end();
      float bestScore = FLT_MIN;
      for (auto entry = _animations.begin(); entry != _animations.end(); entry++)
      {
        auto entryScore = entry->Evaluate(moodManager);
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
