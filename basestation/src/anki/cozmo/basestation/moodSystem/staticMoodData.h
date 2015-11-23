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


#include "anki/cozmo/basestation/moodSystem/emotionEventMapper.h"
#include "clad/types/emotionTypes.h"
#include "util/graphEvaluator/graphEvaluator2d.h"


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
  bool  SetDecayGraph(EmotionType emotionType, const Anki::Util::GraphEvaluator2d& newGraph);
  const Anki::Util::GraphEvaluator2d& GetDecayGraph(EmotionType emotionType);
  
  static bool VerifyDecayGraph(const Anki::Util::GraphEvaluator2d& newGraph, bool warnOnErrors=true);

  const EmotionEventMapper& GetEmotionEventMapper() const { return _emotionEventMapper; }
        EmotionEventMapper& GetEmotionEventMapper()       { return _emotionEventMapper; }
  
  // ===== Json =====
  
  bool ReadFromJson(const Json::Value& inJson);
  bool WriteToJson(Json::Value& outJson) const;
  
private:
  
  Anki::Util::GraphEvaluator2d   _emotionDecayGraphs[(size_t)EmotionType::Count];
  EmotionEventMapper             _emotionEventMapper;
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_StaticMoodData_H__

