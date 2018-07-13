#include "profile.h"
#include "profile-trace.h"
#include "hook.h"
#include "walk-syms.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <pthread.h>
#include <malloc.h>

// -------------------------------------------------------------------
// Traps for this profiler module
DUAL_HOOK(1, void *, domalloc, _main, _libc,
          (size_t n), (n),
          "malloc", 0, igprof_getenv("IGPROF_MALLOC_LIB"))
DUAL_HOOK(2, void *, docalloc, _main, _libc,
          (size_t n, size_t m), (n, m),
          "calloc", 0, igprof_getenv("IGPROF_MALLOC_LIB"))
DUAL_HOOK(2, void *, dorealloc, _main, _libc,
          (void *ptr, size_t n), (ptr, n),
          "realloc", 0, igprof_getenv("IGPROF_MALLOC_LIB"))
DUAL_HOOK(3, int, dopmemalign, _main, _libc,
          (void **ptr, size_t alignment, size_t size),
          (ptr, alignment, size),
          "posix_memalign", 0, igprof_getenv("IGPROF_MALLOC_LIB"))
DUAL_HOOK(2, void *, domemalign, _main, _libc,
          (size_t alignment, size_t size), (alignment, size),
          "memalign", 0, igprof_getenv("IGPROF_MALLOC_LIB"))
DUAL_HOOK(1, void *, dovalloc, _main, _libc,
          (size_t size), (size),
          "valloc", 0, igprof_getenv("IGPROF_MALLOC_LIB"))

static void *doothermain(void *a, void *b, void *c, void *d, void *e, void *f);
static IgHook::TypedData<void*(void *a, void *b, void *c, void *d, void *e, void *f)>
       doother_hook_main = { { 0, igprof_getenv("IGPROF_FP_FUNC"), 0, 0,
       &doothermain, 0, 0, 0 } };

static void *dootherlib(void *a, void *b, void *c, void *d, void *e, void *f);
static IgHook::TypedData<void*(void *a, void *b, void *c, void *d, void *e, void *f)>
    doother_hook_lib = { { 0, igprof_getenv("IGPROF_FP_FUNC"), 0, igprof_getenv("IGPROF_FP_LIB"),
      &dootherlib, 0, 0, 0 } };

static double dodoublemain(void *a, void *b, void *c, void *d, void *e, void *f);
static IgHook::TypedData<double(void *a, void *b, void *c, void *d, void *e, void *f)>
    dodouble_hook_main = { { 0, igprof_getenv("IGPROF_FP_FUNC"), 0, 0,
      &dodoublemain, 0, 0, 0 } };

static double dodoublelib(void *a, void *b, void *c, void *d, void *e, void *f);
static IgHook::TypedData<double(void *a, void *b, void *c, void *d, void *e, void *f)>
    dodouble_hook_lib = { { 0, igprof_getenv("IGPROF_FP_FUNC"), 0, igprof_getenv("IGPROF_FP_LIB"),
      &dodoublelib, 0, 0, 0 } };

static IgProfTrace::CounterDef  s_ct_total      = { "CALLS_TOTAL",    IgProfTrace::TICK, -1, 0 };
static bool                     s_initialized   = false;

/** Records calling a given function (only free for the moment). */
static void  __attribute__((noinline))
add(size_t ticks)
{
  void *addresses[IgProfTrace::MAX_DEPTH];
  IgProfTrace *buf = igprof_buffer();
  IgProfTrace::Stack *frame;
  uint64_t tstart, tend;
  int depth;

  if (UNLIKELY(! buf))
    return;

  RDTSC(tstart);
  depth = IgHookTrace::stacktrace(addresses, IgProfTrace::MAX_DEPTH);
  RDTSC(tend);

  // Drop top two stack frames (me, hook).
  buf->lock();
  frame = buf->push(addresses+2, depth-2);
  buf->tick(frame, &s_ct_total, ticks, 1);
  buf->traceperf(depth, tstart, tend);
  buf->unlock();
}

// -------------------------------------------------------------------
/** Initialise function profiling. For the moment only profiles calls to
    free and *alloc related symbols. */
static void
initialize(void)
{
  if (s_initialized) return;
  s_initialized = true;

  const char    *options = igprof_options();
  bool          enable = false;
  bool          trace_malloc = false;
  bool		trace_other = false;
  bool		trace_otherf = false;

  while (options && *options)
  {
    while (*options == ' ' || *options == ',')
      ++options;

    if (! strncmp(options, "func", 4))
    {
      options = options + 4;
      enable = true;
      while (*options)
      {
        if (! strncmp(options, ":name=malloc", 12))
        {
          trace_malloc=true;
          options += 12;
        }
        else if (! strncmp(options, ":name=otherf", 12))
	{
	  trace_otherf = true;
	  options += 12;
	}
	else if (! strncmp(options, ":name=other", 11))
	{
	  trace_other = true;
	  options += 11;
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

  if ((! trace_malloc)&& (! trace_other) && (! trace_otherf))
  {
    igprof_debug("function profiler: can only profile malloc and free for the time being\n");
    return;
  }

  if (! igprof_init("function profiler", 0, false))
    return;

  igprof_disable_globally();
  if (trace_other || trace_otherf || trace_malloc)
    igprof_debug("function profiler: profiling %s\n",
		igprof_getenv("IGPROF_FP_FUNC"));
  else
  {
    igprof_debug("function profiler: no function given, quitting");
    return;
  }

  if (trace_malloc)
  {
    IgHook::hook(domalloc_hook_main.raw);
    IgHook::hook(docalloc_hook_main.raw);
    IgHook::hook(dorealloc_hook_main.raw);
    IgHook::hook(dopmemalign_hook_main.raw);
    IgHook::hook(domemalign_hook_main.raw);
    IgHook::hook(dovalloc_hook_main.raw);
  }
  else if (trace_other)
    IgHook::hook(doother_hook_main.raw);
  else
    IgHook::hook(dodouble_hook_main.raw);

#if __linux
  if (trace_malloc)
  {
    if (domalloc_hook_main.raw.chain)    IgHook::hook(domalloc_hook_libc.raw);
    if (docalloc_hook_main.raw.chain)    IgHook::hook(docalloc_hook_libc.raw);
    if (domemalign_hook_main.raw.chain)  IgHook::hook(domemalign_hook_libc.raw);
    if (dovalloc_hook_main.raw.chain)    IgHook::hook(dovalloc_hook_libc.raw);
  }
  else if (trace_other)
  {
    if(doother_hook_main.raw.chain)	 IgHook::hook(doother_hook_lib.raw);
  }
  else
    if(dodouble_hook_main.raw.chain)	 IgHook::hook(dodouble_hook_lib.raw);

#endif
  igprof_enable_globally();
}

// -------------------------------------------------------------------
// Traps for this profiler module.  Track memory allocation routines.
static void *
domalloc(IgHook::SafeData<igprof_domalloc_t> &hook, size_t n)
{
  bool enabled = igprof_disable();
  uint64_t tstart, tend;

  RDTSC(tstart);
  void *result = (*hook.chain)(n);
  RDTSC(tend);

  if (LIKELY(enabled))
    add(tend - tstart);

  igprof_enable();
  return result;
}

static void *
docalloc(IgHook::SafeData<igprof_docalloc_t> &hook, size_t n, size_t m)
{
  bool enabled = igprof_disable();
  uint64_t tstart, tend;

  RDTSC(tstart);
  void *result = (*hook.chain)(n, m);
  RDTSC(tend);

  if (LIKELY(enabled))
    add(tend-tstart);

  igprof_enable();
  return result;
}

static void *
dorealloc(IgHook::SafeData<igprof_dorealloc_t> &hook, void *ptr, size_t n)
{
  bool enabled = igprof_disable();
  uint64_t tstart, tend;

  RDTSC(tstart);
  void *result = (*hook.chain)(ptr, n);
  RDTSC(tend);

  if (LIKELY(enabled))
    add(tend-tstart);

  igprof_enable();
  return result;
}

static void *
domemalign(IgHook::SafeData<igprof_domemalign_t> &hook, size_t alignment, size_t size)
{
  bool enabled = igprof_disable();
  uint64_t tstart, tend;

  RDTSC(tstart);
  void *result = (*hook.chain)(alignment, size);
  RDTSC(tend);

  if (LIKELY(enabled))
    add(tend-tstart);

  igprof_enable();
  return result;
}

static void *
dovalloc(IgHook::SafeData<igprof_dovalloc_t> &hook, size_t size)
{
  bool enabled = igprof_disable();
  uint64_t tstart, tend;

  RDTSC(tstart);
  void *result = (*hook.chain)(size);
  RDTSC(tend);

  if (LIKELY(enabled))
    add(tend-tstart);

  igprof_enable();
  return result;
}

static int
dopmemalign(IgHook::SafeData<igprof_dopmemalign_t> &hook,
            void **ptr, size_t alignment, size_t size)
{
  bool enabled = igprof_disable();
  uint64_t tstart, tend;

  RDTSC(tstart);
  int result = (*hook.chain)(ptr, alignment, size);
  RDTSC(tend);

  if (LIKELY(enabled))
    add(tend-tstart);

  igprof_enable();
  return result;
}

static void*
doothermain(void *a, void *b, void *c, void *d, void *e, void *f)
{
  bool enabled = igprof_disable();
  uint64_t tstart, tend;

  RDTSC(tstart);
  void *result = (*doother_hook_main.typed.chain)(a,b,c,d,e,f);
  RDTSC(tend);

  if (LIKELY(enabled))
    add(tend-tstart);

  igprof_enable();
  return result;
}
static void*
dootherlib(void *a, void *b, void *c, void *d, void *e, void *f)
{
  bool enabled = igprof_disable();
  uint64_t tstart, tend;

  RDTSC(tstart);
  void *result = (*doother_hook_lib.typed.chain)(a,b,c,d,e,f);
  RDTSC(tend);

  if (LIKELY(enabled))
    add(tend-tstart);

  igprof_enable();
  return result;
}

static double
dodoublemain(void *a, void *b, void *c, void *d, void *e, void *f)
{
  bool enabled = igprof_disable();
  uint64_t tstart, tend;

  RDTSC(tstart);
  double result = (*dodouble_hook_main.typed.chain)(a,b,c,d,e,f);
  RDTSC(tend);

  if(LIKELY(enabled))
    add(tend-tstart);

  igprof_enable();
  return result;
}

static double
dodoublelib(void *a, void *b, void *c, void *d, void *e, void *f)
{
  bool enabled = igprof_disable();
  uint64_t tstart, tend;

  RDTSC(tstart);
  double result = (*dodouble_hook_lib.typed.chain)(a,b,c,d,e,f);
  RDTSC(tend);

  if(LIKELY(enabled))
    add(tend-tstart);

  igprof_enable();
  return result;
}

// -------------------------------------------------------------------
static bool autoboot __attribute__((used)) = (initialize(), true);
