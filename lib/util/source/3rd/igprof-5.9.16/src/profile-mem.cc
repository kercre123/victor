#include "profile.h"
#include "profile-trace.h"
#include "hook.h"
#include "walk-syms.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <pthread.h>
#include <malloc.h>
#include <unistd.h>
#include <new>

// -------------------------------------------------------------------
// Traps for this profiler module
LIBHOOK(1, void *, donew, _tc,
        (size_t n), (n),
        "tc_new", 0, 0)
LIBHOOK(2, void *, donew_nothrow, _tc,
        (size_t n, const std::nothrow_t &nothrow), (n, nothrow),
        "tc_new_nothrow", 0, 0)
LIBHOOK(1, void *, donew, _tc_newarray,
        (size_t n), (n),
        "tc_newarray", 0, 0)
LIBHOOK(2, void *, donew_nothrow, _tc_newarray,
        (size_t n, const std::nothrow_t &nothrow), (n, nothrow),
        "tc_newarray_nothrow", 0, 0)
LIBHOOK(1, void *, domalloc, _tc,
        (size_t n), (n),
        "tc_malloc", 0, 0)
LIBHOOK(2, void *, docalloc, _tc,
        (size_t n, size_t m), (n, m),
        "tc_calloc", 0, 0)
LIBHOOK(2, void *, dorealloc, _tc,
        (void *ptr, size_t n), (ptr, n),
        "tc_realloc", 0, 0)
LIBHOOK(3, int, dopmemalign, _tc,
        (void **ptr, size_t alignment, size_t size),
        (ptr, alignment, size),
        "tc_posix_memalign", 0, 0)
LIBHOOK(2, void *, domemalign, _tc,
        (size_t alignment, size_t size), (alignment, size),
        "tc_memalign", 0, 0)
LIBHOOK(1, void *, dopvalloc, _tc,
        (size_t size), (size),
        "tc_pvalloc", 0, 0)
LIBHOOK(1, void *, dovalloc, _tc,
        (size_t size), (size),
        "tc_valloc", 0, 0)
LIBHOOK(1, void, dofree, _tc,
        (void *ptr), (ptr),
        "tc_free", 0, 0)
LIBHOOK(1, void, dofree, _tc_cfree,
        (void *ptr), (ptr),
        "tc_cfree", 0, 0)
LIBHOOK(1, void, dofree, _tc_delete,
        (void *ptr), (ptr),
        "tc_delete", 0, 0)
LIBHOOK(2, void, dofree_nothrow, _tc_delete,
        (void *ptr, const std::nothrow_t &nothrow), (ptr, nothrow),
        "tc_delete_nothrow", 0, 0)
LIBHOOK(1, void, dofree, _tc_deletearray,
        (void *ptr), (ptr),
        "tc_deletearray", 0, 0)
LIBHOOK(2, void, dofree_nothrow, _tc_deletearray,
        (void *ptr, const std::nothrow_t &nothrow), (ptr, nothrow),
        "tc_deletearray_nothrow", 0, 0)
LIBHOOK(2, void, dodelsize, _tc,
        (void *ptr, size_t n), (ptr, n),
        "tc_delete_sized", 0, 0)

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
DUAL_HOOK(1, void *, dopvalloc, _main, _libc,
          (size_t size), (size),
          "pvalloc", 0, igprof_getenv("IGPROF_MALLOC_LIB"))
DUAL_HOOK(1, void *, dovalloc, _main, _libc,
          (size_t size), (size),
          "valloc", 0, igprof_getenv("IGPROF_MALLOC_LIB"))
DUAL_HOOK(1, void, dofree, _main, _libc,
          (void *ptr), (ptr),
          "free", 0, igprof_getenv("IGPROF_MALLOC_LIB"))

// Data for this profiler module
static const int                OVERHEAD_NONE   = 0; // Memory use without malloc overheads
static const int                OVERHEAD_WITH   = 1; // Memory use including malloc overheads
static const int                OVERHEAD_DELTA  = 2; // Memory use malloc overhead only

static IgProfTrace::CounterDef  s_ct_total      = { "MEM_TOTAL",    IgProfTrace::TICK, -1, 0 };
static IgProfTrace::CounterDef  s_ct_largest    = { "MEM_MAX",      IgProfTrace::MAX, -1, 0 };
static IgProfTrace::CounterDef  s_ct_live       = { "MEM_LIVE",     IgProfTrace::TICK, -1, 0 };
static int                      s_overhead      = OVERHEAD_NONE;
static bool                     s_initialized   = false;
static size_t                   pagesize        = 0;

/** Record an allocation at @a ptr of @a size bytes.  Increments counters
    in the tree for the allocations as per current configuration and adds
    the pointer to current live memory map if we are tracking leaks.  */
static void  __attribute__((noinline))
add(void *ptr, size_t size)
{
  void *addresses[IgProfTrace::MAX_DEPTH];
  IgProfTrace *buf = igprof_buffer();
  IgProfTrace::Stack *frame;
  IgProfTrace::Counter *ctr;
  uint64_t tstart, tend;
  int depth;

  if (UNLIKELY(! buf))
    return;

  if (UNLIKELY(s_overhead != OVERHEAD_NONE))
  {
    size_t actual = malloc_usable_size(ptr);
    if (s_overhead == OVERHEAD_DELTA)
    {
      if ((size = actual - size) == 0)
        return;
    }
    else
      size = actual;
  }

  RDTSC(tstart);
  depth = IgHookTrace::stacktrace(addresses, IgProfTrace::MAX_DEPTH);
  RDTSC(tend);

  // Drop top two stack frames (me, hook).
  buf->lock();
  frame = buf->push(addresses+2, depth-2);
  buf->tick(frame, &s_ct_total, size, 1);
  buf->tick(frame, &s_ct_largest, size, 1);
  ctr = buf->tick(frame, &s_ct_live, size, 1);
  buf->acquire(ctr, (IgProfTrace::Address) ptr, size);
  buf->traceperf(depth, tstart, tend);
  buf->unlock();
}

/** Remove knowledge about allocation.  If we are tracking leaks,
    removes the memory allocation from the live map and subtracts
    from the live memory counters.  */
static void
remove (void *ptr)
{
  if (LIKELY(ptr))
  {
    IgProfTrace *buf = igprof_buffer();
    if (UNLIKELY(! buf))
      return;

    buf->lock();
    buf->release((IgProfTrace::Address) ptr);
    buf->unlock();
  }
}

// -------------------------------------------------------------------
/** Initialise memory profiling.  Traps various system calls to keep track
    of memory usage, and if requested, leaks.  */
static void
initialize(void)
{
  if (s_initialized) return;
  s_initialized = true;

  const char    *options = igprof_options();
  bool          enable = false;

  while (options && *options)
  {
    while (*options == ' ' || *options == ',')
      ++options;

    if (! strncmp(options, "mem", 3))
    {
      enable = true;
      options += 3;
      while (*options)
      {
        if (! strncmp(options, ":overhead=none", 14))
        {
          s_overhead = OVERHEAD_NONE;
          options += 14;
        }
        else if (! strncmp(options, ":overhead=include", 17))
        {
          s_overhead = OVERHEAD_WITH;
          options += 17;
        }
        else if (! strncmp(options, ":overhead=delta", 15))
        {
          s_overhead = OVERHEAD_DELTA;
          options += 15;
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

  if (! igprof_init("memory profiler", 0, false))
    return;

  igprof_disable_globally();
  igprof_debug("memory profiler: reporting %sallocation overhead%s\n",
               (s_overhead == OVERHEAD_NONE ? "memory use without "
                : s_overhead == OVERHEAD_WITH ? "memory use with " : ""),
               (s_overhead == OVERHEAD_DELTA ? " only" : ""));

  IgHook::hook(domalloc_hook_main.raw);
  IgHook::hook(docalloc_hook_main.raw);
  IgHook::hook(dorealloc_hook_main.raw);
  IgHook::hook(dopmemalign_hook_main.raw);
  IgHook::hook(domemalign_hook_main.raw);
  IgHook::hook(dopvalloc_hook_main.raw);
  IgHook::hook(dovalloc_hook_main.raw);
  IgHook::hook(dofree_hook_main.raw);

  IgHook::hook(donew_hook_tc.raw);
  IgHook::hook(donew_nothrow_hook_tc.raw);
  IgHook::hook(donew_hook_tc_newarray.raw);
  IgHook::hook(donew_nothrow_hook_tc_newarray.raw);
  IgHook::hook(domalloc_hook_tc.raw);
  IgHook::hook(docalloc_hook_tc.raw);
  IgHook::hook(dorealloc_hook_tc.raw);
  IgHook::hook(dopmemalign_hook_tc.raw);
  IgHook::hook(domemalign_hook_tc.raw);
  IgHook::hook(dopvalloc_hook_tc.raw);
  IgHook::hook(dovalloc_hook_tc.raw);
  IgHook::hook(dofree_hook_tc.raw);
  IgHook::hook(dofree_hook_tc_cfree.raw);
  IgHook::hook(dofree_hook_tc_delete.raw);
  IgHook::hook(dofree_nothrow_hook_tc_delete.raw);
  IgHook::hook(dofree_hook_tc_deletearray.raw);
  IgHook::hook(dofree_nothrow_hook_tc_deletearray.raw);
  IgHook::hook(dodelsize_hook_tc.raw);

#if __linux
  if (domalloc_hook_main.raw.chain)    IgHook::hook(domalloc_hook_libc.raw);
  if (docalloc_hook_main.raw.chain)    IgHook::hook(docalloc_hook_libc.raw);
  if (dorealloc_hook_main.raw.chain)   IgHook::hook(dorealloc_hook_libc.raw);
  if (dopmemalign_hook_main.raw.chain) IgHook::hook(dopmemalign_hook_libc.raw);
  if (domemalign_hook_main.raw.chain)  IgHook::hook(domemalign_hook_libc.raw);
  if (dopvalloc_hook_main.raw.chain)   IgHook::hook(dopvalloc_hook_libc.raw);
  if (dovalloc_hook_main.raw.chain)    IgHook::hook(dovalloc_hook_libc.raw);
  if (dofree_hook_main.raw.chain)      IgHook::hook(dofree_hook_libc.raw);
#endif

  igprof_debug("memory profiler enabled\n");
  igprof_enable_globally();
}

// -------------------------------------------------------------------
// Traps for this profiler module.  Track memory allocation routines.
static void *
donew(IgHook::SafeData<igprof_donew_t> &hook, size_t n)
{
  bool enabled = igprof_disable();
  void *result = (*hook.chain)(n);

  if (LIKELY(enabled && result))
    add(result, n);

  igprof_enable();
  return result;
}

static void *
donew_nothrow(IgHook::SafeData<igprof_donew_nothrow_t> &hook, size_t n,
              const std::nothrow_t &t)
{
  bool enabled = igprof_disable();
  void *result = (*hook.chain)(n, t);

  if (LIKELY(enabled && result))
    add(result, n);

  igprof_enable();
  return result;
}

static void *
domalloc(IgHook::SafeData<igprof_domalloc_t> &hook, size_t n)
{
  bool enabled = igprof_disable();
  void *result = (*hook.chain)(n);

  if (LIKELY(enabled && result))
    add(result, n);

  igprof_enable();
  return result;
}

static void *
docalloc(IgHook::SafeData<igprof_docalloc_t> &hook, size_t n, size_t m)
{
  bool enabled = igprof_disable();
  void *result = (*hook.chain)(n, m);

  if (LIKELY(enabled && result))
    add(result, n * m);

  igprof_enable();
  return result;
}

static void *
dorealloc(IgHook::SafeData<igprof_dorealloc_t> &hook, void *ptr, size_t n)
{
  bool enabled = igprof_disable();

  // Remove the allocation in profile data before calling realloc() to
  // avoid a race where another thread acquires the same memory address
  // and we end up thinking the allocation leaked. See below why this is
  // not entirely correct.
  if (LIKELY(enabled && ptr))
    remove(ptr);

  void *result = (*hook.chain)(ptr, n);

  if (LIKELY(enabled))
  {
    if (LIKELY(result))
      add(result, n);
    else if (LIKELY(ptr))
    {
      // This is incorrect; the original size of the allocation is lost.
      // This avoids us having to get feedback data from profile buffers
      // on remove() above, and realloc() should fail rarely. So use the
      // new size and live with the reporting inconsistency.
      add(ptr, n);
    }
  }

  igprof_enable();
  return result;
}

static void *
domemalign(IgHook::SafeData<igprof_domemalign_t> &hook, size_t alignment, size_t size)
{
  bool enabled = igprof_disable();
  void *result = (*hook.chain)(alignment, size);

  if (LIKELY(enabled && result))
    add(result, size);

  igprof_enable();
  return result;
}

static void *
dopvalloc(IgHook::SafeData<igprof_dopvalloc_t> &hook, size_t size)
{
  if (!pagesize)
    pagesize = getpagesize();

  if (!size)
    size = pagesize;

  size = (size + pagesize - 1) & ~(pagesize-1);

  bool enabled = igprof_disable();
  void *result = (*hook.chain)(size);

  if (LIKELY(enabled && result))
    add(result, size);

  igprof_enable();
  return result;
}

static void *
dovalloc(IgHook::SafeData<igprof_dovalloc_t> &hook, size_t size)
{
  bool enabled = igprof_disable();
  void *result = (*hook.chain)(size);

  if (LIKELY(enabled && result))
    add(result, size);

  igprof_enable();
  return result;
}

static int
dopmemalign(IgHook::SafeData<igprof_dopmemalign_t> &hook,
            void **ptr, size_t alignment, size_t size)
{
  bool enabled = igprof_disable();
  int result = (*hook.chain)(ptr, alignment, size);

  if (LIKELY(enabled && ptr && *ptr))
    add(*ptr, size);

  igprof_enable();
  return result;
}

static void
dofree(IgHook::SafeData<igprof_dofree_t> &hook, void *ptr)
{
  igprof_disable();
  remove(ptr);
  (*hook.chain)(ptr);
  igprof_enable();
}

static void
dofree_nothrow(IgHook::SafeData<igprof_dofree_nothrow_t> &hook, void *ptr,
               const std::nothrow_t &nothrow)
{
  igprof_disable();
  remove(ptr);
  (*hook.chain)(ptr, nothrow);
  igprof_enable();
}

static void
dodelsize(IgHook::SafeData<igprof_dodelsize_t> &hook, void *ptr, size_t n)
{
  igprof_disable();
  remove(ptr);
  (*hook.chain)(ptr, n);
  igprof_enable();
}

// -------------------------------------------------------------------
static bool autoboot __attribute__((used)) = (initialize(), true);
