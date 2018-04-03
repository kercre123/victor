/**
 * File: staticMoodData
 *
 * Author: Mark Wesley
 * Created: 11/16/15
 *
 * Description: Storage for static data (e.g. loaded from files) for the mood system, can be shared amongst multiple
 *              Robots / MoodManagers etc.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_MoodSystem_StaticMoodData_H__
#define __Cozmo_Basestation_MoodSystem_StaticMoodData_H__


#include "engine/moodSystem/emotionEventMapper.h"
#include "clad/types/emotionTypes.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include <map>


namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {
  
  
class EmotionEvent;
  

class StaticMoodData
{
public:

  StaticMoodData();
  
  void  Init(const Json::Value& inJson);
  
  void  InitDecayGraphs();
  bool  SetDecayGraph(EmotionType emotionType, const Util::GraphEvaluator2d& newGraph);
  const Util::GraphEvaluator2d& GetDecayGraph(EmotionType emotionType) const;
  
  static bool VerifyDecayGraph(const Util::GraphEvaluator2d& newGraph, bool warnOnErrors=true);

  void  SetDefaultRepetitionPenalty(const Util::GraphEvaluator2d& newGraph) { _defaultRepetitionPenalty = newGraph; }
  const Util::GraphEvaluator2d& GetDefaultRepetitionPenalty() const { return _defaultRepetitionPenalty; }
  
  const EmotionEventMapper& GetEmotionEventMapper() const { return _emotionEventMapper; }
        EmotionEventMapper& GetEmotionEventMapper()       { return _emotionEventMapper; }

  // maps emotions which don't use default values to the (min, max) range pair
  using EmotionValueRangeMap = std::map< EmotionType, std::pair< float, float > >;
  const EmotionValueRangeMap& GetEmotionValueRangeMap() const { return _emotionValueRangeMap; }

  // ===== Json =====
  
  bool LoadEmotionEvents(const Json::Value& inJson);
  
  bool ReadFromJson(const Json::Value& inJson);
  bool WriteToJson(Json::Value& outJson) const;
  
private:
  
  Util::GraphEvaluator2d _emotionDecayGraphs[(size_t)EmotionType::Count];
  EmotionEventMapper     _emotionEventMapper;
  Util::GraphEvaluator2d _defaultRepetitionPenalty;
  EmotionValueRangeMap   _emotionValueRangeMap;
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_StaticMoodData_H__

