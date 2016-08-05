/** Implementation for Espressif crash reporter
 * @author Daniel Casner <daniel@anki.com>
 */

#include "crashReporter.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/esp.h"
#include "anki/cozmo/robot/crashLogs.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"
extern "C"
{
  #include "mem.h"
  #include "driver/crash.h"
}

#define DEBUG_CR 0
#if DEBUG_CR
#define debug(...) os_printf(__VA_ARGS__)
#else
#define debug(...)
#endif

namespace Anki {
namespace Cozmo {
namespace CrashReporter {

static CrashLog_K02* rtipCrashLog;
static CrashLog_NRF* bodyCrashLog;
static int8_t rtipCrashReadState;
static int8_t bodyCrashReadState;
static int reportIndex;

enum BodyNVTag
{ // Depends on storage.h
  BodyNVTagRTIPCrash = 2,
  BodyNVTagBodyCrash = 1,
};

enum BodyCrashReadState {
  RBS_read = 0, // 0 or more
  RBS_wait_for_init = -1,
  RBS_wait_for_read = -2,
  RBS_put_report    = -3,
  RBS_erase         = -4,
  RBS_done          = -5,
};

#define BODY_NV_STORAGE_MAX_PAYLOAD 26

bool Init()
{
  rtipCrashLog = NULL;
  bodyCrashLog = NULL;
  rtipCrashReadState = RBS_wait_for_init;
  bodyCrashReadState = RBS_wait_for_init;
  reportIndex = MAX_CRASH_LOGS;
  return true;
}

void StartSending()
{
  reportIndex = 0;
}

void StartQuerry()
{
  rtipCrashReadState = RBS_read;
}

void Update()
{  
  if (rtipCrashReadState >= RBS_read)
  {
    ReadBodyStorage rbs;
    rbs.key = BodyNVTagRTIPCrash;
    rbs.offset = rtipCrashReadState;
    debug("Requesting rtip crash report: %d %d...", rbs.key, rbs.offset);
    if (RobotInterface::SendMessage(rbs))
    {
      rtipCrashReadState = RBS_wait_for_read;
      debug("sent\r\n");
    }
    else
    {
      debug("fail\r\n");
    }
  }
  else if (rtipCrashReadState == RBS_put_report)
  {
    if (rtipCrashLog == NULL)
    {
      AnkiWarn( 208, "CrashReporter.PutRtipCrashLog.NULL", 515, "RTIP Crash log pointer is null", 0);
    }
    else
    {
      CrashRecord record;
      record.reporter  = RobotInterface::RTIPCrash;
      record.errorCode = 0;
      os_memcpy(record.dump, rtipCrashLog, sizeof(CrashLog_K02));
      const int putRslt = crashHandlerPutReport(&record);
      if (putRslt < 0) 
      {
        AnkiWarn( 209, "CrashReporter.PutRtipCrashLog.Failed", 516, "Failed to put crash record %d", 1, putRslt);
      }
      else if (reportIndex == MAX_CRASH_LOGS) 
      {
        reportIndex = putRslt; // Report this one to engine
      }
    }
    rtipCrashReadState = RBS_erase;
    os_free(rtipCrashLog);
    rtipCrashLog = NULL;
  }
  else if (rtipCrashReadState == RBS_erase)
  {
    WriteBodyStorage wbs;
    wbs.key = BodyNVTagRTIPCrash;
    wbs.data_length = 0;
    debug("Erasing RTIP crash log: %d %d\r\n", wbs.key, wbs.data_length);
    if (RobotInterface::SendMessage(wbs)) 
    {
      rtipCrashReadState = RBS_done;
      bodyCrashReadState = RBS_read;
    }
  }
  
  else if (bodyCrashReadState >= RBS_read)
  {
    ReadBodyStorage rbs;
    rbs.key = BodyNVTagBodyCrash;
    rbs.offset = bodyCrashReadState;
    debug("Requesting body crash report: %d %d...", rbs.key, rbs.offset);
    if (RobotInterface::SendMessage(rbs))
    {
      bodyCrashReadState = RBS_wait_for_read;
      debug("success\r\n");
    }
    else
    {
      debug("fail\r\n");
    }
  }
  else if (bodyCrashReadState == RBS_put_report)
  {
    if (bodyCrashLog == NULL)
    {
      AnkiWarn( 210, "CrashReporter.PutBodyCrashLog.NULL", 517, "Body Crash log pointer is null", 0);
    }
    else
    {
      CrashRecord record;
      record.reporter  = RobotInterface::BodyCrash;
      record.errorCode = 0;
      os_memcpy(record.dump, bodyCrashLog, sizeof(CrashLog_NRF));
      const int putRslt = crashHandlerPutReport(&record);
      if (putRslt < 0) 
      {
        AnkiWarn( 211, "CrashReporter.PutBodyCrashLog.Failed", 516, "Failed to put crash record %d", 1, putRslt);
      }
      else if (reportIndex == MAX_CRASH_LOGS) 
      {
        reportIndex = putRslt; // Report this one to engine
      }
    }
    bodyCrashReadState = RBS_erase;
    os_free(bodyCrashLog);
    bodyCrashLog = NULL;
  }
  else if (bodyCrashReadState == RBS_erase)
  {
    WriteBodyStorage wbs;
    wbs.key = BodyNVTagBodyCrash;
    wbs.data_length = 0;
    debug("Erasing body crash log: %d %d\r\n", wbs.key, wbs.data_length);
    if (RobotInterface::SendMessage(wbs))
    {
      bodyCrashReadState = RBS_done;
    }
  }

  else if (reportIndex < MAX_CRASH_LOGS)
  {
    CrashRecord record;
    crashHandlerGetReport(reportIndex, &record);
    if (record.nWritten == 0 && record.nReported != 0)
    {
      RobotInterface::CrashReport report;
      report.errorCode = record.errorCode;
      switch (record.reporter)
      {
        case RobotInterface::WiFiCrash:
        {
          report.which = RobotInterface::WiFiCrash;
          report.dump_length = sizeof(CrashLog_ESP)/sizeof(uint32_t);
          break;
        }
        case RobotInterface::RTIPCrash:
        {
          report.which = RobotInterface::RTIPCrash;
          report.dump_length = sizeof(CrashLog_K02)/sizeof(uint32_t);
          break;
        }
        case RobotInterface::BodyCrash:
        {
          report.which = RobotInterface::BodyCrash;
          report.dump_length = sizeof(CrashLog_NRF)/sizeof(uint32_t);
          break;
        }
        default:
        {
          AnkiWarn( 206, "CrashReporter.UnknownReporter", 513, "Reporter = %d", 1, record.reporter);
          report.dump_length = 0;
        }
      }
      os_memcpy(report.dump, record.dump, report.dump_length);
      if (RobotInterface::SendMessage(report))
      {
        crashHandlerMarkReported(reportIndex);
        reportIndex++;
      }
      else
      {
        AnkiDebug( 207, "CrashReporter.FailedToSend", 514, "Couldn't send crash report, will retry.", 0);
      }
    }
  } // End sending crash reports to engine
}

void AcceptBodyStorage(BodyStorageContents& msg)
{
  debug("Got body storage data: %d, %d, %d\r\n", msg.key, msg.offset, msg.data_length);
  if (msg.key == BodyNVTagRTIPCrash)
  {
    if ((msg.offset == 0) && (msg.data_length == 0))
    {
      rtipCrashReadState = RBS_done;
      bodyCrashReadState = RBS_read;
    }
    else
    {
      if (rtipCrashLog == NULL)
      {
        rtipCrashLog = reinterpret_cast<CrashLog_K02*>(os_zalloc(sizeof(CrashLog_K02)));
        AnkiConditionalWarnAndReturn(rtipCrashLog != NULL, 212, "CrashRecorder.AcceptBodyStorage.NoMem", 518, "Couldn't allocate memory for RTIP crash log", 0);
      }
      uint8_t* dataPtr = reinterpret_cast<uint8_t*>(rtipCrashLog);
      os_memcpy(dataPtr + msg.offset, msg.data, msg.data_length);
      if (msg.data_length < BODY_NV_STORAGE_MAX_PAYLOAD)
      {
        rtipCrashReadState = RBS_put_report;
      }
      else
      {
        rtipCrashReadState = msg.offset + msg.data_length;
      }
    }
  }
  else if (msg.key == BodyNVTagBodyCrash)
  {
    if ((msg.offset == 0) && (msg.data_length == 0))
    {
      bodyCrashReadState = RBS_done;
    }
    else
    {
      if (bodyCrashLog == NULL)
      {
        bodyCrashLog = reinterpret_cast<CrashLog_NRF*>(os_zalloc(sizeof(CrashLog_NRF)));
        AnkiConditionalWarnAndReturn(bodyCrashLog != NULL, 212, "CrashRecorder.AcceptBodyStorage.NoMem", 519, "Couldn't allocate memory for body crash log", 0);
      }
      uint8_t* dataPtr = reinterpret_cast<uint8_t*>(bodyCrashLog);
      os_memcpy(dataPtr + msg.offset, msg.data, msg.data_length);
      if (msg.data_length < BODY_NV_STORAGE_MAX_PAYLOAD)
      {
        bodyCrashReadState = RBS_put_report;
      }
      else
      {
        bodyCrashReadState = msg.offset + msg.data_length;
      }
    }
  }
}


} // Namespace CrashReporter
} // Namespace Cozmo
} // Namespace Anki
