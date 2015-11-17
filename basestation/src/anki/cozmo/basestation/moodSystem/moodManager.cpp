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
#include "util/math/math.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/vizInterface/messageViz.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/logging/logging.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include <assert.h>


namespace Anki {
namespace Cozmo {
  
  
Anki::Util::GraphEvaluator2d MoodManager::sEmotionDecayGraphs[(size_t)EmotionType::Count];

  
MoodManager::MoodManager(Robot* inRobot)
  : _robot(inRobot)
  , _lastUpdateTime(0.0)
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
      decayGraph.AddNode(  0.0f, 1.0f);
      decayGraph.AddNode( 15.0f, 1.0f);
      decayGraph.AddNode( 60.0f, 0.9f);
      decayGraph.AddNode(150.0f, 0.6f);
      decayGraph.AddNode(300.0f, 0.0f);
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
  const float kMinTimeStep = 0.0001f; // minimal sensible timestep, should be at least > epsilon
  float timeDelta = (_lastUpdateTime != 0.0) ? float(currentTime - _lastUpdateTime) : kMinTimeStep;
  if (timeDelta < kMinTimeStep)
  {
    PRINT_NAMED_WARNING("MoodManager.BadTimeStep", "TimeStep %f (%f-%f) is < %f - clamping!", timeDelta, currentTime, _lastUpdateTime, kMinTimeStep);
    timeDelta = kMinTimeStep;
  }
  
  _lastUpdateTime = currentTime;

  SEND_MOOD_TO_VIZ_DEBUG_ONLY( VizInterface::RobotMood robotMood );
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( robotMood.emotion.reserve((size_t)EmotionType::Count) );

  for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
  {
    const EmotionType emotionType = (EmotionType)i;
    Emotion& emotion = GetEmotionByIndex(i);
    
    emotion.Update(GetDecayGraph(emotionType), currentTime, timeDelta);

    SEND_MOOD_TO_VIZ_DEBUG_ONLY( robotMood.emotion.push_back(emotion.GetValue()) );
  }
  
  SendEmotionsToGame();

  #if SEND_MOOD_TO_VIZ_DEBUG
  robotMood.recentEvents = std::move(_eventNames);
  _eventNames.clear();
  VizManager::getInstance()->SendRobotMood(std::move(robotMood));
  #endif //SEND_MOOD_TO_VIZ_DEBUG
}
  
  
void MoodManager::HandleEvent(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  const auto& eventData = event.GetData();
  
  switch (eventData.GetTag())
  {
    case ExternalInterface::MessageGameToEngineTag::MoodMessage:
      {
        const Anki::Cozmo::ExternalInterface::MoodMessageUnion& moodMessage = eventData.Get_MoodMessage().MoodMessageUnion;

        switch (moodMessage.GetTag())
        {
          case ExternalInterface::MoodMessageUnionTag::GetEmotions:
            SendEmotionsToGame();
            break;
          case ExternalInterface::MoodMessageUnionTag::SetEmotion:
          {
            const Anki::Cozmo::ExternalInterface::SetEmotion& msg = moodMessage.Get_SetEmotion();
            SetEmotion(msg.emotionType, msg.newVal);
            break;
          }
          case ExternalInterface::MoodMessageUnionTag::AddToEmotion:
          {
            const Anki::Cozmo::ExternalInterface::AddToEmotion& msg = moodMessage.Get_AddToEmotion();
            AddToEmotion(msg.emotionType, msg.deltaVal, msg.uniqueIdString.c_str());
            break;
          }
          default:
            PRINT_NAMED_ERROR("MoodManager.HandleEvent.UnhandledMessageUnionTag", "Unexpected tag %u", (uint32_t)moodMessage.GetTag());
            assert(0);
        }
      }
      break;
    default:
      PRINT_NAMED_ERROR("MoodManager.HandleEvent.UnhandledMessageGameToEngineTag", "Unexpected tag %u", (uint32_t)eventData.GetTag());
      assert(0);
  }
}
  
  
void MoodManager::SendEmotionsToGame()
{
  if (_robot)
  {
    std::vector<float> emotionValues;
    emotionValues.reserve((size_t)EmotionType::Count);
    
    for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
    {
      const Emotion& emotion = GetEmotionByIndex(i);
      emotionValues.push_back(emotion.GetValue());
    }
    
    ExternalInterface::MoodState message(_robot->GetID(), std::move(emotionValues));
    _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
  }
}
  
  
void MoodManager::AddToEmotion(EmotionType emotionType, float baseValue, const char* uniqueIdString)
{
  GetEmotion(emotionType).Add(baseValue, uniqueIdString);
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( AddEvent(uniqueIdString) );
}
  

void MoodManager::AddToEmotions(EmotionType emotionType1, float baseValue1,
                                EmotionType emotionType2, float baseValue2, const char* uniqueIdString)
{
  AddToEmotion(emotionType1, baseValue1, uniqueIdString);
  AddToEmotion(emotionType2, baseValue2, uniqueIdString);
}

  
void MoodManager::AddToEmotions(EmotionType emotionType1, float baseValue1,
                                EmotionType emotionType2, float baseValue2,
                                EmotionType emotionType3, float baseValue3, const char* uniqueIdString)
{
  AddToEmotion(emotionType1, baseValue1, uniqueIdString);
  AddToEmotion(emotionType2, baseValue2, uniqueIdString);
  AddToEmotion(emotionType3, baseValue3, uniqueIdString);
}


void MoodManager::SetEmotion(EmotionType emotionType, float value)
{
  GetEmotion(emotionType).SetValue(value);
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( AddEvent("SetEmotion") );
}

  
#if SEND_MOOD_TO_VIZ_DEBUG
void MoodManager::AddEvent(const char* eventName)
{
  if (_eventNames.empty() || (_eventNames.back() != eventName))
  {
    _eventNames.push_back(eventName);
  }
}
#endif // SEND_MOOD_TO_VIZ_DEBUG

  
} // namespace Cozmo
} // namespace Anki

