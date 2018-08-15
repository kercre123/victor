/**
 * File: moodScorer.h
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


#ifndef __Cozmo_Basestation_MoodSystem_MoodScorer_H__
#define __Cozmo_Basestation_MoodSystem_MoodScorer_H__

#include "engine/moodSystem/emotionScorer.h"
#include <vector>

namespace Json {
  class Value;
}

namespace Anki {
  namespace Vector {
    
    //Forward declaration
    class MoodManager;
    
    
    class MoodScorer
    {
    public:
      
      MoodScorer()
      : _emotionScorers()
      {
      }
      
      MoodScorer(const std::vector<EmotionScorer>& emotionScorers)
      : _emotionScorers(emotionScorers)
      {
      }
      
      explicit MoodScorer(const Json::Value& inJson);
      
      // ensure noexcept default move and copy assignment-operators/constructors are created
      MoodScorer(const MoodScorer& other) = default;
      MoodScorer& operator=(const MoodScorer& other) = default;
      MoodScorer(MoodScorer&& rhs) noexcept = default;
      MoodScorer& operator=(MoodScorer&& rhs) noexcept = default;
      
      // ===== Json =====
      
      bool ReadFromJson(const Json::Value& inJson);
      bool WriteToJson(Json::Value& outJson) const;
      
      // return true if no scorers are currently set
      bool IsEmpty() const { return _emotionScorers.empty(); }
      
      void ClearEmotionScorers()                         { _emotionScorers.clear(); }
      void AddEmotionScorer(const EmotionScorer& scorer) { _emotionScorers.push_back(scorer); }
      size_t GetEmotionScorerCount() const { return _emotionScorers.size(); }
      const EmotionScorer& GetEmotionScorer(size_t index) const { return _emotionScorers[index]; }
      
      float EvaluateEmotionScore(const MoodManager& moodManager) const;
      
    private:
      
      std::vector<EmotionScorer> _emotionScorers;
    };
    
    
  } // namespace Vector
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_MoodScorer_H__

