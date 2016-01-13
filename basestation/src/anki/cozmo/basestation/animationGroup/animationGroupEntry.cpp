/**
 * File: animationGroupEntry.h
 *
 * Authors: Trevor Dasch
 * Created: 2016-01-11
 *
 * Description:
 *    Class for storing an animation select
 *    Which defines a set of mood animation curves
 *    by which to evaluate the suitability of this animation
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/animationGroup/animationGroupEntry.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "anki/cozmo/basestation/moodSystem/emotionScoreEvaluator.h"
#include "util/logging/logging.h"
//#include <cassert>

#define DEBUG_ANIMATIONS 0

namespace Anki {
  namespace Cozmo {
    
    AnimationGroupEntry::AnimationGroupEntry()
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
      
      if(jsonRoot.isMember("EmotionScorers")) {
        
        Json::Value& jsonEmotionScorers = jsonRoot["EmotionScorers"];
        
        
        const s32 numEntries = jsonEmotionScorers.size();
        for(s32 i = 0; i < numEntries; ++i)
        {
          Json::Value& jsonEmotionScorer = jsonEmotionScorers[i];
          
          _emotionScorers.push_back(EmotionScorer(jsonEmotionScorer));
        } // for each Emotion Scorer
      }
      
      
      return RESULT_OK;
    }
    
    
    float AnimationGroupEntry::Evaluate(const MoodManager& moodManager) const {
      return EmotionScoreEvaluator::EvaluateEmotionScore(_emotionScorers, moodManager);
    }
    
  } // namespace Cozmo
} // namespace Anki
