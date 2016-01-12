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
//#include <cassert>
#include <vector>

#define DEBUG_ANIMATIONS 0

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
      
      PRINT_NAMED_ERROR("AnimationGroup.GetAnimation.TMP",
                        "Getting animation from group %s.",
                        _name.c_str());
      
      float max = FLT_MIN;
      std::vector<std::string> results;
      
      
      for(auto it = _animations.begin(); it != _animations.end(); it++) {
        float val = it->Evaluate(moodManager);
        if(fabsf(val - max) < 0.1) {
          results.push_back(it->GetName());
          float invCount = 1.0 / results.size();
          max = val * invCount + max * (1 - invCount);
        }
        else if(val > max) {
          max = val;
          results.clear();
          results.push_back(it->GetName());
        }
      }
      
      return results[rand() % results.size()];
    }
    
  } // namespace Cozmo
} // namespace Anki
