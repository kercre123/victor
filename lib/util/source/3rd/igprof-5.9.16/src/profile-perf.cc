#include "profile.h"
#include "profile-trace.h"
#include "hook.h"
#include "walk-syms.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <sys/time.h>

#ifdef __APPLE__
typedef sig_t sighandler_t;
#endif

// -------------------------------------------------------------------
// Traps for this profiler module
LIBHOOK(0, int, dofork, _main, (), (), "fork", 0, 0)
LIBHOOK(1, int, dosystem, _main, (const char *cmd), (cmd), "system", 0, 0)
LIBHOOK(3, int, dopthread_sigmask, _main,
        (int how, sigset_t *newmask, sigset_t *oldmask),
        (how, newmask, oldmask),
        "pthread_sigmask", 0, 0)
LIBHOOK(3, int, dosigaction, _main,
        (int signum, const struct sigaction *act, struct sigaction *oact),
        (signum, act, oact),
        "sigaction", 0, 0)
//Looks like the dynamic loader invokes `close` on ARM, which leads to a
//segfault in the doclose / dofclose hooks. For the moment I just exclude the
//hook, however given it is actually used to protect an output corruption in
//case a program closes stderr, we should probably find a better solution.
#ifndef __arm__
LIBHOOK(1, int, dofclose, _main, (FILE * stream), (stream), "fclose", 0, 0)
LIBHOOK(1, int, doclose, _main, (int fd), (fd), "close", 0, 0)
#endif

// Data for this profiler module
static IgProfTrace::CounterDef  s_ct_ticks      = { "PERF_TICKS", IgProfTrace::TICK, -1, 0 };
static bool                     s_initialized   = false;
static bool                     s_keep          = false;
static int                      s_signal        = SIGPROF;
static int                      s_itimer        = ITIMER_PROF;

/** Convert timeval to seconds. */
static inline double tv2sec(const timeval &tv)
{ return tv.tv_sec + tv.tv_usec * 1e-6; }

/** Performance profiler signal handler, SIGPROF or SIGALRM depending
    on the current profiler mode.  Record a tick for the current
    program location.  Assumes the signal handler is registered for
    the correct thread.  Skip ticks when this profiler is not
    enabled.  */
static void
profileSignalHandler(int /* nsig */, siginfo_t * /* info */, void * /* ctx */)
{
  void *addresses[IgProfTrace::MAX_DEPTH];
  if (LIKELY(igprof_disable()))
  {
    IgProfTrace *buf = igprof_buffer();
    if (LIKELY(buf))
    {
      IgProfTrace::Stack *frame;
      uint64_t tstart, tend;
      int depth;

      RDTSC(tstart);
      depth = IgHookTrace::stacktrace(addresses, IgProfTrace::MAX_DEPTH);
      RDTSC(tend);

      // Drop top two stackframes (me, signal frame).
      buf->lock();
      frame = buf->push(addresses+2, depth-2);
      buf->tick(frame, &s_ct_ticks, 1, 1);
      buf->traceperf(depth, tstart, tend);
      buf->unlock();
    }
  }
  igprof_enable();
}

/** Enable profiling timer.  You should have called
    #enableSignalHandler() before calling this function.
    This needs to be executed in every thread to be profiled. */
static void
enableTimer(void)
{
  itimerval interval = { { 0, 5000 }, { 0, 5000 } };
  setitimer(s_itimer, &interval, 0);
}

/** Enable profiling signal handler.  */
static void
enableSignalHandler(void)
{
  sigset_t profset;
  sigemptyset(&profset);
  sigaddset(&profset, s_signal);
  pthread_sigmask(SIG_UNBLOCK, &profset, 0);

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = (sighandler_t) &profileSignalHandler;
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sigaction(s_signal, &sa, 0);
}

/** Thread setup function.  */
static void
threadInit(void)
{
  // Enable profiling in this thread.
  enableSignalHandler();
  enableTimer();
}

// -------------------------------------------------------------------
/** Possibly start performance profiler.  */
static void
initialize(void)
{
  if (s_initialized) return;
  s_initialized = true;

  const char    *options = igprof_options();
  bool          enable = false;
  bool          enable_on_init = true;

  while (options && *options)
  {
    while (*options == ' ' || *options == ',')
      ++options;

    if (! strncmp(options, "perf", 4))
    {
      enable = true;
      options += 4;
      while (*options)
      {
        if (! strncmp(options, ":real", 5))
        {
          s_signal = SIGALRM;
          s_itimer = ITIMER_REAL;
          options += 5;
        }
        else if (! strncmp(options, ":user", 5))
        {
          s_signal = SIGVTALRM;
          s_itimer = ITIMER_VIRTUAL;
          options += 5;
        }
        else if (! strncmp(options, ":process", 7))
        {
          s_signal = SIGPROF;
          s_itimer = ITIMER_PROF;
          options += 7;
        }
        else if (! strncmp(options, ":keep", 5))
        {
          s_keep = true;
          options += 5;
        }
        else if (! strncmp(options, ":nostart", 8))
        {
          enable_on_init = false;
          options += 8;
        }
        else
          break;
      }
    }
    else
      options++;

    while (*options && *options != ',' && *options != ' ')
      options++;
  }

  if (! enable)
    return;

  double clockres = 0;
  itimerval precision;
  itimerval interval = { { 0, 5000 }, { 100, 0 } };
  itimerval nullified = { { 0, 0 }, { 0, 0 } };
  setitimer(s_itimer, &interval, 0);
  getitimer(s_itimer, &precision);
  setitimer(s_itimer, &nullified, 0);
  clockres = precision.it_interval.tv_sec
             + 1e-6 * precision.it_interval.tv_usec;

  if (! igprof_init("performance profiler", &threadInit, true, clockres))
    return;

  igprof_disable_globally();
  if (s_itimer == ITIMER_REAL)
    igprof_debug("performance profiler: measuring real time\n");
  else if (s_itimer == ITIMER_VIRTUAL)
    igprof_debug("performance profiler: measuring user time\n");
  else if (s_itimer == ITIMER_PROF)
    igprof_debug("performance profiler: measuring process cpu time\n");

  // Enable profiler.
  IgHook::hook(dofork_hook_main.raw);
  IgHook::hook(dosystem_hook_main.raw);
  IgHook::hook(dopthread_sigmask_hook_main.raw);
  IgHook::hook(dosigaction_hook_main.raw);
#ifndef __arm__
  IgHook::hook(doclose_hook_main.raw);
  IgHook::hook(dofclose_hook_main.raw);
#endif
  igprof_debug("performance profiler enabled\n");

  enableSignalHandler();
  enableTimer();
  if (enable_on_init)
    igprof_enable_globally();
}

// -------------------------------------------------------------------
// Trap fiddling with thread signal masks
static int
dopthread_sigmask(IgHook::SafeData<igprof_dopthread_sigmask_t> &hook,
                  int how, sigset_t *newmask,  sigset_t *oldmask)
{
  struct sigaction cursig;
  struct itimerval curtimer;
  if (newmask
      && (how == SIG_BLOCK || how == SIG_SETMASK)
      && sigismember(newmask, s_signal)
      && sigaction(s_signal, 0, &cursig) == 0
      && cursig.sa_handler
      && getitimer(s_itimer, &curtimer) == 0
      && (curtimer.it_interval.tv_sec || curtimer.it_interval.tv_usec))
  {
    igprof_debug("pthread_sigmask(): prevented profiling signal"
                 " %d from being blocked in thread 0x%lx"
                 " [handler 0x%lx, interval %.0f us]\n",
                 s_signal, (unsigned long) pthread_self(),
                 (unsigned long) cursig.sa_handler,
                 1e6 * curtimer.it_interval.tv_sec
                 + curtimer.it_interval.tv_usec);
    sigdelset(newmask, s_signal);
  }

  return hook.chain(how, newmask, oldmask);
}

// Trap fiddling with the profiling signal.
static int
dosigaction(IgHook::SafeData<igprof_dosigaction_t> &hook,
            int signum, const struct sigaction *act, struct sigaction *oact)
{
  struct sigaction sa;
  if (signum == s_signal
      && act
      && act->sa_handler != (sighandler_t) &profileSignalHandler)
  {
    igprof_debug("sigaction(): prevented profiling signal"
                 " %d from being overridden in thread 0x%lx\n",
                 s_signal, (unsigned long) pthread_self());
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = (sighandler_t) &profileSignalHandler;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    act = &sa;
  }

  return hook.chain(signum, act, oact);
}

// Trap fork to deactivate profiling around it, then artificially
// add back the cost associated to actual fork call. This trickery
// is required because large processes can take a rather long time
// to fork, and linux clone() routine can go into an infinite loop
// processing ERESTARTNOINTR if a signal arrives within clone().
static int
dofork(IgHook::SafeData<igprof_dofork_t> &hook)
{
  // Slow down profiling to once per 10sec, which should be slow
  // enough to complete fork() under any circumstances.
  double ival;
  double dt = 0;
  int nticks = 0;
  IgProfTrace *buf;
  bool enabled = igprof_disable();
  itimerval orig;
  itimerval slow = { { 10, 0 }, { 10, 0 } };
  itimerval fast = { { 0, 5000 }, { 0, 5000 } };
  itimerval left = { { 0, 0 }, { 0, 0 } };
  getitimer(s_itimer, &left);
  setitimer(s_itimer, &slow, &orig);
  getitimer(s_itimer, &slow);
  dt = tv2sec(left.it_interval) - tv2sec(left.it_value);

  // Do the fork() call.
  int ret = hook.chain();

  // Now calculate how much time we spent doing the fork, and blame
  // the actual system call for it, drop this frame out of stack.
  // Assign costs to the parent, but re-enable timer also in child.
  // Normally we reset profiles in child, but allow an override.
  if (ret >= 0)
  {
    getitimer(s_itimer, &left);
    setitimer(s_itimer, &orig, 0);
    getitimer(s_itimer, &fast);
    ival = tv2sec(fast.it_interval);
    dt += tv2sec(slow.it_value) - tv2sec(left.it_value);
    nticks = (ival > 0 ? int(dt / ival + 0.5) : 0);

    if (ret == 0)
    {
      dt = nticks = 0;
      if (! s_keep)
        igprof_reset_profiles();
    }

    if (enabled && nticks && (buf = igprof_buffer()))
    {
      void *addresses[IgProfTrace::MAX_DEPTH];
      IgProfTrace::Stack *frame;
      uint64_t tstart, tend;
      int depth;

      RDTSC(tstart);
      depth = IgHookTrace::stacktrace(addresses, IgProfTrace::MAX_DEPTH);
      RDTSC(tend);

      // Replace top stack frame (this hook) with the original.
      if (depth > 0) addresses[0] = __extension__ (void *) hook.original;
      frame = buf->push(addresses, depth);
      buf->tick(frame, &s_ct_ticks, 1, nticks);
      buf->traceperf(depth, tstart, tend);
    }

    if (ret != 0)
      igprof_debug("resuming profiling after blinking for fork() for"
		   " %.3fms, %d ticks\n", dt*1000, nticks);
  }

  igprof_enable();
  return ret;
}

// Trap system to deactivate profiling around it, like fork(). The
// system() normally calls clone() system call directly via vsyscall
// interface, without going thru fork() or clone() functions, so we
// can't trap into clone().
static int
dosystem(IgHook::SafeData<igprof_dosystem_t> &hook, const char *cmd)
{
  // See fork() for the implementation details.
  double ival;
  double dt = 0;
  int nticks = 0;
  IgProfTrace *buf;
  bool enabled = igprof_disable();
  itimerval orig;
  itimerval slow = { { 10, 0 }, { 10, 0 } };
  itimerval fast = { { 0, 5000 }, { 0, 5000 } };
  itimerval left = { { 0, 0 }, { 0, 0 } };
  getitimer(s_itimer, &left);
  setitimer(s_itimer, &slow, &orig);
  getitimer(s_itimer, &slow);
  dt = tv2sec(left.it_interval) - tv2sec(left.it_value);

  int ret = hook.chain(cmd);

  getitimer(s_itimer, &left);
  setitimer(s_itimer, &orig, 0);
  getitimer(s_itimer, &fast);
  ival = tv2sec(fast.it_interval);
  dt += tv2sec(slow.it_value) - tv2sec(left.it_value);
  nticks = (ival > 0 ? int(dt / ival + 0.5) : 0);
  if (enabled && nticks && (buf = igprof_buffer()))
  {
    void *addresses[IgProfTrace::MAX_DEPTH];
    IgProfTrace::Stack *frame;
    uint64_t tstart, tend;
    int depth;

    RDTSC(tstart);
    depth = IgHookTrace::stacktrace(addresses, IgProfTrace::MAX_DEPTH);
    RDTSC(tend);

    // Replace top stack frame (this hook) with the original.
    if (depth > 0) addresses[0] = __extension__ (void *) hook.original;
    frame = buf->push(addresses, depth);
    buf->tick(frame, &s_ct_ticks, 1, nticks);
    buf->traceperf(depth, tstart, tend);
  }

  igprof_debug("resuming profiling after blinking for system() for"
	       " %.3fms, %d ticks\n", dt*1000, nticks);
  igprof_enable();
  return ret;
}

#ifndef __arm__
// If the profiled program closes stderr stream the igprof_debug got to be
// disabled by changing the value of s_igprof_stderrOpen (declared in profile.h
// an defined in profile.cc) to false. This is don by instrumenting close and and
// fclose via the two methods below.
static int
doclose(IgHook::SafeData<igprof_doclose_t> &hook, int fd)
{
  if (fd == 2)
  {
    pthread_t thread = pthread_self();
    igprof_debug("close(2) called in thread %x. Igprof debug disabled.\n",thread);
    s_igprof_stderrOpen = false;
  }
  return hook.chain(fd);
}

static int
dofclose(IgHook::SafeData<igprof_dofclose_t> &hook, FILE * stream)
{
  if (stream == stderr)
  {
    pthread_t thread = pthread_self();
    igprof_debug("fclose(stderr) called in thread %x. Igprof debug disabled.\n",thread);
    s_igprof_stderrOpen = false;
  }
  return hook.chain(stream);
}
#endif
// -------------------------------------------------------------------
static bool autoboot __attribute__((used)) = (initialize(), true);
