/**
 * File: victorCrashReports/tombstoneHooks.cpp
 *
 * Description: Implementation of tombstone crash hooks
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "tombstoneHooks.h"
#include "debugger.h"

#include <list>
#include <unordered_map>

#include <signal.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace {

  // Which signals do we hook for intercept?
  const std::list<int> gHookSignals = { SIGILL, SIGABRT, SIGBUS, SIGFPE, SIGSEGV };

  // Keep a stash of original signal actions so they can be restored
  std::unordered_map<int, struct sigaction> gHookStash;
}

// Return OS thread ID.  Note this is not the same as POSIX thread ID or pthread_self()!
// http://man7.org/linux/man-pages/man2/gettid.2.html
static pid_t gettid()
{
  return (pid_t) syscall(SYS_gettid);
}

// Deliver a signal to a specific thread
// http://man7.org/linux/man-pages/man2/tgkill.2.html
static int tgkill(pid_t tgid, pid_t tid, int signum)
{
  return syscall(SYS_tgkill, tgid, tid, signum);
}

//
// Ask debuggerd to create tombstone for this process,
// then set up a call to default handler.
//
static void DebuggerHook(int signum, siginfo_t * info, void * ctx)
{
  const auto pid = getpid();
  const auto tid = gettid();

  // Call MODIFIED VERSION of libcutils dump_tombstone_timeout()
  // to create a tombstone for this process.  Modified version
  // will return without waiting for dump to complete.
  victor_dump_tombstone_timeout(tid, nullptr, 0, -1);

  /* remove our handler so we fault for real when we return */
  signal(signum, SIG_DFL);

  /*
   * These signals are not re-thrown when we resume.  This means that
   * crashing due to (say) SIGPIPE doesn't work the way you'd expect it
   * to.  We work around this by throwing them manually.  We don't want
   * to do this for *all* signals because it'll screw up the address for
   * faults like SIGSEGV.
   */
  switch (signum) {
    case SIGABRT:
    case SIGFPE:
    case SIGPIPE:
#ifdef SIGSTKFLT
    case SIGSTKFLT:
#endif
      (void) tgkill(pid, tid, signum);
      break;
    default:    // SIGILL, SIGBUS, SIGSEGV
      break;
  }
}

//
// Install signal handler for a given signal
//
static void InstallTombstoneHook(int signum)
{
  struct sigaction newAction;
  struct sigaction oldAction;
  memset(&newAction, 0, sizeof(newAction));
  memset(&oldAction, 0, sizeof(oldAction));

  newAction.sa_flags = (SA_SIGINFO | SA_RESTART | SA_ONSTACK);
  newAction.sa_sigaction = DebuggerHook;

  if (sigaction(signum, &newAction, &oldAction) == 0) {
    gHookStash[signum] = std::move(oldAction);
  }

}

//
// Restore original handler for a given signal
//
static void UninstallTombstoneHook(int signum)
{
  const auto pos = gHookStash.find(signum);
  if (pos != gHookStash.end()) {
    sigaction(signum, &pos->second, nullptr);
  }
}

namespace Anki {
namespace Victor {

// Install handler for each signal we want to intercept
void InstallTombstoneHooks()
{
  for (auto signum : gHookSignals) {
    InstallTombstoneHook(signum);
  }
}

// Restore handlers to original state
void UninstallTombstoneHooks()
{
  for (auto signum : gHookSignals) {
    UninstallTombstoneHook(signum);
  }
}

} // end namespace Victor
} // end namespace Anki
