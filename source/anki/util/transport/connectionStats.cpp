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

const char* kNetworkStatsSection = "Network.Stats";

CONSOLE_VAR(bool, kNetConnStatsUpdate, kNetworkStatsSection, true);
int   gNetStat1NumConnections  = 0;
float gNetStat2LatencyAvg      = 0.0f;
float gNetStat3LatencySD       = 0.0f;
float gNetStat4LatencyMin      = 0.0f;
float gNetStat5LatencyMax      = 0.0f;
float gNetStat6PingArrivedPC   = 0.0f;
float gNetStat7ExtQueuedAvg_ms = 0.0f;
float gNetStat8ExtQueuedMin_ms = 0.0f;
float gNetStat9ExtQueuedMax_ms = 0.0f;
float gNetStatAQueuedAvg_ms    = 0.0f;
float gNetStatBQueuedMin_ms    = 0.0f;
float gNetStatCQueuedMax_ms    = 0.0f;

// Stats are written into console vars so they can be live viewed from console menu
WRAP_CONSOLE_VAR(int,    gNetStat1NumConnections,  kNetworkStatsSection);
WRAP_CONSOLE_VAR(float,  gNetStat2LatencyAvg,      kNetworkStatsSection);
WRAP_CONSOLE_VAR(float,  gNetStat3LatencySD,       kNetworkStatsSection);
WRAP_CONSOLE_VAR(float,  gNetStat4LatencyMin,      kNetworkStatsSection);
WRAP_CONSOLE_VAR(float,  gNetStat5LatencyMax,      kNetworkStatsSection);
WRAP_CONSOLE_VAR(float,  gNetStat6PingArrivedPC,   kNetworkStatsSection);
WRAP_CONSOLE_VAR(float,  gNetStat7ExtQueuedAvg_ms, kNetworkStatsSection);
WRAP_CONSOLE_VAR(float,  gNetStat8ExtQueuedMin_ms, kNetworkStatsSection);
WRAP_CONSOLE_VAR(float,  gNetStat9ExtQueuedMax_ms, kNetworkStatsSection);
WRAP_CONSOLE_VAR(float,  gNetStatAQueuedAvg_ms,    kNetworkStatsSection);
WRAP_CONSOLE_VAR(float,  gNetStatBQueuedMin_ms,    kNetworkStatsSection);
WRAP_CONSOLE_VAR(float,  gNetStatCQueuedMax_ms,    kNetworkStatsSection);

} // end namespace Util
} // end namespace Anki
