#include "profile.h"
#include <cstdio>
#include <cstring>
#include "profile-trace.h"
#include "walk-syms.h"

static void do_enter();
static IgHook::TypedData<void()> do_enter_hook = { { 0, "__cyg_profile_func_enter",
       0, "libigprof.so" , &do_enter, 0, 0, 0 } };

static void do_exit();
static IgHook::TypedData<void()> do_exit_hook = { { 0, "__cyg_profile_func_exit",
       0, "libigprof.so", &do_exit, 0, 0, 0 } };

static bool s_initialized = false;
static IgProfTrace::CounterDef  s_ct_time      = { "CALL_TIME",    IgProfTrace::TICK, -1, 0 };
static IgProfTrace::CounterDef  s_ct_calls     = { "CALL_COUNT",   IgProfTrace::TICK, -1, 0 };
//enter time stack for functions in each treads
uint64_t igprof_times[IgProfTrace::MAX_DEPTH];
//enter counter
int callCount = 0;
//times spent in child functions
uint64_t child[IgProfTrace::MAX_DEPTH] = {0};

static void
initialize(void)
{
  if (s_initialized) return;
  s_initialized = true;

  const char* options = igprof_options();
  bool enable = false;


  while (options && *options)
  {
    while (*options == ' ' || *options == ',')
      ++options;
    if (! strncmp(options, "finst", 5))
    {
      options = options + 5;
      enable = true;
    }
    else
      options++;

    while (*options && *options != ',' && *options != ' ')
      options++;
  }
  if (! enable)
    return;

  if (! igprof_init("finstrument-profiler", 0, false))
    return;

  IgHook::hook(do_enter_hook.raw);
  IgHook::hook(do_exit_hook.raw);

  igprof_disable_globally();
  igprof_debug("gcc finstrument-profiler\n");
  igprof_debug("finstrument-profiler enabled\n");
  igprof_enable_globally();
}

// dummy functions which will be hooked
extern "C" void __cyg_profile_func_enter(void *func UNUSED, void *caller UNUSED)
{
#if __arm__
  __asm__ ("push {r1}\n"
           "pop {r1}\n");
#endif
}

extern "C" void __cyg_profile_func_exit(void *func UNUSED, void *caller UNUSED)
{
#if __arm__
  __asm__ ("push {r1}\n"
           "pop {r1}\n");
#endif
}

// save TSC value at before entering the real function
static void
do_enter ()
{
  uint64_t tstart;
  ++callCount;
  child[callCount-1] = 0;
  RDTSC(tstart);
  igprof_times[callCount-1] = -tstart;
}

//stop timer and tick counter
static void
do_exit ()
{
    uint64_t tstop,texit;
    RDTSC(tstop);
    --callCount;
    //diff = time spent in the function subtracted by time spent on childs
    uint64_t diff = igprof_times[callCount] - child[callCount] + tstop;
    void *addresses[IgProfTrace::MAX_DEPTH];
    IgProfTrace *buf = igprof_buffer();
    IgProfTrace::Stack *frame;
    int depth;

    if (UNLIKELY(! buf))
      return;

    depth = IgHookTrace::stacktrace(addresses, IgProfTrace::MAX_DEPTH);

    frame = buf->push(addresses+1, depth-1);
    buf->tick(frame, &s_ct_time, diff, 1);
    buf->tick(frame, &s_ct_calls, 1, 1);
    //if function is child, add time spent on child array for parent
    if (callCount > 0)
    {
      RDTSC(texit);
      child[callCount-1] += child[callCount] + diff + (texit - tstop);
    }
}

static bool autoboot __attribute__((used)) = (initialize(), true);
