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
    
    AnimationGroupEntry::AnimationGroupEntry()
    : _moodScorer()
    {
    }
    
    Result AnimationGroupEntry::DefineFromJson(Json::Value &jsonRoot)
    {
      if(!jsonRoot.isMember("Name")) {
        PRINT_NAMED_ERROR("AnimationGroupEntry.DefineFromJson.NoName",
                          "Missing 'Name' field for animation.");
        return RESULT_FAIL;
      }
      
      _name = jsonRoot["Name"].asString();
      
      if(jsonRoot.isMember("MoodScorer")) {
        
        Json::Value& jsonEmotionScorers = jsonRoot["MoodScorer"];
        
        if(!_moodScorer.ReadFromJson(jsonEmotionScorers)) {
          PRINT_NAMED_ERROR("AnimationGroupEntry.DefineFromJson.BadMoodScorer",
                            "'MoodScorer' field contains bad data");
          
          return RESULT_FAIL;
        }
      }
      
      
      return RESULT_OK;
    }
    
    
    float AnimationGroupEntry::EvaluateScore(const MoodManager& moodManager) const {
      return _moodScorer.EvaluateEmotionScore(moodManager);
    }
    
  } // namespace Cozmo
} // namespace Anki
