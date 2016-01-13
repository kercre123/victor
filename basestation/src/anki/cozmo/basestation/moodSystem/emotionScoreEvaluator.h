/**
 * File: emotionScoreEvaluator.h
 *
 * Author: Trevor Dasch
 * Created: 2016-01-12
 *
 * Description: Evaluate the score for
 *  A set of emotion scorers given
 *  the state of the mood mananger
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef EMOTION_SCORE_EVALUATOR_H
#define EMOTION_SCORE_EVALUATOR_H

namespace Anki {
  namespace Cozmo {

    class EmotionScoreEvaluator {

    public:
      static inline float EvaluateEmotionScore(const std::vector<EmotionScorer>& emotionScorers, const MoodManager& moodManager)
      {
        float totalScore = 0.0f;
        uint32_t numEmotionsScored = 0;
        const uint32_t kTickRangeForDelta = 60; // 2 would give any chances between the last 2 updates, but we might want to track more if it takes longer for a running behavior to interrupt and allow this to run
        
        for (const EmotionScorer& emotionScorer : emotionScorers)
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

    };
  }
}

#endif /* EMOTION_SCORE_EVALUATOR_H */
