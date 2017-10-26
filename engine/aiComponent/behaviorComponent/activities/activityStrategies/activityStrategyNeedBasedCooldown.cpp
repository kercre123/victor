/**
 * File: ActivityStrategySinging.cpp
 *
 * Author: Al Chaussee
 * Created: 07/17/2017
 *
 * Description: Activity strategy for singing. Makes Cozmo more likely to sing when his Play need is high
 *              by lowering the strategy cooldown
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/activities/activityStrategies/activityStrategyNeedBasedCooldown.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/ankiEventUtil.h"
#include "engine/cozmoContext.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robot.h"

#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace {
static const char* kNeedIdKey                      = "needId";
static const char* kNeedCooldownGraphKey           = "needCooldownGraph";
static const char* kNeedCooldownRandomnessGraphKey = "needCooldownRandomnessGraph";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrategyNeedBasedCooldown::ActivityStrategyNeedBasedCooldown(BehaviorExternalInterface& behaviorExternalInterface,
                                                                     IExternalInterface* robotExternalInterface,
                                                                     const Json::Value& config)
: IActivityStrategy(behaviorExternalInterface, robotExternalInterface, config)
{

  if(robotExternalInterface != nullptr)
  {
    auto helper = MakeAnkiEventUtil(*robotExternalInterface, *this, _eventHandles);
    using namespace ExternalInterface;
    helper.SubscribeEngineToGame<MessageEngineToGameTag::NeedsState>();
  }
  
  std::string needId = "";
  const bool res = JsonTools::GetValueOptional(config, kNeedIdKey, needId);
  DEV_ASSERT(res, "ActivityStrategySinging.MissingNeedIdKey");
  _needId = NeedIdFromString(needId);

  // Populate the need to cooldown graph
  _needCooldownGraph.Clear();
  
  const Json::Value& needCooldownScorer = config[kNeedCooldownGraphKey];
  if(!needCooldownScorer.isNull())
  {
    _needCooldownGraph.ReadFromJson(needCooldownScorer);
  }
  else
  {
    _needCooldownGraph.AddNode(0, GetBaseCooldownSecs());
  }
  
  // Populate the need to cooldownRandomness graph
  _needCooldownRandomnessGraph.Clear();
  
  const Json::Value& needCooldownRandomnessScorer = config[kNeedCooldownRandomnessGraphKey];
  if(!needCooldownRandomnessScorer.isNull())
  {
    _needCooldownRandomnessGraph.ReadFromJson(needCooldownRandomnessScorer);
  }
  else
  {
    _needCooldownRandomnessGraph.AddNode(0, GetCooldownRandomnessSecs());
  }
}

template<>
void ActivityStrategyNeedBasedCooldown::HandleMessage(const ExternalInterface::NeedsState& msg)
{
  const float playNeedLevel = msg.curNeedLevel[static_cast<int>(_needId)];
  const float cooldown = _needCooldownGraph.EvaluateY(playNeedLevel);
  const float cooldownRandomness = _needCooldownRandomnessGraph.EvaluateY(playNeedLevel);
  
  SetCooldown(cooldown, cooldownRandomness);
}

} // namespace
} // namespace
