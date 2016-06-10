/**
 * File: connectionStats
 *
 * Author: Mark Wesley
 * Created: 06/02/16
 *
 * Description: Stats for timings etc. on all connections
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "util/transport/connectionStats.h"
#include "util/console/consoleInterface.h"


namespace Anki {
namespace Util {
  

#if ENABLE_RELIABLE_CONNECTION_STATS
  const char* kNetworkStatsSection = "Network.Stats";
  CONSOLE_VAR(bool,   kNetConnStatsUpdate,      kNetworkStatsSection, true);
  // Stats are written into console vars so they can be live viewed from console menu
  CONSOLE_VAR(int,    gNetStat1NumConnections,  kNetworkStatsSection, 0);
  CONSOLE_VAR(float,  gNetStat2LatencyAvg,      kNetworkStatsSection, 0.0f);
  CONSOLE_VAR(float,  gNetStat3LatencySD,       kNetworkStatsSection, 0.0f);
  CONSOLE_VAR(float,  gNetStat4LatencyMin,      kNetworkStatsSection, 0.0f);
  CONSOLE_VAR(float,  gNetStat5LatencyMax,      kNetworkStatsSection, 0.0f);
  CONSOLE_VAR(float,  gNetStat6PingArrivedPC,   kNetworkStatsSection, 0.0f);
  CONSOLE_VAR(float,  gNetStat7ExtQueuedAvg_ms, kNetworkStatsSection, 0.0f);
  CONSOLE_VAR(float,  gNetStat8ExtQueuedMin_ms, kNetworkStatsSection, 0.0f);
  CONSOLE_VAR(float,  gNetStat9ExtQueuedMax_ms, kNetworkStatsSection, 0.0f);
  CONSOLE_VAR(float,  gNetStatAQueuedAvg_ms,    kNetworkStatsSection, 0.0f);
  CONSOLE_VAR(float,  gNetStatBQueuedMin_ms,    kNetworkStatsSection, 0.0f);
  CONSOLE_VAR(float,  gNetStatCQueuedMax_ms,    kNetworkStatsSection, 0.0f);
#endif // REMOTE_CONSOLE_ENABLED


} // end namespace Util
} // end namespace Anki
