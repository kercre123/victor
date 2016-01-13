/**
 * File: animationGroupEntry.cpp
 *
 * Authors: Trevor Dasch
 * Created: 2016-01-11
 *
 * Description:
 *    Class for storing an animation selection
 *    Which defines a set of mood score graphs
 *    by which to evaluate the suitability of this animation
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/animationGroup/animationGroupEntry.h"
#include "util/logging/logging.h"
#include "anki/common/basestation/jsonTools.h"

namespace Anki {
  namespace Cozmo {
    
    static const char* kNameKey = "Name";
    static const char* kMoodScorerKey = "MoodScorer";
    
    AnimationGroupEntry::AnimationGroupEntry()
    : _moodScorer()
    {
    }
    
    Result AnimationGroupEntry::DefineFromJson(const Json::Value &jsonRoot)
    {
      const Json::Value& jsonName = jsonRoot[kNameKey];
      
      if(jsonName.isNull()) {
        PRINT_NAMED_ERROR("AnimationGroupEntry.DefineFromJson.NoName",
                          "Missing '%s' field for animation.", kNameKey);
        return RESULT_FAIL;
      }
      
      if(!jsonName.isString()) {
        PRINT_NAMED_ERROR("AnimationGroupEntry.DefineFromJson.NameNotString",
                          "'%s' field is not a string.", kNameKey);
        return RESULT_FAIL;
      }
      
      _name = jsonName.asString();
      
      const Json::Value& jsonEmotionScorers = jsonRoot[kMoodScorerKey];
      
      _moodScorer.ClearEmotionScorers();
      
      if(!_moodScorer.ReadFromJson(jsonEmotionScorers)) {
        PRINT_NAMED_ERROR("AnimationGroupEntry.DefineFromJson.BadMoodScorer",
                          "'%s' field contains bad data", kMoodScorerKey);
        
        return RESULT_FAIL;
      }
      
      return RESULT_OK;
    }
    
    
    float AnimationGroupEntry::EvaluateScore(const MoodManager& moodManager) const {
      return _moodScorer.EvaluateEmotionScore(moodManager);
    }
    
  } // namespace Cozmo
} // namespace Anki
