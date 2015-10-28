/**
 * File: emotionScorer
 *
 * Author: Mark Wesley
 * Created: 10/21/15
 *
 * Description: Rules for scoring a given emotion
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_MoodSystem_EmotionScorer_H__
#define __Cozmo_Basestation_MoodSystem_EmotionScorer_H__


#include "clad/types/emotionTypes.h"
#include "util/graphEvaluator/graphEvaluator2d.h"


namespace Anki {
namespace Cozmo {


class EmotionScorer
{
public:
  
  EmotionScorer(EmotionType type, const Util::GraphEvaluator2d& graph, bool trackDelta)
    : _emotionType(type)
    , _scoreGraph(graph)
    , _trackDeltaScore(trackDelta)
  {  
  }
  
  EmotionType             GetEmotionType() const  { return _emotionType; }
  Util::GraphEvaluator2d  GetScoreGraph()  const  { return _scoreGraph; }
  bool                    TrackDeltaScore() const { return _trackDeltaScore; }
  
private:
  
  EmotionType             _emotionType;
  Util::GraphEvaluator2d  _scoreGraph;
  bool                    _trackDeltaScore;
};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_EmotionScorer_H__

