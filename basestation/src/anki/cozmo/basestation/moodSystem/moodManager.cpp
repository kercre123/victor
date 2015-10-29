/**
 * File: moodManager
 *
 * Author: Mark Wesley
 * Created: 10/14/15
 *
 * Description: Manages the Mood (a selection of emotions) for a Cozmo Robot
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include <assert.h>


namespace Anki {
namespace Cozmo {
  
  
Anki::Util::GraphEvaluator2d MoodManager::sEmotionDecayGraphs[(size_t)EmotionType::Count];

  
MoodManager::MoodManager()
  : _lastUpdateTime(0.0)
{
  InitDecayGraphs();
}

  
void MoodManager::Reset()
{
  for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
  {
    GetEmotionByIndex(i).Reset();
  }
  _lastUpdateTime = 0.0;
}
  
  
bool VerifyDecayGraph(const Anki::Util::GraphEvaluator2d& newGraph)
{
  const size_t numNodes = newGraph.GetNumNodes();
  if (numNodes == 0)
  {
    PRINT_NAMED_ERROR("VerifyDecayGraph.NoNodes", "Invalid graph has 0 nodes");
    return false;
  }
  
  bool isValid = true;
  
  float lastY = 0.0f;
  for (size_t i=0; i < numNodes; ++i)
  {
    const Anki::Util::GraphEvaluator2d::Node& node = newGraph.GetNode(i);

    if (node._y < 0.0f)
    {
      PRINT_NAMED_ERROR("VerifyDecayGraph.NegativeYNode", "Node[%zu] = (%f, %f) is Negative!", i, node._x, node._y);
      isValid = false;
    }
    if ((i != 0) && (node._y > lastY))
    {
      PRINT_NAMED_ERROR("VerifyDecayGraph.IncreasingYNode", "Node[%zu] = (%f, %f) is has y > than previous (%f)", i, node._x, node._y, lastY);
      isValid = false;
    }
    
    lastY = node._y;
  }
  
  return isValid;
}
  

void MoodManager::SetDecayGraph(EmotionType emotionType, const Anki::Util::GraphEvaluator2d& newGraph)
{
  const size_t index = (size_t)emotionType;
  assert((index >= 0) && (index < (size_t)EmotionType::Count));
  
  if (!VerifyDecayGraph(newGraph))
  {
    PRINT_NAMED_ERROR("MoodManager.SetDecayGraph.Invalid", "Invalid graph for emotion '%s'", EnumToString(emotionType));
  }
  else
  {
    sEmotionDecayGraphs[index] = newGraph;
  }
}


// Hard coded static init for now, will replace with data loading eventually
void MoodManager::InitDecayGraphs()
{
  for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
  {
    Anki::Util::GraphEvaluator2d& decayGraph = sEmotionDecayGraphs[i];
    if (decayGraph.GetNumNodes() == 0)
    {
      decayGraph.AddNode( 0.0f, 1.0f);
      decayGraph.AddNode( 1.0f, 1.0f);
      decayGraph.AddNode(10.0f, 0.0f);
    }
    assert(VerifyDecayGraph(decayGraph));
  }
}
  
  
const Anki::Util::GraphEvaluator2d& MoodManager::GetDecayGraph(EmotionType emotionType)
{
  const size_t index = (size_t)emotionType;
  assert((index >= 0) && (index < (size_t)EmotionType::Count));
  
  Anki::Util::GraphEvaluator2d& decayGraph = sEmotionDecayGraphs[index];
  assert(decayGraph.GetNumNodes() > 0);
  
  return decayGraph;
}


void MoodManager::Update(double currentTime)
{
  const float kMinTimeStep = 0.01f;//(1.0f / 60.0f); // minimal sensible timestep, should be at least > epsilon
  float timeDelta = (_lastUpdateTime != 0.0) ? float(currentTime - _lastUpdateTime) : kMinTimeStep;
  if (timeDelta < kMinTimeStep)
  {
    PRINT_NAMED_ERROR("MoodManager.BadTimeStep", "TimeStep %f (%f-%f) is < %f - clamping!", timeDelta, currentTime, _lastUpdateTime, kMinTimeStep);
    //assert(0); // TEMP
    timeDelta = kMinTimeStep;
  }
  
  _lastUpdateTime = currentTime;
  
  for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
  {
    EmotionType emotionType = (EmotionType)i;
    GetEmotionByIndex(i).Update(GetDecayGraph(emotionType), currentTime, timeDelta);
  }
}
  
  
void MoodManager::AddToEmotion(EmotionType emotionType, float baseValue, const char* uniqueIdString)
{
  GetEmotion(emotionType).Add(baseValue, uniqueIdString);
}


} // namespace Cozmo
} // namespace Anki

