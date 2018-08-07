/**
 * File: moodScorer.cpp
 *
 * Author: Trevor Dasch
 * Created: 01/12/16
 *
 * Description: Rules for scoring a given mood
 * (a set of emotions)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "engine/moodSystem/moodManager.h"
#include "engine/moodSystem/moodScorer.h"
#include "util/logging/logging.h"
#include "json/json.h"
#include "util/math/math.h"


namespace Anki {
  namespace Vector {
    
    MoodScorer::MoodScorer(const Json::Value& inJson)
    : _emotionScorers()
    {
      ReadFromJson(inJson);
    }
    
    
    
    bool MoodScorer::ReadFromJson(const Json::Value& emotionScorersJson)
    {
      ClearEmotionScorers();
      
      if(emotionScorersJson.isNull()) {
        PRINT_NAMED_ERROR("MoodScorer.ReadFromJson.NullJson",
                            "Json is null for MoodScorer");
        return false;
      }
      
      if(!emotionScorersJson.isArray()) {
        PRINT_NAMED_ERROR("MoodScorer.ReadFromJson.JsonNotArray",
                            "Json is not an array for MoodScorer");
        return false;
      }
      
      // read array
      const uint32_t numEmotionScorers = emotionScorersJson.size();
      
      _emotionScorers.reserve(numEmotionScorers);
      
      const Json::Value kNullScorerValue;
      
      for (uint32_t i = 0; i < numEmotionScorers; ++i)
      {
        const Json::Value& emotionScorerJson = emotionScorersJson.get(i, kNullScorerValue);
        
        if (emotionScorerJson.isNull())
        {
          PRINT_NAMED_WARNING("MoodScorer.BadEmotionScorer",
                              "MoodScorer: failed to read emotion scorer %u", i);
        }
        else
        {
          _emotionScorers.emplace_back(emotionScorerJson);
        }
      }
      
      return true;
    }
    
    
    bool MoodScorer::WriteToJson(Json::Value& outJson) const
    {
      outJson = Json::Value(Json::ValueType::arrayValue);
      
      // make array
      
      for (auto it = _emotionScorers.begin(); it != _emotionScorers.end(); it++) {
        Json::Value entry;
        if(it->WriteToJson(entry)) {
          outJson.append(entry);
        }
      }
      
      return true;
    }
    
    float MoodScorer::EvaluateEmotionScore(const MoodManager& moodManager) const
    {
      float totalScore = 0.0f;
      uint32_t numEmotionsScored = 0;
      const uint32_t kTickRangeForDelta = 60; // 2 would give any chances between the last 2 updates, but we might want to track more if it takes longer for a running behavior to interrupt and allow this to run
      
      for (const EmotionScorer& emotionScorer : _emotionScorers)
      {
        const EmotionType emotionType = emotionScorer.GetEmotionType();
        const float emotionValue = emotionScorer.TrackDeltaScore() ? moodManager.GetEmotionDeltaRecentTicks(emotionType, kTickRangeForDelta)
        : moodManager.GetEmotionValue(emotionType);
        const float score = emotionScorer.GetScoreGraph().EvaluateY(emotionValue);
        if (FLT_NEAR(score, 0.0f))
        {
          // any component scoring 0 forces an overal 0 score (e.g. prevent behavior ever being picked)
          return 0.0f;
        }
        else
        {
          totalScore += score;
          ++numEmotionsScored;
        }
      }
      
      const float averageScore = (numEmotionsScored > 0) ? (totalScore / float(numEmotionsScored)) : 0.0f;
      return averageScore;
    }

    
  } // namespace Vector
} // namespace Anki

