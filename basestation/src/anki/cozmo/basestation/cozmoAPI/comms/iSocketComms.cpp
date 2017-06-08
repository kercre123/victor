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


#include "anki/cozmo/basestation/cozmoAPI/comms/iSocketComms.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kPrintUiMessageLatency, "UiComms", false);
  
const uint32_t kMaxLatencySamples = 20;
const uint32_t kReportFrequency = 10;

ISocketComms::ISocketComms(bool isEnabled)
  : _latencyStats(kMaxLatencySamples)
  , _pingCounter(0)
  , _isEnabled(isEnabled)
{
}

  
ISocketComms::~ISocketComms()
{
}


void ISocketComms::HandlePingResponse(const ExternalInterface::Ping& pingMsg)
{
  const double latency_ms = (Util::Time::UniversalTime::GetCurrentTimeInMilliseconds() - pingMsg.timeSent_ms);
  _latencyStats.AddStat(latency_ms);
  if (kPrintUiMessageLatency)
  {
    uint32_t numSamples = _latencyStats.GetNumDbl();
    if (numSamples && ((numSamples % kReportFrequency) == 0))
    {
      const double averageLatency = _latencyStats.GetMean();
      const double stdDevLatency  = _latencyStats.GetStd();
    
      const float minLatency = (numSamples > 0) ? _latencyStats.GetMin() : 0.0f;
      const float maxLatency = (numSamples > 0) ? _latencyStats.GetMax() : 0.0f;
    
      PRINT_CH_INFO("UiComms", "UiMessageLatency", "%.2f ms, [%.2f..%.2f], SD= %.2f, %u samples",
                           averageLatency, minLatency, maxLatency, stdDevLatency, numSamples);
    }
  }
}

  
} // namespace Cozmo
} // namespace Anki

