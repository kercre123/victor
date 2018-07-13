#include "profile.h"
#include "profile-trace.h"
#include "sym-cache.h"
#include "atomic.h"
#include "fastio.h"
#include "hook.h"
#include "walk-syms.h"
#include <algorithm>
#include <sys/types.h>
#include <sys/time.h>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cerrno>
#include <cmath>
#include <set>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <locale.h>

#ifdef __APPLE__
# include <crt_externs.h>
# define program_invocation_name **_NSGetArgv()
#endif

// Global variables initialised once here.
HIDDEN void             (*igprof_abort)(void) __attribute__((noreturn)) = &abort;
HIDDEN char *           (*igprof_getenv)(const char *) = &getenv;
HIDDEN int              (*igprof_unsetenv)(const char *) = &unsetenv;
HIDDEN bool             s_igprof_activated = false;
       IgProfAtomic     s_igprof_enabled = 0;
HIDDEN pthread_key_t    s_igprof_bufkey;
HIDDEN pthread_key_t    s_igprof_flagkey;
HIDDEN int              s_igprof_stderrOpen = true;
// -------------------------------------------------------------------
// Used to capture real user start arguments in our custom thread wrapper
struct HIDDEN IgProfWrappedArg
{ void *(*start_routine)(void *); void *arg; };

struct HIDDEN IgProfDumpInfo
{ int depth; int nsyms; int nlibs; int nctrs;
  const char *tofile; FILE *output; FastIO io;
  IgProfSymCache *symcache; int blocksig;
  IgProfTrace::PerfStat perf; };

// -------------------------------------------------------------------
// Traps for this profiling module
DUAL_HOOK(1, void, doexit, _main, _libc,
          (int code), (code),
          "exit", 0, "libc.so.6")
DUAL_HOOK(1, void, doexit, _main2, _libc2,
          (int code), (code),
          "_exit", 0, "libc.so.6")
DUAL_HOOK(2, int,  dokill, _main, _libc,
          (pid_t pid, int sig), (pid, sig),
          "kill", 0, "libc.so.6")

// Trap vDSO wrappers in glibc and disable IgProf in such code
// paths. Details below.
// TODO: These should be DUAL_HOOK, otherwise we have ~1% failure rate
#if defined(__aarch64__)
LIBHOOK(2, int, dogettimeofday, _main,
        (struct timeval *tv, struct timezone *tz),
        (tv, tz),
        "gettimeofday", 0, "libc.so.6")

LIBHOOK(2, int, doclock_gettime, _main,
        (clockid_t clk_id, struct timespec *tp),
        (clk_id, tp),
        "clock_gettime", 0, "libc.so.6")

LIBHOOK(2, int, doclock_getres, _main,
        (clockid_t clk_id, struct timespec *res),
        (clk_id, res),
        "clock_getres", 0, "libc.so.6")

LIBHOOK(1, int, dosigreturn, _main,
        (unsigned long __unused),
        (__unused),
        "sigreturn", 0, "libc.so.6")
#endif /* defined(__aarch64__) */

LIBHOOK(4, int, dopthread_create, _main,
        (pthread_t *thread, const pthread_attr_t *attr,
         void * (*start_routine)(void *), void *arg),
        (thread, attr, start_routine, arg),
        "pthread_create", 0, 0)

LIBHOOK(4, int, dopthread_create, _pthread20,
        (pthread_t *thread, const pthread_attr_t *attr,
         void * (*start_routine)(void *), void *arg),
        (thread, attr, start_routine, arg),
        "pthread_create", "GLIBC_2.0", 0)

LIBHOOK(4, int, dopthread_create, _pthread21,
        (pthread_t *thread, const pthread_attr_t *attr,
         void * (*start_routine)(void *), void *arg),
        (thread, attr, start_routine, arg),
        "pthread_create", "GLIBC_2.1", 0)


// Data for this profiler module
static const int        MAX_FNAME       = 1024;
static const char       *s_initialized  = 0;
static bool             s_perthread     = false;
static volatile int     s_quitting      = 0;
static double           s_clockres      = 0;
static pthread_mutex_t  s_buflock       = PTHREAD_MUTEX_INITIALIZER;
static IgProfTrace      *s_masterbuf    = 0;
static IgProfTrace      *s_tracebuf     = 0;
static void             (*s_threadinit)() = 0;
static const char       *s_options      = 0;
static char             s_masterbufdata[sizeof(IgProfTrace)];
static pthread_t        s_mainthread;
static pthread_t        s_dumpthread;
static char             s_outname[MAX_FNAME];
static char             s_dumpflag[MAX_FNAME];

/** Return set of currently outstanding profile buffers. */
static std::set<IgProfTrace *> &
allTraceBuffers(void)
{
  static std::set<IgProfTrace *> *s_bufs = 0;
  if (! s_bufs) s_bufs = new std::set<IgProfTrace *>;
  return *s_bufs;
}

/** Create a new profile buffer and remember it. */
static IgProfTrace *
makeTraceBuffer(void)
{
  if (s_perthread)
  {
    IgProfTrace *buf = new IgProfTrace;
    pthread_mutex_lock(&s_buflock);
    allTraceBuffers().insert(buf);
    pthread_mutex_unlock(&s_buflock);
    return buf;
  }
  else
    return s_masterbuf;
}

/** Dispose a profile buffer. */
static void
disposeTraceBuffer(IgProfTrace *buf)
{
  if (buf && buf != s_masterbuf)
  {
    igprof_debug("merging profile buffer %p to master buffer %p\n",
                 (void *) buf, (void *) s_masterbuf);
    s_masterbuf->mergeFrom(*buf);
    allTraceBuffers().erase(buf);
    delete buf;
  }
}

/** Free a thread's trace buffer. */
static void
freeTraceBuffer(void *arg)
{
  ASSERT(arg);
  pthread_mutex_lock(&s_buflock);
  disposeTraceBuffer((IgProfTrace *) arg);
  pthread_mutex_unlock(&s_buflock);
}

/** Free a thread's profile-enabled flag. */
static void
freeThreadFlag(void *arg)
{
  delete (IgProfAtomic *) arg;
}

/** Dump out the profile data.  */
static void
dumpOneProfile(IgProfDumpInfo &info, IgProfTrace::Stack *frame)
{
  if (info.depth) // No address at root
  {
    IgProfSymCache::Symbol *sym = info.symcache->get(frame->address);

    if (LIKELY(sym->id >= 0))
      info.io.put("C").put(info.depth)
	     .put(" FN").put(sym->id)
	     .put("+").put(sym->symoffset);
    else
    {
      const char *symname = sym->name;
      char       symgen[32];
      size_t     symlen = 0;

      sym->id = info.nsyms++;

      if (UNLIKELY(! symname || ! *symname))
      {
        symlen = sprintf(symgen, "@?%p", sym->address);
        symname = symgen;
	ASSERT(symlen <= sizeof(symgen));
      }
      else
	symlen = strlen(symname);

      if (LIKELY(sym->binary->id >= 0))
	info.io.put("C").put(info.depth)
	       .put(" FN").put(sym->id)
	       .put("=(F").put(sym->binary->id)
	       .put("+").put(sym->binoffset)
	       .put(" N=(").put(symname, symlen)
	       .put("))+").put(sym->symoffset);
      else
      {
	const char *binname = sym->binary->name ? sym->binary->name : "";
	size_t binlen = strlen(binname);
	info.io.put("C").put(info.depth)
	       .put(" FN").put(sym->id)
	       .put("=(F").put(sym->binary->id = info.nlibs++)
	       .put("=(").put(binname, binlen)
	       .put(")+").put(sym->binoffset)
	       .put(" N=(").put(symname, symlen)
	       .put("))+").put(sym->symoffset);
      }
    }

    IgProfTrace::Counter **ctr = &frame->counters[0];
    for (int i = 0; i < IgProfTrace::MAX_COUNTERS && *ctr; ++i, ++ctr)
    {
      IgProfTrace::Counter *c = *ctr;
      if (c->ticks || c->peak)
      {
        if (LIKELY(c->def->id >= 0))
	  info.io.put(" V").put(c->def->id)
		 .put(":(").put(c->ticks)
		 .put(",").put(c->value)
		 .put(",").put(c->peak)
		 .put(")");
        else
	  info.io.put(" V").put(c->def->id = info.nctrs++)
		 .put("=(").put(c->def->name, strlen(c->def->name))
		 .put("):(").put(c->ticks)
		 .put(",").put(c->value)
                 .put(",").put(c->peak)
	         .put(")");

        if (c->def->derivedLeakSize)
        {  // Leak size is computed from the live resource
          for (IgProfTrace::Resource *res = c->resources; res; res = res->nextlive)
          {
            IgProfTrace::Value derived_size;
            derived_size = c->def->derivedLeakSize(res->hashslot->resource, res->size);
            if (derived_size)
              info.io.put(";LK=(").put((void *) res->hashslot->resource)
                     .put(",").put(derived_size)
                     .put(")");
          }
        }
        else
        {  // Resource size is the leak size
          for (IgProfTrace::Resource *res = c->resources; res; res = res->nextlive)
            info.io.put(";LK=(").put((void *) res->hashslot->resource)
            .put(",").put(res->size)
            .put(")");
        }
      }
    }
    info.io.put("\n");
  }

  info.depth++;
  for (frame = frame->children; frame; frame = frame->sibling)
    dumpOneProfile(info, frame);
  info.depth--;
}

/** Reset IDs used in dumping out profile data.  */
static void
dumpResetIDs(IgProfTrace::Stack *frame)
{
  IgProfTrace::Counter **ctr = &frame->counters[0];
  for (int i = 0; i < IgProfTrace::MAX_COUNTERS && *ctr; ++i, ++ctr)
    (*ctr)->def->id = -1;

  for (frame = frame->children; frame; frame = frame->sibling)
    dumpResetIDs(frame);
}

/** Utility function to dump out the profiler data from all current
    profile buffers: trace tree and live maps.  The strange calling
    convention is so this can be launched as a thread.  */
static void *
dumpAllProfiles(void *arg)
{
  // Use C locale when printing out, to avoid weird formatting of floating
  // point numbers and similar amenities.
  char *old_locale = setlocale(LC_ALL, "C");
  IgProfDumpInfo *info = (IgProfDumpInfo *) arg;
  IgProfTrace::PerfStat &perf = info->perf;
  itimerval stopped = { { 0, 0 }, { 0, 0 } };
  itimerval prof, virt, real;
  sigset_t  sigmask;

  if (info->blocksig)
  {
    setitimer(ITIMER_PROF, &stopped, &prof);
    setitimer(ITIMER_VIRTUAL, &stopped, &virt);
    setitimer(ITIMER_REAL, &stopped, &real);

    sigset_t everything;
    sigfillset(&everything);
    pthread_sigmask(SIG_BLOCK, &everything, &sigmask);
  }

  char outname[MAX_FNAME];
  const char *tofile = info->tofile;
  if (! tofile || ! tofile[0])
  {
    const char *progname = program_invocation_name;
    const char *slash = strrchr(progname, '/');
    if (slash && slash[1])
      progname = slash+1;
    else if (slash)
      progname = "unnamed";

    timeval tv;
    gettimeofday(&tv, 0);
    sprintf(outname, "|gzip -c>igprof.%.100s.%ld.%f.gz",
            progname, (long) getpid(), tv.tv_sec + 1e-6*tv.tv_usec);
    tofile = outname;
  }

  igprof_debug("dumping state to %s\n", tofile);
  info->output = (tofile[0] == '|'
                  ? (igprof_unsetenv("LD_PRELOAD"), popen(tofile+1, "w"))
                  : fopen(tofile, "w+"));
  if (! info->output)
    igprof_debug("can't write to output %s: %s (error %d)\n",
                 tofile, strerror(errno), errno);
  else
  {
    char clockres[32];
    size_t clockreslen = sprintf(clockres, "%f", s_clockres);
    size_t prognamelen = strlen(program_invocation_name);
    info->io.attach(fileno(info->output));
    info->io.put("P=(HEX ID=").put(getpid())
	    .put(" N=(").put(program_invocation_name, prognamelen)
	    .put(") T=").put(clockres, clockreslen)
	    .put(")\n");

    pthread_mutex_lock(&s_buflock);
    IgProfSymCache symcache;
    info->symcache = &symcache;
    std::set<IgProfTrace *> &bufs = allTraceBuffers();
    std::set<IgProfTrace *>::iterator i, e;
    for (i = bufs.begin(), e = bufs.end(); i != e; ++i)
    {
      IgProfTrace *buf = *i;
      buf->lock();
      dumpOneProfile(*info, buf->stackRoot());
      dumpResetIDs(buf->stackRoot());
      perf += buf->perfStats();
      buf->unlock();
    }

    s_masterbuf->lock();
    dumpOneProfile(*info, s_masterbuf->stackRoot());
    dumpResetIDs(s_masterbuf->stackRoot());
    perf += s_masterbuf->perfStats();
    s_masterbuf->unlock();

    info->io.flush();
    if (tofile[0] == '|')
      pclose(info->output);
    else
      fclose(info->output);
    pthread_mutex_unlock(&s_buflock);
  }

  if (info->blocksig)
  {
    setitimer(ITIMER_PROF, &prof, 0);
    setitimer(ITIMER_VIRTUAL, &virt, 0);
    setitimer(ITIMER_REAL, &real, 0);
    pthread_sigmask(SIG_SETMASK, &sigmask, 0);
  }

  double depthAvg = (1. * perf.sumDepth) / perf.ntraces;
  double ticksAvg = (1. * perf.sumTicks) / perf.ntraces;
  double tperdAvg = (1./16 * perf.sumTPerD) / perf.ntraces;
  igprof_debug("trace perf: ntraces=%.0f"
	       " depth=[av %.1f, rms %.1f]"
	       " ticks=[av %.1f, rms %.1f]"
	       " ticks-per-depth=[av %.1f, rms %.1f]\n",
               1. * perf.ntraces,
	       depthAvg, sqrt((1. * perf.sum2Depth) / perf.ntraces - depthAvg * depthAvg),
	       ticksAvg, sqrt((1. * perf.sum2Ticks) / perf.ntraces - ticksAvg * ticksAvg),
	       tperdAvg, sqrt((1./16/16 * perf.sum2TPerD) / perf.ntraces - tperdAvg * tperdAvg));
  setlocale(LC_ALL, old_locale);
  return 0;
}

/** Thread generating in-flight profile data dumps.  Handles both
    external asynchronous and in-program synchronous dump requests.

    The dumps are generated from this separate, non-profiled thread,
    so that we can guarantee we will never attempt to lock the profile
    pool lock recursively in the same thread.  */
static void *
asyncDumpThread(void *)
{
  int dodump = 0;
  struct stat st;
  while (true)
  {
    // If we are done processing, quit.  Give threads max ~1s to quit.
    if (s_quitting && ++s_quitting > 100)
      break;

    // Check every once in a while if a dump has been requested.
    if (! (++dodump % 32) && ! stat(s_dumpflag, &st))
    {
      unlink(s_dumpflag);
      IgProfDumpInfo info = { 0, 0, 0, 0, s_outname, 0, -1, 0, 1,
                              { 0, 0, 0, 0, 0, 0, 0 } };
      dumpAllProfiles(&info);
      dodump = 0;
    }

    // Have a nap.
    usleep(10000);
  }

  return 0;
}

extern "C" VISIBLE void
igprof_dump_now(const char *tofile)
{
  pthread_t tid;
  IgProfDumpInfo info = { 0, 0, 0, 0, tofile, 0, -1, 0, 1,
                          { 0, 0, 0, 0, 0, 0, 0 } };
  pthread_create(&tid, 0, &dumpAllProfiles, &info);
  pthread_join(tid, 0);
}

/** Dump out profile data when application is about to exit. */
static void
exitDump(void *)
{
  if (! s_igprof_activated) return;
  igprof_debug("final exit in thread 0x%lx, saving profile data\n",
               (unsigned long) pthread_self());

  // Deactivate.
  igprof_disable_globally();
  s_igprof_activated = false;
  s_igprof_enabled = 0;
  s_quitting = 1;
  itimerval stopped = { { 0, 0 }, { 0, 0 } };
  setitimer(ITIMER_PROF, &stopped, 0);
  setitimer(ITIMER_VIRTUAL, &stopped, 0);
  setitimer(ITIMER_REAL, &stopped, 0);

  // Dump all buffers.
  IgProfDumpInfo info = { 0, 0, 0, 0, s_outname, 0, -1, 0, 0,
                          { 0, 0, 0, 0, 0, 0, 0 } };
  dumpAllProfiles(&info);
  igprof_debug("igprof quitting\n");
  s_initialized = 0; // signal local data is unsafe to use
}

// -------------------------------------------------------------------
/** Initialise the profiler core itself.  Prepares the the program
    for profiling.  Captures various exit points so we generate a
    dump before the program goes "out".  Automatically triggered
    to run on library load.  All profiler modules should invoke
    this method before doing their own initialisation.

    Returns @c true if profiling is activated in this process.  */
bool
igprof_init(const char *id, void (*threadinit)(void), bool perthread, double clockres)
{
  // Refuse to initialise more than once.
  if (s_initialized)
  {
    fprintf(stderr, "IgProf: %s is already active, cannot also activate %s\n",
            s_initialized, id);
    _exit(1);
  }

  s_initialized = id;

  // The main binary may define some functions which we need. In
  // particular bourne shells tend to redefine getenv(), unsetenv(),
  // other binaries define a custom abort(). Get the real function
  // definitions from libc. (What we'd really want is -Bdirect?)`
  if (void *libc = dlopen("libc.so.6", RTLD_LAZY | RTLD_GLOBAL))
  {
    if (void *sym = dlsym(libc, "abort"))
      igprof_abort = __extension__ (IgProfAbortFunc *) sym;

    if (void *sym = dlsym(libc, "getenv"))
      igprof_getenv = __extension__ (char *(*)(const char *)) sym;

    if (void *sym = dlsym(libc, "unsetenv"))
      igprof_unsetenv = __extension__ (int (*)(const char *)) sym;

    dlclose(libc);
  }

  // Check if we should be activated at all.
  const char *target = igprof_getenv("IGPROF_TARGET");
  if (target && ! strstr(program_invocation_name, target))
  {
    igprof_debug("current process not selected for profiling:"
                 " process '%s' does not match '%s'\n",
                 program_invocation_name, target);
    return s_igprof_activated = false;
  }

  // Check profiling options.
  const char *options = igprof_options();
  if (! options || ! *options)
  {
    igprof_debug("$IGPROF not set, not profiling this process (%s)\n",
		 program_invocation_name);
    return s_igprof_activated = false;
  }

  for (const char *opts = options; *opts; )
  {
    while (*opts == ' ' || *opts == ',')
      ++opts;

    if (! strncmp(opts, "igprof:out='", 12))
    {
      int i = 0;
      opts += 12;
      while (i < MAX_FNAME-1 && *opts && *opts != '\'')
        s_outname[i++] = *opts++;
      s_outname[i] = 0;
    }
    else if (! strncmp(opts, "igprof:dump='", 13))
    {
      int i = 0;
      opts += 13;
      while (i < MAX_FNAME-1 && *opts && *opts != '\'')
        s_dumpflag[i++] = *opts++;
      s_dumpflag[i] = 0;
    }
    else
      opts++;

    while (*opts && *opts != ',' && *opts != ' ')
      opts++;
  }

  // Install exit handler to generate actual dump.
  abi::__cxa_atexit(&exitDump, 0, 0);

  // Create master buffer. If in per-thread mode, create another buffer
  // for profiling this thread; master buffer is then just for merging.
  // Otherwise, in global buffer mode, just use master buffer for all.
  s_masterbuf = new (s_masterbufdata) IgProfTrace;
  s_perthread = perthread;
  s_threadinit = threadinit;
  s_mainthread = pthread_self();
  s_tracebuf = makeTraceBuffer();

  // Report as activated.
  igprof_debug("profiler activated in %s, main thread id 0x%lx\n",
               program_invocation_name, s_mainthread);
  igprof_debug("profiler options: %s\n", options);

  // Report override function use.
  if (igprof_abort != &abort)
    igprof_debug("abort() from system %p, app had %p\n",
                 __extension__ (void *) igprof_abort,
		 __extension__ (void *) &abort);
  if (igprof_getenv != &getenv)
    igprof_debug("getenv() from system %p, app had %p\n",
                 __extension__ (void *) igprof_getenv,
		 __extension__ (void *) &getenv);
  if (igprof_unsetenv != &unsetenv)
    igprof_debug("unsetenv() from system %p, app had %p\n",
                 __extension__ (void *) igprof_unsetenv,
		 __extension__ (void *) &unsetenv);

  // Remember clock resolution.
  if (clockres > 0)
  {
    igprof_debug("timing resolution is %f s\n", clockres);
    s_clockres = clockres;
  }

  // Initialise per thread stuff.
  pthread_key_create(&s_igprof_flagkey, &freeThreadFlag);
  pthread_setspecific(s_igprof_flagkey, new IgProfAtomic(0));

  pthread_key_create(&s_igprof_bufkey, &freeTraceBuffer);
  pthread_setspecific(s_igprof_bufkey, s_tracebuf);

  // Start dump thread if we watch for a file.
  if (s_dumpflag[0])
    pthread_create(&s_dumpthread, 0, &asyncDumpThread, 0);

  // Hook into functions we care about.
  IgHook::hook(doexit_hook_main.raw);
  IgHook::hook(doexit_hook_main2.raw);
  IgHook::hook(dokill_hook_main.raw);
  IgHook::hook(dopthread_create_hook_main.raw);
#if __linux
  if (doexit_hook_main.raw.chain)  IgHook::hook(doexit_hook_libc.raw);
  if (doexit_hook_main2.raw.chain) IgHook::hook(doexit_hook_libc2.raw);
  if (dokill_hook_main.raw.chain)  IgHook::hook(dokill_hook_libc.raw);
// TODO: This should be done only for -pp mode
#if defined(__aarch64__)
  igprof_debug("IgProf will be disabled in code paths with vDSO symbols.\n");
  IgHook::Status s_gettimeofday = IgHook::hook(dogettimeofday_hook_main.raw);
  if (s_gettimeofday != IgHook::Success)
    igprof_debug("failed to hook 'gettimeofday' (status: %i). IgProf profiling might fail.\n", s_gettimeofday);
  IgHook::Status s_clock_gettime = IgHook::hook(doclock_gettime_hook_main.raw);
  if (s_clock_gettime != IgHook::Success)
    igprof_debug("failed to hook 'clock_gettime' (status: %i). IgProf profiling might fail.\n", s_clock_gettime);
  IgHook::Status s_clock_getres = IgHook::hook(doclock_getres_hook_main.raw);
  if (s_clock_getres != IgHook::Success)
    igprof_debug("failed to hook 'clock_getres' (status: %i). IgProf profiling might fail.\n", s_clock_getres);
  IgHook::Status s_sigreturn = IgHook::hook(dosigreturn_hook_main.raw);
  if (s_sigreturn != IgHook::Success)
    igprof_debug("failed to hook 'sigreturn' (status: %i). IgProf profiling might fail.\n", s_sigreturn);
#endif /* defined(__aarch64__) */
  IgHook::hook(dopthread_create_hook_pthread20.raw);
  IgHook::hook(dopthread_create_hook_pthread21.raw);
#endif

  // Activate.
  s_igprof_enabled = 1;
  s_igprof_activated = true;
  igprof_enable();
  return true;
}

// Trap glibc wrappers for vDSO symbols on AArch64 and disable
// IgProf profiling in such code paths. The vDSO on AArch64
// is handwritten in assembly and does not provide needed
// unwind information for libunwind.
// vDSO symbols:
//  __kernel_rt_sigreturn  (sigreturn)
//  __kernel_gettimeofday  (gettimeofday)
//  __kernel_clock_gettime (clock_gettime)
//  __kernel_clock_getres  (clock_getres)
#if defined(__aarch64__)
static int
dogettimeofday(IgHook::SafeData<igprof_dogettimeofday_t> &hook,
               struct timeval *tv, struct timezone *tz)
{
  igprof_disable();
  int ret = hook.chain(tv, tz);
  igprof_enable();

  return ret;
}

static int
doclock_gettime(IgHook::SafeData<igprof_doclock_gettime_t> &hook,
               clockid_t clk_id, struct timespec *tp)
{
  igprof_disable();
  int ret = hook.chain(clk_id, tp);
  igprof_enable();

  return ret;
}

static int
doclock_getres(IgHook::SafeData<igprof_doclock_getres_t> &hook,
               clockid_t clk_id, struct timespec *res)
{
  igprof_disable();
  int ret = hook.chain(clk_id, res);
  igprof_enable();

  return ret;
}

static int
dosigreturn(IgHook::SafeData<igprof_dosigreturn_t> &hook,
               unsigned long __unused)
{
  igprof_disable();
  int ret = hook.chain(__unused);
  igprof_enable();

  return ret;
}
#endif /* defined(__aarch64__) */

/** Get user-provided profiling options.  */
const char *
igprof_options(void)
{
  if (! s_options)
    s_options = igprof_getenv("IGPROF");
  return s_options;
}

/** Reset all current profile buffers. */
void
igprof_reset_profiles(void)
{
  pthread_mutex_lock(&s_buflock);
  std::set<IgProfTrace *>::iterator i, e;
  std::set<IgProfTrace *> &bufs = allTraceBuffers();
  for (i = bufs.begin(), e = bufs.end(); i != e; ++i)
  {
    IgProfTrace *buf = *i;
    buf->lock();
    buf->reset();
    buf->unlock();
  }

  s_masterbuf->lock();
  s_masterbuf->reset();
  s_masterbuf->unlock();
  pthread_mutex_unlock(&s_buflock);
}

/** Internal assertion helper routine.  */
int
igprof_panic(const char *file, int line, const char *func, const char *expr)
{
  igprof_disable_globally();

  fprintf(stderr, "%s: %s:%d: %s: assertion failure: %s\n",
	  program_invocation_name, file, line, func, expr);

  void *trace[128];
  int levels = IgHookTrace::stacktrace(trace, 128);
  for (int i = 2; i < levels; ++i)
  {
    const char  *sym = 0;
    const char  *lib = 0;
    long        offset = 0;
    long        liboffset = 0;

    IgHookTrace::symbol(trace[i], sym, lib, offset, liboffset);
    fprintf(stderr, "  %p %s %s %ld [%s %s %ld]\n",
            trace[i], sym ? sym : "?",
            (offset < 0 ? "-" : "+"), labs(offset), lib ? lib : "?",
            (liboffset < 0 ? "-" : "+"), labs(liboffset));
  }

  // igprof_abort();
  return 1;
}

/** Internal printf()-like debugging utility.  Produces output if
    $IGPROF_DEBUGGING environment variable is set to any value.  */
void
igprof_debug(const char *format, ...)
{
  static const char *debugging = igprof_getenv("IGPROF_DEBUGGING");
  char msgbuf[1024];
  char *msg = msgbuf;
  int left = sizeof(msgbuf);
  int out = 0;
  int len;

  if (debugging && s_igprof_stderrOpen)
  {
    timeval tv;
    gettimeofday(&tv, 0);
    len = snprintf(msg, left,
		   "*** IgProf(%lu, %.3f): ",
		   (unsigned long) getpid(),
		   tv.tv_sec + 1e-6*tv.tv_usec);
    ASSERT(len < left);
    left -= len;
    msg += len;
    out += len;

    va_list args;
    va_start(args, format);
    len = vsnprintf(msg, left, format, args);
    va_end(args);

    out += (len > left ? left : len);
    write(2, msgbuf, out);
  }
}

// -------------------------------------------------------------------
/** A wrapper for starting user threads to enable profiling.  */
static void *
threadWrapper(void *arg)
{
  // Get arguments.
  IgProfWrappedArg *wrapped = (IgProfWrappedArg *) arg;
  void *(*start_routine)(void*) = wrapped->start_routine;
  void *start_arg = wrapped->arg;
  delete wrapped;

  // Report the thread and enable per-thread profiling pools.
  if (s_igprof_activated)
  {
    __extension__
      igprof_debug("captured thread id 0x%lx for profiling (%p(%p))\n",
		   (unsigned long) pthread_self(),
		   (void *)(start_routine), start_arg);

    /* Setup thread for use in profiling. */
    pthread_setspecific(s_igprof_bufkey, makeTraceBuffer());
    pthread_setspecific(s_igprof_flagkey, new IgProfAtomic(1));
  }

  // Make sure we've called stack trace code at least once in
  // this thread before the profile signal hits.
  void *dummy = 0; IgHookTrace::stacktrace(&dummy, 1);

  // Run per-profiler initialisation.
  if (s_igprof_activated && s_threadinit)
    (*s_threadinit)();

  // Run the user thread.
  void *ret = (*start_routine)(start_arg);

  // Report thread exits. Profile result is harvested by key destructor.
  if (s_igprof_activated)
  {
    __extension__
      igprof_debug("leaving thread id 0x%lx from profiling (%p(%p))\n",
		   (unsigned long) pthread_self(),
		   (void *) start_routine, start_arg);

    itimerval stopped = { { 0, 0 }, { 0, 0 } };
    setitimer(ITIMER_PROF, &stopped, 0);
    setitimer(ITIMER_VIRTUAL, &stopped, 0);
    setitimer(ITIMER_REAL, &stopped, 0);
  }
  return ret;
}

/** Trap thread creation to run per-profiler initialisation.  */
static int
dopthread_create(IgHook::SafeData<igprof_dopthread_create_t> &hook,
                 pthread_t *thread,
                 const pthread_attr_t *attr,
                 void * (*start_routine)(void *),
                 void *arg)
{
  size_t stack = 0;
  if (attr && pthread_attr_getstacksize(attr, &stack) == 0 && stack < 64*1024)
  {
    igprof_debug("pthread_create increasing stack from %lu to 64kB\n",
		 (unsigned long) stack);
    pthread_attr_setstacksize((pthread_attr_t *) attr, 64*1024);
  }

  if (start_routine == dumpAllProfiles)
    return hook.chain(thread, attr, start_routine, arg);
  else
  {
    // Pass the actual arguments to our wrapper in a temporary memory
    // structure.  We need to hide the creation from memory profiler
    // in case it's running concurrently with this profiler.
    igprof_disable();
    IgProfWrappedArg *wrapped = new IgProfWrappedArg;
    wrapped->start_routine = start_routine;
    wrapped->arg = arg;
    igprof_enable();
    return hook.chain(thread, attr, &threadWrapper, wrapped);
  }
}

/** Trapped calls to exit() and _exit().  */
static void
doexit(IgHook::SafeData<igprof_doexit_t> &hook, int code)
{
  // Force the merge of per-thread profile tree into the main tree
  // if a thread calls exit().  Then forward the call.
  pthread_t thread = pthread_self();
  igprof_debug("%s(%d) called in thread 0x%lx\n",
               hook.function, code, (unsigned long) thread);
  hook.chain(code);
}

/** Trapped calls to kill().  Dump out profiler data if the signal
    looks dangerous.  Mostly really to trap calls to abort().  */
static int
dokill(IgHook::SafeData<igprof_dokill_t> &hook, pid_t pid, int sig)
{
  if ((pid == 0 || pid == getpid())
      && (sig == SIGHUP || sig == SIGINT || sig == SIGQUIT
          || sig == SIGILL || sig == SIGABRT || sig == SIGFPE
          || sig == SIGKILL || sig == SIGSEGV || sig == SIGPIPE
          || sig == SIGALRM || sig == SIGTERM || sig == SIGUSR1
          || sig == SIGUSR2 || sig == SIGBUS || sig == SIGIOT))
  {
    if (igprof_disable())
    {
      igprof_disable_globally();
      igprof_debug("kill(%d,%d) called, dumping state\n", (int) pid, sig);
      IgProfDumpInfo info = { 0, 0, 0, 0, s_outname, 0, -1, 0, 0,
                              { 0, 0, 0, 0, 0, 0, 0 } };
      dumpAllProfiles(&info);
      igprof_enable_globally();
    }
    igprof_enable();
  }
  return hook.chain(pid, sig);
}
