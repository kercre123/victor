/**
 * File: progressionManager
 *
 * Author: Mark Wesley
 * Created: 11/10/15
 *
 * Description: Manages the Progression (ratings for a selection of stats) for a Cozmo Robot
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "anki/cozmo/basestation/progressionSystem/progressionManager.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

  
ProgressionManager::ProgressionManager(Robot* inRobot)
  : _robot(inRobot)
{  
}
  
  
void ProgressionManager::Reset()
{
  for (size_t i = 0; i < (size_t)ProgressionStatType::Count; ++i)
  {
    GetStatByIndex(i).Reset();
  }
}


void ProgressionManager::Update(double currentTime)
{
  for (size_t i = 0; i < (size_t)ProgressionStatType::Count; ++i)
  {
    ProgressionStat& stat = GetStatByIndex(i); 
    stat.Update(currentTime);
  }
  
  SendStatsToGame();
}


void ProgressionManager::HandleEvent(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  const auto& eventData = event.GetData();
  
  switch (eventData.GetTag())
  {
    case ExternalInterface::MessageGameToEngineTag::ProgressionMessage:
    {
      const Anki::Cozmo::ExternalInterface::ProgressionMessageUnion& progressionMessage = eventData.Get_ProgressionMessage().ProgressionMessageUnion;
      
      switch (progressionMessage.GetTag())
      {
        case ExternalInterface::ProgressionMessageUnionTag::GetProgressionStats:
          SendStatsToGame();
          break;
        case ExternalInterface::ProgressionMessageUnionTag::SetProgressionStat:
        {
          const Anki::Cozmo::ExternalInterface::SetProgressionStat& msg = progressionMessage.Get_SetProgressionStat();
          GetStat(msg.statType).SetValue(msg.newVal);
          break;
        }
        case ExternalInterface::ProgressionMessageUnionTag::AddToProgressionStat:
        {
          const Anki::Cozmo::ExternalInterface::AddToProgressionStat& msg = progressionMessage.Get_AddToProgressionStat();
          GetStat(msg.statType).SetValue(msg.deltaVal);
          break;
        }
        default:
          PRINT_NAMED_ERROR("ProgressionManager.HandleEvent.UnhandledMessageUnionTag", "Unexpected tag %u", (uint32_t)progressionMessage.GetTag());
          assert(0);
      }
    }
      break;
    default:
      PRINT_NAMED_ERROR("ProgressionManager.HandleEvent.UnhandledMessageGameToEngineTag", "Unexpected tag %u", (uint32_t)eventData.GetTag());
      assert(0);
  }
}
  
  
void ProgressionManager::SendStatsToGame() const
{
  if (_robot)
  {
    std::vector<ProgressionStat::ValueType> statValues;
    statValues.reserve((size_t)ProgressionStatType::Count);
    
    for (size_t i = 0; i < (size_t)ProgressionStatType::Count; ++i)
    {
      const ProgressionStat& stat = GetStatByIndex(i);
      statValues.push_back(stat.GetValue());
    }

    ExternalInterface::ProgressionStats message(_robot->GetID(), std::move(statValues));
    _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
  }
}


} // namespace Cozmo
} // namespace Anki

