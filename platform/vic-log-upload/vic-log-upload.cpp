/**
* File: vic-log-upload.cpp
*
* Description: Victor Log Upload application main
*
* Copyright: Anki, inc. 2018
*
*/

#include "platform/robotLogUploader/robotLogDumper.h"
#include "platform/robotLogUploader/robotLogUploader.h"

#include "json/json.h"
#include "platform/victorCrashReports/victorCrashReporter.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/logging/victorLogger.h"
#include "util/string/stringUtils.h"

#define LOG_PROCNAME "vic-log-upload"
#define LOG_CHANNEL "VicLogUpload"

//
// Report result to stdout as parsable json struct
//
static void Report(const std::string & status, const std::string & value)
{
  LOG_INFO("VicLogUpload.Report", "result[%s] = %s", status.c_str(), value.c_str());

  Json::Value json;
  json["result"][status] = value;

  Json::StyledWriter writer;
  fprintf(stdout, "%s", writer.write(json).c_str());
}

//
// Clean up logging and exit w/given status
//
static void Exit(int status)
{
  LOG_INFO("VicLogUpload.Exit", "exit(%d)", status);
  Anki::Util::gLoggerProvider = nullptr;
  Anki::Util::gEventProvider = nullptr;
  exit(status);
}

int main(int argc, const char * argv[])
{
  using namespace Anki;
  using namespace Anki::Util;
  using namespace Anki::Vector;

  // Set up crash reporter
  Anki::Victor::InstallCrashReporter(LOG_PROCNAME);

  // Set up logging
  auto logger = std::make_unique<VictorLogger>(LOG_PROCNAME);
  gLoggerProvider = logger.get();
  gEventProvider = logger.get();

  // Do the thing
  RobotLogDumper logDumper;
  const std::string & gzpath = "/tmp/" + Anki::Util::GetUUIDString() + ".gz";

  Result result = logDumper.Dump(gzpath);
  if (result != RESULT_OK) {
    LOG_ERROR("VicLogUpload", "Unable to create log dump %s (error %d)", gzpath.c_str(), result);
    Report("error", "Unable to create log dump");
    Exit(1);
  }

  RobotLogUploader logUploader;
  std::string url;

  result = logUploader.Upload(gzpath, url);

  Anki::Util::FileUtils::DeleteFile(gzpath);

  if (result != RESULT_OK) {
    LOG_ERROR("VicLogUpload", "Unable to upload log dump %s (error %d)", gzpath.c_str(), result);
    Report("error", "Unable to upload log dump");
    Exit(1);
  }

  Report("success", url);
  Exit(0);
}
