/**
 * File: neuralNetProcess.cpp
 *
 * Author: Andrew Stein
 * Date:   1/29/2018
 *
 * Description: Implements vicos-specific INeuralNetMain interface and main() to create vic-neuralnets process
 *              to perform forward inference with neural nets on the robot.
 *
 * Copyright: Anki, Inc. 2018
 **/


#include "coretech/neuralnets/iNeuralNetMain.h"
#include "coretech/vision/engine/image_impl.h"
#include "platform/victorCrashReports/victorCrashReporter.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/logging/victorLogger.h"
#include "util/logging/multiLoggerProvider.h"
#include "util/logging/saveToFileLoggerProvider.h"

#include <signal.h>
#include <thread>

#define LOG_PATH "/tmp/logs/"
#define LOG_PROCNAME "vic-neuralnets"
#define LOG_CHANNEL "NeuralNets"

namespace Anki {
namespace Util {
  bool DropBreadcrumb(bool result, const char* file, int line);
}
}

namespace Anki {

class VicNeuralNetProcess : public NeuralNets::INeuralNetMain
{
public:
  VicNeuralNetProcess()
  {
    signal(SIGTERM, VicNeuralNetProcess::TriggerShutdown);
  }
  
protected:
  
  virtual bool ShouldShutdown() override
  {
    return sShouldShutdown;
  }
  
  virtual Util::ILoggerProvider* GetLoggerProvider() override
  {
    // Install crash handlers
    Victor::InstallCrashReporter(LOG_PROCNAME);
    
    // Create and set logger, depending on platform
    _logger.reset(new Util::VictorLogger(LOG_PROCNAME));
#if ANKI_NO_CRASHLOGGING
    return _logger.get();
#else
    Anki::Util::FileUtils::RemoveDirectory(LOG_PATH LOG_PROCNAME);

    _fileLoggerQueue.create("FileLoggerQueue");

    _fileLogger = std::make_unique<Anki::Util::SaveToFileLoggerProvider>(_fileLoggerQueue.get(), LOG_PATH LOG_PROCNAME);

    std::vector<Anki::Util::ILoggerProvider*> loggers = {_logger.get(), _fileLogger.get()};

    _combinedLoggers = std::make_unique<Anki::Util::MultiLoggerProvider>(loggers);
    return _combinedLoggers.get();
#endif
  }
  
  virtual void DerivedCleanup() override
  {
    Victor::UninstallCrashReporter();
  }
  
  virtual int GetPollPeriod_ms(const Json::Value& config) const override
  {
    return config["pollPeriod_ms"].asInt();
  }
  
  virtual void Step(int pollPeriod_ms) override
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(pollPeriod_ms));
  }
  
private:
  
  static bool sShouldShutdown;
  
  static void TriggerShutdown(int signum)
  {
    Anki::Util::DropBreadcrumb(false, nullptr, -1);
    LOG_INFO("VicNeuralNets.Shutdown", "Shutdown on signal %d", signum);
    sShouldShutdown = true;
  }

  Anki::Util::Dispatch::QueueHandle _fileLoggerQueue;
  std::unique_ptr<Util::VictorLogger> _logger;
  std::unique_ptr<Anki::Util::SaveToFileLoggerProvider> _fileLogger;
  std::unique_ptr<Anki::Util::MultiLoggerProvider> _combinedLoggers;
};

bool VicNeuralNetProcess::sShouldShutdown = false;
  
} // namespace Anki


int main(int argc, char** argv)
{
  if(argc < 4)
  {
    LOG_ERROR("VicNeuralNets.Main.UnexpectedArguments", "");
    std::cout << std::endl << "Usage: " << argv[0] << " <configFile>.json modelPath cachePath <imageFile>" << std::endl;
    std::cout << std::endl << " If no imageFile is provided, polls cachePath for neuralNetImage.png" << std::endl;
    return -1;
  }
  
  Anki::VicNeuralNetProcess vicNeuralNetMain;
  
  Anki::Result result = Anki::RESULT_OK;
  
  result = vicNeuralNetMain.Init(argv[1], argv[2], argv[3], (argc > 4 ? argv[4] : ""));
  
  if(Anki::RESULT_OK == result)
  {
    result = vicNeuralNetMain.Run();
  }
  
  if(Anki::RESULT_OK == result)
  {
    PRINT_NAMED_INFO("VicNeuralNets.Completed.Success", "");
    return 0;
  }
  else
  {
    PRINT_NAMED_ERROR("VicNeuralNets.Completed.Failure", "Result:%d", result);
    return -1;
  }
}
