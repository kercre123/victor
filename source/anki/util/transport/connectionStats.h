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

CONSOLE_VAR_EXTERN(bool, kNetConnStatsUpdate);
extern int   gNetStat1NumConnections;
extern float gNetStat2LatencyAvg;
extern float gNetStat3LatencySD;
extern float gNetStat4LatencyMin;
extern float gNetStat5LatencyMax;
extern float gNetStat6PingArrivedPC;
extern float gNetStat7ExtQueuedAvg_ms;
extern float gNetStat8ExtQueuedMin_ms;
extern float gNetStat9ExtQueuedMax_ms;
extern float gNetStatAQueuedAvg_ms;
extern float gNetStatBQueuedMin_ms;
extern float gNetStatCQueuedMax_ms;

} // end namespace Util
} // end namespace Anki

#endif // __Util_Transport_ConnectionStats_H__
