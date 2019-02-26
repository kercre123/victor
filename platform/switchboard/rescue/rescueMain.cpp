#include "rescue/miniFaceDisplay.h"
#include "anki-ble/common/log.h"
#include "ev++.h"
#include "switchboardd/daemon.h"
#include "switchboardd/taskExecutor.h"
#include "rescue/rescueClient.h"

#include "core/lcd.h"
#include "platform/victorCrashReports/victorCrashReporter.h"
#include "util/logging/DAS.h"
#include "util/logging/logging.h"
#include "util/logging/victorLogger.h"

#define LOG_PROCNAME "vic-rescue"

namespace { // private members
  static struct ev_signal sIntSig;
  static struct ev_signal sTermSig;
  static ev_timer sTimer;
  static struct ev_loop* sLoop;
  const static uint32_t kTick_s = 30;
  std::unique_ptr<Anki::Switchboard::Daemon> _daemon;
  std::shared_ptr<Anki::Switchboard::RescueClient> _rescueClient;
}

static void ExitHandler(int status = 0) {
  // todo: smoothly handle termination

  Anki::Util::gLoggerProvider = nullptr;
  Anki::Util::gEventProvider = nullptr;

  Anki::Vector::UninstallCrashReporter();

  _exit(status);
}

static void SignalCallback(struct ev_loop* loop, struct ev_signal* w, int revents)
{
  logi("Exiting for signal %d", w->signum);

  if(_daemon != nullptr) {
    _daemon->Stop();
  }

  ev_timer_stop(sLoop, &sTimer);
  ev_unloop(sLoop, EVUNLOOP_ALL);
  ExitHandler();
}

static void Tick(struct ev_loop* loop, struct ev_timer* w, int revents) {
  // static bool start_pairing = false;
  // if (!start_pairing) {
  //   _rescueClient->StartPairing();
  //   start_pairing = true;
  // }
  // noop
}

int RescueMain() {
  setAndroidLoggingTag("vic-rescue");

  Anki::Vector::InstallCrashReporter(LOG_PROCNAME);

  Anki::Util::VictorLogger logger(LOG_PROCNAME);
  Anki::Util::gLoggerProvider = &logger;
  Anki::Util::gEventProvider = &logger;

  DASMSG(rescue_hello, "rescue.hello", "vic-rescue start");
  DASMSG_SET(s1, "hello", "Test string");
  DASMSG_SET(i1, getpid(), "Test value");
  DASMSG_SEND();

  sLoop = ev_default_loop(0);

  ev_signal_init(&sIntSig, SignalCallback, SIGINT);
  ev_signal_start(sLoop, &sIntSig);
  ev_signal_init(&sTermSig, SignalCallback, SIGTERM);
  ev_signal_start(sLoop, &sTermSig);

  {
    // initialize daemon
    using namespace Anki::Switchboard;
    Daemon::Params params{};
    params.taskExecutor = std::make_shared<Anki::TaskExecutor>(sLoop);
    params.connectionIdManager = std::make_shared<ConnectionIdManager>();
    _rescueClient = std::make_shared<RescueClient>();
    params.commandClient = _rescueClient;
    _daemon = std::make_unique<Daemon>(sLoop, params);
    _daemon->Start();

    // // start pairing after short delay
    // auto when = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    // params.taskExecutor->WakeAfter([_rescueClient](){
    //   _rescueClient->StartPairing();
    // }, when);

  }

  ev_timer_init(&sTimer, Tick, kTick_s, kTick_s);
  ev_timer_start(sLoop, &sTimer);

  // Wait for IPC connection to ankibluetoothd & command client init messaging
  while ( !(_rescueClient->IsConnected() && _daemon->IsBLEReady()) ) {
    ev_loop(sLoop, EVRUN_ONCE);
  }

  ev_loop(sLoop, EVRUN_ONCE);
  ev_loop(sLoop, EVRUN_ONCE);
  ev_loop(sLoop, EVRUN_ONCE);

  printf("StartPairing!");
  _rescueClient->StartPairing();

  ev_loop(sLoop, 0);

  // exit
  ExitHandler();
  return 0;
}

int main(int argc, char* argv[])
{
    // Init lcd
  int rc = lcd_init();
  if (rc != 0)
  {
    printf("Failed to init lcd\n");
    return rc;
  }

  Anki::Vector::DrawFaultCode(666, true);

  rc = RescueMain();

  // Leave last rendering on screen
  // lcd_shutdown();

  return rc;
}

