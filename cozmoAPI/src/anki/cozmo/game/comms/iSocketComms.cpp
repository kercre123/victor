/**
 * File: iSocketComms
 *
 * Author: Mark Wesley
 * Created: 05/14/16
 *
 * Description: Interface for any socket-based communications from e.g. Game/SDK to Engine
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "anki/cozmo/game/comms/iSocketComms.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kPrintUiMessageLatency, "GameToEngineConnection", false);
  
const uint32_t kMaxLatencySamples = 2500;
const uint32_t kReportFrequency = 250;

ISocketComms::ISocketComms(UiConnectionType connectionType)
  : _latencyStats(kMaxLatencySamples)
  , _pingCounter(0)
{
}

void ISocketComms::HandlePingResponse(const ExternalInterface::MessageGameToEngine& message)
{
  const double latency = (Util::Time::UniversalTime::GetCurrentTimeInSeconds() - message.Get_Ping().timeSent);
  _latencyStats.AddStat(latency);
  if (kPrintUiMessageLatency)
  {
    uint32_t numSamples = _latencyStats.GetNumDbl();
    if (numSamples && ((numSamples % kReportFrequency) == 0))
    {
      const double averageLatency = _latencyStats.GetMean();
      const double stdDevLatency  = _latencyStats.GetStd();
    
      const float minLatency = (numSamples > 0) ? _latencyStats.GetMin() : 0.0f;
      const float maxLatency = (numSamples > 0) ? _latencyStats.GetMax() : 0.0f;
    
      PRINT_CHANNELED_INFO("Network", "UiMessageHandler.Latency", "Latency Data (sec): Samples= %u Average= %.5f Sd= %.5f Min= %.5f Max %.5f",
                           numSamples, averageLatency, stdDevLatency, minLatency, maxLatency);
    }
  }
}


ISocketComms::~ISocketComms()
{
}

  
} // namespace Cozmo
} // namespace Anki

