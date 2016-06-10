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

#ifndef __Util_Transport_ConnectionStats_H__
#define __Util_Transport_ConnectionStats_H__


#include "util/console/consoleInterface.h"
#include <stdint.h>


namespace Anki {
namespace Util {
  
  
#if REMOTE_CONSOLE_ENABLED
  #define ENABLE_RELIABLE_CONNECTION_STATS 1
#else
  #define ENABLE_RELIABLE_CONNECTION_STATS 0
#endif
  
#if ENABLE_RELIABLE_CONNECTION_STATS
  CONSOLE_VAR_EXTERN(bool,   kNetConnStatsUpdate);
  // Stats are written into console vars so they can be live viewed from console menu
  CONSOLE_VAR_EXTERN(int,    gNetStat1NumConnections);
  CONSOLE_VAR_EXTERN(float,  gNetStat2LatencyAvg);
  CONSOLE_VAR_EXTERN(float,  gNetStat3LatencySD);
  CONSOLE_VAR_EXTERN(float,  gNetStat4LatencyMin);
  CONSOLE_VAR_EXTERN(float,  gNetStat5LatencyMax);
  CONSOLE_VAR_EXTERN(float,  gNetStat6PingArrivedPC);
  CONSOLE_VAR_EXTERN(float,  gNetStat7ExtQueuedAvg_ms);
  CONSOLE_VAR_EXTERN(float,  gNetStat8ExtQueuedMin_ms);
  CONSOLE_VAR_EXTERN(float,  gNetStat9ExtQueuedMax_ms);
  CONSOLE_VAR_EXTERN(float,  gNetStatAQueuedAvg_ms);
  CONSOLE_VAR_EXTERN(float,  gNetStatBQueuedMin_ms);
  CONSOLE_VAR_EXTERN(float,  gNetStatCQueuedMax_ms);
#endif // REMOTE_CONSOLE_ENABLED


} // end namespace Util
} // end namespace Anki

#endif // __Util_Transport_ConnectionStats_H__
