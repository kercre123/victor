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
DUAL_HOOK(1, void, dofree, _main, _libc,
          (void *ptr), (ptr),
          "free", 0, igprof_getenv("IGPROF_MALLOC_LIB"))

// Checkerboard pattern (binary 10101010).
// Pages full of a checkerboard patter are considered untouched.
static const unsigned char      MAGIC_BYTE      = 0xAA;

HIDDEN unsigned char            s_zero_page[4096];
HIDDEN unsigned char            s_magic_page[4096];
static IgProfTrace::CounterDef  s_ct_empty      = { "MEM_LIVE", IgProfTrace::TICK, -1, 0 };
static bool                     s_init_memory   = false;
static bool                     s_track_unused  = false;
static bool                     s_initialized   = false;

/** Counts zero pages and checkerboard pages in a memory range */
static void
CountSpecialPages(IgProfTrace::Address address,
                  size_t size,
                  IgProfTrace::Value *num_zero_pages,
                  IgProfTrace::Value *num_magic_pages)
{
  ASSERT(num_zero_pages);
  ASSERT(num_magic_pages);

  *num_zero_pages = *num_magic_pages = 0;
  // Align the page iterator at next page boundary
  unsigned char *page =
    (unsigned char *)( (address+4095) & ~(size_t)4095 );
  unsigned char *scan_end = (unsigned char *)address + size;
  unsigned char *aligned_end = (unsigned char *)((size_t)scan_end & ~(size_t)4095);
  for (; page < aligned_end; page += 4096)
  {
    *num_zero_pages += !page[0] && !memcmp(page+1, s_zero_page, 4096-1);
    if (s_track_unused)
    {
      *num_magic_pages += (page[0] == MAGIC_BYTE) &&
                          !memcmp(page+1, s_magic_page, 4096-1);
    }
  }
}

static IgProfTrace::Value
derivedLeakSize(IgProfTrace::Address address, size_t size)
{
  IgProfTrace::Value num_zero_pages;
  IgProfTrace::Value num_magic_pages;

  CountSpecialPages(address, size, &num_zero_pages, &num_magic_pages);
  if (s_track_unused)
    return num_magic_pages*4096;
  return num_zero_pages*4096;
}

/** Record an allocation at @a ptr of @a size bytes.  Adds a pointer to
    current live memory map for live-zero checking and in order
    to match the free().  */
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

  RDTSC(tstart);
  depth = IgHookTrace::stacktrace(addresses, IgProfTrace::MAX_DEPTH);
  RDTSC(tend);

  // Drop top two stack frames (me, hook).
  buf->lock();
  frame = buf->push(addresses+2, depth-2);
  // Defer size estimation to free()
  ctr = buf->tick(frame, &s_ct_empty, 0, 1);
  buf->acquire(ctr, (IgProfTrace::Address) ptr, size);
  buf->traceperf(depth, tstart, tend);
  buf->unlock();
}

/** Remove knowledge about allocation.  Removes the memory allocation from the
    live map and adds the amount of memory in zero pages to the counter.
    Returns the size of the memory region */
static size_t
remove (void *ptr)
{
  if (UNLIKELY(! ptr))
    return 0;

  IgProfTrace *buf = igprof_buffer();
  if (UNLIKELY(! buf))
    return 0;

  size_t size = 0;
  buf->lock();
  IgProfTrace::HResource *hres = buf->findResource((IgProfTrace::Address) ptr);
  ASSERT(! hres || ! hres->record || hres->resource == (IgProfTrace::Address) ptr);
  if (UNLIKELY(hres && hres->record))
  {  // The free() call is likely to correspond to a small malloc()
    IgProfTrace::Resource *res = hres->record;
    IgProfTrace::Counter *ctr = res->counter;
    size = res->size;
    ASSERT(ctr);
    IgProfTrace::Value empty_mem = derivedLeakSize(res->hashslot->resource, size);
    buf->tick(ctr->frame, &s_ct_empty, empty_mem, 1);
    // buf->release() will decrease the counter value by res->size
    ctr->value += size;
    buf->release((IgProfTrace::Address) ptr);
  }
  buf->unlock();
  return size;
}

// -------------------------------------------------------------------
/** Initialise memory profiling.  Traps various system calls to keep track
    of memory usage, and if requested, leaks.  */
static void
initialize(void)
{
  if (s_initialized) return;
  s_initialized = true;

  memset(s_zero_page, 0, 4096);
  memset(s_magic_page, MAGIC_BYTE, 4096);
  s_ct_empty.derivedLeakSize = derivedLeakSize;

  const char    *options = igprof_options();
  bool          enable = false;

  while (options && *options)
  {
    while (*options == ' ' || *options == ',')
      ++options;

    if (! strncmp(options, "empty", 5))
    {
      enable = true;
      options += 5;
      while (*options)
      {
        if (! strncmp(options, ":initmem", 7))
        {
          s_init_memory = true;
          options += 7;
        }
        if (! strncmp(options, ":trackunused", 12))
        {
          s_track_unused = true;
          s_init_memory = true;
          options += 12;
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

  if (! igprof_init("empty memory profiler", 0, false))
    return;

  igprof_disable_globally();
  igprof_debug("empty memory profiler%s%s\n",
               s_init_memory ? ", initialize malloc'd memory with checkerboard" : "",
               s_track_unused ? ", tracking unused pages" : "tracking zero pages");

  IgHook::hook(domalloc_hook_main.raw);
  IgHook::hook(docalloc_hook_main.raw);
  IgHook::hook(dorealloc_hook_main.raw);
  IgHook::hook(dopmemalign_hook_main.raw);
  IgHook::hook(domemalign_hook_main.raw);
  IgHook::hook(dovalloc_hook_main.raw);
  IgHook::hook(dofree_hook_main.raw);
#if __linux
  if (domalloc_hook_main.raw.chain)    IgHook::hook(domalloc_hook_libc.raw);
  if (docalloc_hook_main.raw.chain)    IgHook::hook(docalloc_hook_libc.raw);
  if (domemalign_hook_main.raw.chain)  IgHook::hook(domemalign_hook_libc.raw);
  if (dovalloc_hook_main.raw.chain)    IgHook::hook(dovalloc_hook_libc.raw);
  if (dofree_hook_main.raw.chain)      IgHook::hook(dofree_hook_libc.raw);
#endif
  igprof_debug("empty memory profiler enabled\n");
  igprof_enable_globally();
}

// -------------------------------------------------------------------
// Traps for this profiler module.  Track memory allocation routines.
static void *
domalloc(IgHook::SafeData<igprof_domalloc_t> &hook, size_t n)
{
  bool enabled = igprof_disable();
  void *result = (*hook.chain)(n);

  if (LIKELY(enabled && result))
  {
    if (s_init_memory) memset(result, MAGIC_BYTE, n);
    add(result, n);
  }

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
  void *result = (*hook.chain)(ptr, n);

  if (LIKELY(result))
  {
    size_t old_size = 0;
    if (LIKELY(ptr))
      old_size = remove(ptr);
    if (LIKELY(enabled && result)) add(result, n);
    if (s_init_memory)
    {
      if (!ptr)
      {  // realloc is the same as malloc
        memset(result, MAGIC_BYTE, n);
      }
      else
      {
        if (old_size && (n > old_size))
          memset((unsigned char *)result + old_size, MAGIC_BYTE, n-old_size);
      }
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
  {
    if (s_init_memory) memset(result, MAGIC_BYTE, size);
    add(result, size);
  }

  igprof_enable();
  return result;
}

static void *
dovalloc(IgHook::SafeData<igprof_dovalloc_t> &hook, size_t size)
{
  bool enabled = igprof_disable();
  void *result = (*hook.chain)(size);

  if (LIKELY(enabled && result))
  {
    if (s_init_memory) memset(result, MAGIC_BYTE, size);
    add(result, size);
  }

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
  {
    if (s_init_memory) memset(*ptr, MAGIC_BYTE, size);
    add(*ptr, size);
  }

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

// -------------------------------------------------------------------
static bool autoboot __attribute__((used)) = (initialize(), true);
