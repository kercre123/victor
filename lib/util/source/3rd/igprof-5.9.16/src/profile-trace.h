#ifndef PROFILE_TRACE_H
# define PROFILE_TRACE_H

# include "macros.h"
# include "buffer.h"
# include "profile.h"
# if DEBUG
#  include "walk-syms.h"
# endif
# include <pthread.h>
# include <limits.h>
# include <stdint.h>
# include <string.h>

/** A resizeable profiler trace buffer.

    Each trace buffer tracks stack traces and their linked profiling
    counter values. The buffer also counts resources linked with the
    counters: tracking which call stack acquired which yet-unreleased
    resource and the size of the resource.

    The buffer owner is responsible for arranging correct handling of
    the buffer in presence of multiple threads. Buffers that do not
    track resources can be made thread-local and access need not be
    guarded. Resource-accounting buffers must be shared among all the
    threads able to acquire or release the resource -- the resource
    records are entered into the buffer in the order of the profile
    events, but there is no other concept of time and therefore it
    would not be possible to merge buffers from multiple threads into
    a canonical time order later on. In the rare situation the
    resources are thread-local the buffer can be thread-local too,
    otherwise the caller must ensure atomic access to the buffer.

    Technically a trace buffer consists of a header and a collection
    of fixed-size memory pools for the data and resource hash tables.
    The stack trace and resource counter information is carved out of
    the memory pools. The resource hash points to resource records.
    The trace buffer grows by allocating more memory pools as needed.

    The stack trace is represented as a tree of nodes keyed by call
    address. Each stack frame has a singly linked list of children,
    the addresses called from that stack frame. A frame also has
    pointers to profiling counters associated with that call tree.
    Each counter may point to a list of resources known to be live
    within that buffer; the resources linked with the counter form a
    singly linked list. The root of the stack trace is a null frame:
    one with zero call address.

    The resource hash table provides quick access to live resources.
    The hash table slots have the resource id and pointer to the
    above mentioned live resource record, which in turn points back
    to the hash slot, and the counter owning that resource. This is
    so that the counter can be found and decremented on freeing a
    resource. The hash table is linear open probed and grows when
    ever there is an insert conflict, typically when the hash is one
    half to two thirds full.

    The memory is allocated and the pool otherwise managed by using
    raw operating system pritimives: anonymous memory mappings. The
    buffer avoids calling any non-trivial library calls. The buffer
    implementation is safe for use in asynchronous signals provided
    the caller avoids re-enter the same buffer from nested
    signals.  */
class HIDDEN IgProfTrace : protected IgProfBuffer
{
public:
  struct PerfStat;
  struct StackCache;
  struct Stack;
  struct CounterDef;
  struct Counter;
  struct Resource;
  struct Record;

  /// Deepest supported stack depth.
  static const int MAX_DEPTH = 800;

  /// Maximum number of counters supported per stack frace.
  static const int MAX_COUNTERS = 3;

  /// Maximum number of hashs probe steps to look for a resource.
  static const size_t MAX_HASH_PROBES = 32;

  /// A value that might be an address, usually memory resource.
  typedef uintptr_t Address;

  /// A large-sized accumulated value for counters.
  typedef uintmax_t Value;

  /// Performance statistics for tracing.
  struct PerfStat
  {
    uint64_t    ntraces;        //< Number of traces.
    uint64_t    sumDepth;       //< sum(trace_depth).
    uint64_t    sum2Depth;      //< sum(trace_depth^2).
    uint64_t    sumTicks;       //< sum(ticks_for_trace).
    uint64_t    sum2Ticks;      //< sum(ticks_for_trace^2).
    uint64_t    sumTPerD;       //< sum((ticks << 4) / depth).
    uint64_t    sum2TPerD;      //< sum(((ticks << 4) / depth)^2).

    PerfStat &operator+=(const PerfStat &other);
  };

  /// Structure for call stack cache at the end.
  struct StackCache
  {
    void        *address;       //< Instruction pointer value.
    Stack       *frame;         //< Address of corresponding stack node.
  };

  /// Stack trace node.
  struct Stack
  {
    void        *address;       //< Instruction pointer value.
#if DEBUG
    Stack       *parent;        //< The calling stack frame, or null for the root.
#endif
    Stack       *sibling;       //< The next child frame of the same parent.
    Stack       *children;      //< The first child of this call frame.
    Counter     *counters[MAX_COUNTERS]; //< The counters for this call frame.
  };

  /// Counter type.
  enum CounterType
  {
    TICK,                       //< Ticked cumulative counter.
    MAX                         //< Maximum-value counter.
  };

  /// Counter definition.
  struct CounterDef
  {
    const char  *name;          //< The name of the counter, for output.
    CounterType type;           //< The behaviour type of the counter.
    int         id;             //< Reference ID for output.
    /* Allows for leak size computation derived from the memory content
       of a live resource.  If it is 0, the full resource size is added
       to the leak counter. */
    Value (*derivedLeakSize)(Address address, size_t size);
  };

  /// Counter value.
  struct Counter
  {
    CounterDef  *def;           //< The definition of this counter.
    Value       ticks;          //< The number of times the counter was increased.
    Value       value;          //< The accumulated counter value.
    Value       peak;           //< The maximum value of the counter at any time.
    Resource    *resources;     //< The live resources linked to this counter.
    Stack       *frame;         //< The stack node owning the counter.
  };

  /* Both the resource hash and a counter points to a live resource
     record. The hash pointer is a direct pointer, and the resource
     points back to the hash slot. The counter pointers are a singly
     linked list of resources attached to the counter ("leaks"). Each
     resource points back to the counter object. Updating a resource
     always updates both linked lists.

     When a resource is acquired, the resource hash table is searched
     for a previously existing record. If none exists, the resource
     is entered into the first free resource position, either taking
     one from the free list or allocating a new one. Otherwise the
     existing resource is freed as described below, and acquisition
     proceeds as if the resource wasn't known; it is assumed the
     profiler missed the release of the resource. The counter values
     are then updated.

     When a resource is released and known in the trace buffer, the
     record is removed from the counter, the hash slot is freed, and
     the record object is put on the free list (the "nextlive" chains
     the free list). If the resource is not known in the trace buffer
     the release is ignored on the assumption the profiler missed the
     resource acquisition, for example because it wasn't active at
     the time. */

  /// Resource entry for hash table.
  struct HResource
  {
    Address     resource;       //< Resource identity.
    Resource    *record;        //< Resource record.
  };

  /// Data for a resource.
  struct Resource
  {
    HResource   *hashslot;      //< Pointer to the hash slot of this resource.
    Resource    *prevlive;      //< Previous live resource in the same counter.
    Resource    *nextlive;      //< Next live resource in the same counter.
    Counter     *counter;       //< Counter tracking this resource.
    Value       size;           //< Size of the resource.
  };

  IgProfTrace(void);
  ~IgProfTrace(void);

  void			reset(void);
  void                  lock(void);
  Stack *               push(void **stack, int depth);
  Counter *             tick(Stack *frame, CounterDef *def, Value amount, Value ticks);
  void                  acquire(Counter *ctr, Address resource, Value size);
  void                  release(Address resource);
  HResource *           findResource(Address resource);
  void                  traceperf(int depth, uint64_t tstart, uint64_t tend);
  void                  mergeFrom(IgProfTrace &other);
  void                  unlock(void);

  Stack *               stackRoot(void) const;
  const PerfStat &      perfStats(void) const;

private:
  void                  expandResourceHash(void);
  Stack *               childStackNode(Stack *parent, void *address);
  void                  releaseResource(HResource *hres);
  void                  mergeFrom(int depth, Stack *frame, void **callstack);

  void                  debugDump(void);
  static void           debugDumpStack(Stack *s, int depth);

  pthread_mutex_t       mutex_;         //< Concurrency protection.
  size_t                hashLogSize_;   //< Log size of the resources hash.
  size_t                hashUsed_;      //< Occupancy in the resources hash.
  HResource             *restable_;     //< Start of the resources hash.
  StackCache            *callcache_;    //< Start of address cache.
  Resource              *resfree_;      //< Resource free list.
  Stack                 *stack_;        //< Stack root.
  PerfStat		perfStats_;	//< Performance stats.

#if DEBUG
  static Counter        FREED;		//< Pseudo-counter used to mark free list.
#endif

  // Unavailable copy constructor, assignment operator
  IgProfTrace(IgProfTrace &);
  IgProfTrace &operator=(IgProfTrace &);
};

/** Return the root stack frame.

    The root frame is a virtual frame with null address. All real
    call stacks are children of the root frame. Note that it is not
    guaranteed that all application stacks start with the same root,
    it is definitely possible there are any number of children under
    the virtual root. */
inline IgProfTrace::Stack *
IgProfTrace::stackRoot (void) const
{ return stack_; }

/** Combine trace performance stats. */
inline IgProfTrace::PerfStat &
IgProfTrace::PerfStat::operator+=(const PerfStat &other)
{
  ntraces   += other.ntraces;
  sumDepth  += other.sumDepth;
  sum2Depth += other.sum2Depth;
  sumTicks  += other.sumTicks;
  sum2Ticks += other.sum2Ticks;
  sumTPerD  += other.sumTPerD;
  sum2TPerD += other.sum2TPerD;
  return *this;
}

/** Accumulate stack trace performance statistics.

    Remember we captured stack trace of @a depth levels, and walking
    the stack took @a tend minus @a tstart clock cycles. */
inline void
IgProfTrace::traceperf(int depth, uint64_t tstart, uint64_t tend)
{
  // Remember number of traces, and sum and sum-squared of depth,
  // ticks, and ticks per stack depth level. These will be used to
  // compute average (sum(x)/n) and rms (sqrt(sum(x^2)/n - avg^2)).
  //
  // The ticks per depth is recorded in fixed point, shifted left
  // by four bits (multiplied by 16) to get crude decimal precision.
  //
  // We do _not_ use floating point math here on purpose. The
  // instrumented calls where this gets used do not always save
  // floating point register state across calls - for standard
  // library calls the compiler knows it cannot affect certain
  // registers. Messing with fp math in profiler tends to fire
  // sporadic math failures in the application.

  uint64_t dep         = depth;
  uint64_t nticks      = tend - tstart;
  uint64_t tperd       = (nticks << 4) / dep;

  perfStats_.ntraces++;
  perfStats_.sumDepth  += dep;
  perfStats_.sum2Depth += dep * dep;
  perfStats_.sumTicks  += nticks;
  perfStats_.sum2Ticks += nticks * nticks;
  perfStats_.sumTPerD  += tperd;
  perfStats_.sum2TPerD += tperd * tperd;
}

/** Return trace performance stats for this buffer. */
inline const IgProfTrace::PerfStat &
IgProfTrace::perfStats(void) const
{ return perfStats_; }

/** Lock the trace buffer. Call this before making state changes, or
    walking the buffer, unless you know for sure you are the only one
    accessing the buffer. */
inline void
IgProfTrace::lock(void)
{ pthread_mutex_lock(&mutex_); }

/** Unlock the trace buffer. Call this exactly as many times as you
    called lock(). */
inline void
IgProfTrace::unlock(void)
{ pthread_mutex_unlock(&mutex_); }

/** Locate a resource in the hash table.

    Returns pointer to the hash table slot for the resource, if the
    right slot can be found in at most MAX_HASH_PROBES steps.

    If the resource cannot be found, returns the first free hash slot
    which was seen in the scan.

    In other words, returns a null pointer if and only if the resource
    could not be found and there were no hash slots free. If the caller
    is going to insert data into the hash, it needs to expand the hash
    table and repeat the search until a free slot is found.

    If the function returns non-null pointer, the caller should compare
    hres->record to see if it was the one it looked for, or is a free
    slot. The record pointer will be null if the slot is free. */
inline IgProfTrace::HResource *
IgProfTrace::findResource(Address resource)
{
  HResource *hr;
  HResource *free = 0;
  size_t slot = hash(resource, 8);
  size_t size = (1u << hashLogSize_);
  for (size_t i = 0; i < MAX_HASH_PROBES; ++i, ++slot)
  {
    hr = &restable_[slot & (size-1)];
    if (hr->resource == resource)
      return hr;
    else if (! free && ! hr->record)
      free = hr;
  }

  return free;
}

/** Release the resource occupied by hash slot @a hres.

    Releases the hash slot, puts the resource record itself on the free
    list (&FREED counter) removing it from the chain of counter's live
    records, and decrements the value of the counter owning the resource.

    @param hres -- The resource to be freed. Must be non-null, point
    to a slot currently in use, which points to resource record currently
    in live record list of some counter in some stack frame. */
inline void
IgProfTrace::releaseResource(HResource *hres)
{
  ASSERT(hres);
  ASSERT(hres->resource);
  ASSERT(hashUsed_);

  Resource *res = hres->record;
  ASSERT(res);
  ASSERT(res->counter);
  ASSERT(res->counter != &FREED);
  ASSERT(res->counter->resources);

  // Deduct the resource from the counter.
  Counter *ctr = res->counter;
  ASSERT(ctr->value >= res->size);
  ASSERT(ctr->ticks > 0);
  ctr->value -= res->size;
  ctr->ticks--;

  // Unchain from hash and counter lists.
  hres->resource = 0;
  hres->record = 0;

  if (Resource *prev = res->prevlive)
  {
    ASSERT(prev->nextlive == res);
    prev->nextlive = res->nextlive;
  }
  else
  {
    ASSERT(ctr->resources == res);
    ctr->resources = res->nextlive;
  }

  if (Resource *next = res->nextlive)
  {
    ASSERT(next->prevlive == res);
    next->prevlive = res->prevlive;
  }

  // Put it on free list.
  memset(res, 0, sizeof(*res));
  res->nextlive = resfree_;
#if DEBUG
  res->counter = &FREED;
#endif
  resfree_ = res;
  --hashUsed_;
}

/** Find callee at @a address for the caller @a parent.

    Scan the singly linked list of child nodes, ordered by increasing
    address, and return pointer to the stack node. Creates the stack
    frame object in appropriate location if necessary.

    This code deliberately does things the "slow" way. It has only one
    caller, which already does caching of recently seen stack frames.
    Be careful about trying to optimise things here - there is a fair
    chance of simply making things slower by adding complexity. */
inline IgProfTrace::Stack *
IgProfTrace::childStackNode(Stack *parent, void *address)
{
  // Search for the child's call address in the child stack frames.
  Stack **kid = &parent->children;
  while (*kid)
  {
    Stack *k = *kid;
    if (k->address == address)
      return k;

    if ((char *) k->address > (char *) address)
      break;

    kid = &k->sibling;
  }

  // Didn't find it, add a new child in address-sorted order.
  Stack *next = *kid;
  Stack *k = *kid = allocate<Stack>();
  k->address = address;
#if DEBUG
  k->parent = parent;
#endif
  k->sibling = next;
  k->children = 0;
  for (int i = 0; i < MAX_COUNTERS; ++i)
    k->counters[i] = 0;
  return k;
}

/** Locate stack frame record for a call tree. */
inline IgProfTrace::Stack *
IgProfTrace::push(void **stack, int depth)
{
  // Make sure we operate on non-negative depth.  This allows callers
  // to do strip off call tree layers without checking for sufficient
  // depth themselves.
  if (depth < 0)
    depth = 0;

  // Look up call stack in the cache.
  StackCache    *cache = callcache_;
  Stack         *frame = stack_;

  for (int i = 0, valid = 1; i < depth && i < MAX_DEPTH; ++i)
  {
    void *address = stack[depth-i-1];
    // This is needed because apparently unw_backtrace fills the last entry of
    // the stack array with a 0 in case we are talking about the stacktrace of
    // a (non-main) thread, resulting in a subsequent crash in childStackNode.
    if (address == 0)
      break;
    else if (valid && cache[i].address == address)
      frame = cache[i].frame;
    else
    {
      // Look up this call stack child, then cache result.
      frame = childStackNode(frame, address);
      cache[i].address = address;
      cache[i].frame = frame;
      valid = 0;
    }
  }

  return frame;
}

/** Tick a counter @a def in stack @a frame by @a amount and @a ticks.
    Returns the pointer to the counter object in case the caller wants
    to also call acquire(). */
inline IgProfTrace::Counter *
IgProfTrace::tick(Stack *frame, CounterDef *def, Value amount, Value ticks)
{
  ASSERT(frame);
  ASSERT(def);

  // Locate and possibly initialise the counter.
  Counter *c = 0;
  Counter **ctr = &frame->counters[0];
  for (int i = 0; i < MAX_COUNTERS; ++i, ++ctr)
  {
    if (! *ctr)
    {
      *ctr = c = allocate<Counter>();
      c->def = def;
      c->ticks = 0;
      c->value = 0;
      c->peak = 0;
      c->resources = 0;
      c->frame = frame;
      break;
    }

    ASSERT((*ctr)->def);
    if ((*ctr)->def == def)
    {
      c = *ctr;
      break;
    }
  }

  ASSERT(c);

  // Tick the counter.
  if (def->type == TICK)
  {
    c->value += amount;
    if (c->value > c->peak)
      c->peak = c->value;
  }
  else if (def->type == MAX && c->value < amount)
    c->value = amount;

  c->ticks += ticks;

  // Return the counter for acquire() calls.
  return c;
}

/** Attach resource @a resource of @a size amount to counter @a ctr. */
inline void
IgProfTrace::acquire(Counter *ctr, Address resource, Value size)
{
  ASSERT(ctr);

  // Locate the resource in the hash table.
  HResource *hres = findResource(resource);
  ASSERT(! hres || ! hres->record || hres->resource == resource);

  // If it's already allocated, release the resource then
  // proceed as if we hadn't found it.
  if (UNLIKELY(hres && hres->record))
  {
    igprof_debug("new %s resource 0x%lx of %ju bytes was never freed in %p\n",
                 ctr->def->name, resource, hres->record->size, (void *)this);
#if DEBUG
    int depth = 0;
    for (Stack *s = ctr->frame; s; s = s->parent)
    {
      const char  *sym = 0;
      const char  *lib = 0;
      long        offset = 0;
      long        liboffset = 0;

      IgHookTrace::symbol(s->address, sym, lib, offset, liboffset);
      igprof_debug("  [%u] %10p %s + %d [%s + %d]\n", ++depth, s->address,
                   sym ? sym : "?", offset, lib ? lib : "?", liboffset);
    }
#endif

    // Release the resource, then proceed as if we hadn't found it.
    releaseResource(hres);
  }

  // Find a free hash table entry - may require hash resize.
  while (UNLIKELY(! hres))
  {
    expandResourceHash();
    hres = findResource(resource);
  }

  ASSERT(! hres->record);

  // Insert into the hash and lists.
  Resource *res = resfree_;
  if (LIKELY(res != 0))
    resfree_ = res->nextlive;
  else
    res = allocate<Resource>();
  hres->resource = resource;
  hres->record = res;
  res->hashslot = hres;
  res->prevlive = 0;
  res->nextlive = ctr->resources;
  res->counter = ctr;
  res->size = size;
  ctr->resources = res;
  if (res->nextlive)
    res->nextlive->prevlive = res;
  ++hashUsed_;
}

/** Release @a resource from which ever counter owns it. */
inline void
IgProfTrace::release(Address resource)
{
  // Locate the resource in the hash table.
  HResource *hres = findResource(resource);
  ASSERT(! hres || ! hres->record || hres->resource == resource);

  // If not found, we missed the allocation, ignore this release.
  if (LIKELY(hres && hres->record))
    releaseResource(hres);
}

#endif // PROFILE_TRACE_H
