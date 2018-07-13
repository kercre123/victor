#include "profile-trace.h"
#include "walk-syms.h"
#include <stdio.h>

#if DEBUG
IgProfTrace::Counter IgProfTrace::FREED;
#endif

/** Initialise a trace buffer.  */
IgProfTrace::IgProfTrace(void)
  : hashLogSize_(20),
    hashUsed_(0),
    restable_(0),
    callcache_(0),
    resfree_(0),
    stack_(0)
{
  pthread_mutex_init(&mutex_, 0);

  // Allocate a separate slab of memory the resource hash table.  This
  // has to be big for large memory applications, so it's ok to
  // allocate it separately.  Note the memory obtained here starts out
  // as zeroed out.
  restable_ = (HResource *) allocateRaw((1u << hashLogSize_)*sizeof(HResource));

  // Allocate the call cache next.
  callcache_ = (StackCache *) allocateSpace(MAX_DEPTH*sizeof(StackCache));

  // Allocate the stack root node.
  stack_ = allocate<Stack>();

  // The resource free list starts out empty.
  ASSERT(! resfree_);

  // Initialise performance stats.
  perfStats_.ntraces   = 0;
  perfStats_.sumDepth  = 0;
  perfStats_.sum2Depth = 0;
  perfStats_.sumTicks  = 0;
  perfStats_.sum2Ticks = 0;
  perfStats_.sumTPerD  = 0;
  perfStats_.sum2TPerD = 0;
}

IgProfTrace::~IgProfTrace(void)
{
  if (restable_)
    unallocateRaw(restable_, (1u << hashLogSize_) * sizeof(HResource));
}

void
IgProfTrace::reset(void)
{
  // Free all pools, then create a new one.
  freePools();
  initPool();

  // Reset member variables back to initial values. Keep restable but reset it.
  memset(restable_, 0, (1u << hashLogSize_)*sizeof(HResource));
  callcache_ = (StackCache *) allocateSpace(MAX_DEPTH*sizeof(StackCache));
  stack_ = allocate<Stack>();
  hashUsed_ = 0;
  resfree_ = 0;

  perfStats_.ntraces   = 0;
  perfStats_.sumDepth  = 0;
  perfStats_.sum2Depth = 0;
  perfStats_.sumTicks  = 0;
  perfStats_.sum2Ticks = 0;
  perfStats_.sumTPerD  = 0;
  perfStats_.sum2TPerD = 0;
}

void
IgProfTrace::expandResourceHash(void)
{
  HResource *newTable;
  size_t i, j, slot;
  size_t oldSize = (1u << hashLogSize_);
  size_t newLogSize = hashLogSize_;
  size_t newSize;

TRY_AGAIN:
  newLogSize += 2;
  newSize = (1u << newLogSize);
  newTable = (HResource *) allocateRaw(newSize * sizeof(HResource));
  __extension__
    igprof_debug("expanding resource hash table for %p"
		 " from 2^%ju to 2^%ju, %ju used\n",
		 (void *) this, (uintmax_t) hashLogSize_,
		 (uintmax_t) newLogSize, (uintmax_t) hashUsed_);
  for (i = 0; i < oldSize; ++i)
  {
    if (! restable_[i].record)
      continue;

    slot = hash(restable_[i].resource, 8);
    for (j = 0; true; ++slot)
    {
      slot &= newSize-1;
      if (LIKELY(! newTable[slot].record))
      {
	newTable[slot] = restable_[i];
	restable_[i].record->hashslot = &newTable[slot];
	break;
      }

      if (UNLIKELY(++j == MAX_HASH_PROBES))
      {
	__extension__
	  igprof_debug("rehash of 0x%jx[%ju -> %ju] failed,"
		       " re-expanding another time\n",
		       (uintmax_t) restable_[i].resource,
		       (uintmax_t) i, (uintmax_t) slot);
        unallocateRaw(newTable, newSize * sizeof(HResource));
        goto TRY_AGAIN;
      }
    }
  }

  unallocateRaw(restable_, oldSize * sizeof(HResource));
  hashLogSize_ = newLogSize;
  restable_ = newTable;
}

void
IgProfTrace::mergeFrom(IgProfTrace &other)
{
  pthread_mutex_lock(&mutex_);
  pthread_mutex_lock(&other.mutex_);

  // Scan stack tree and insert each call stack, including resources.
  void *callstack[MAX_DEPTH+1];
  callstack[MAX_DEPTH] = stack_->address; // null really
  mergeFrom(0, other.stack_, &callstack[MAX_DEPTH]);
  perfStats_ += other.perfStats_;

  pthread_mutex_unlock(&other.mutex_);
  pthread_mutex_unlock(&mutex_);
}

void
IgProfTrace::mergeFrom(int depth, Stack *frame, void **callstack)
{
  // Process counters at this call stack level.
  Stack *myframe = push(callstack, depth);
  Counter **ptr = &frame->counters[0];
  for (int i = 0; i < MAX_COUNTERS && *ptr; ++i, ++ptr)
  {
    Counter *c = *ptr;
    if (c->ticks && ! c->resources)
      tick(myframe, c->def, c->value, c->ticks);
    else if (c->ticks)
      for (Resource *r = c->resources; r; r = r->nextlive)
      {
        Counter *ctr = tick(myframe, c->def, r->size, 1);
	acquire(ctr, r->hashslot->resource, r->size);
      }

    // Adjust the peak counter if necessary.
    if (c->def->type == TICK && c->peak > c->value)
      tick(myframe, c->def, c->peak - c->value, 0);
  }

  // Merge the children.
  for (frame = frame->children; frame; frame = frame->sibling)
  {
    ASSERT(depth < MAX_DEPTH);
    callstack[-1] = frame->address;
    mergeFrom(depth+1, frame, &callstack[-1]);
  }
}

#define INDENT(d) for (int i = 0; i < d; ++i) fputc(' ', stderr)

void
IgProfTrace::debugDumpStack(Stack *s, int depth)
{
  INDENT(2*depth);
  fprintf(stderr, "STACK %d frame=%p addr=%p next=%p kids=%p\n",
          depth, (void *)s, (void *)s->address,
          (void *)s->sibling, (void *)s->children);

  Counter **ptr = &s->counters[0];
  for (int i = 0; i < MAX_COUNTERS && *ptr; ++i, ++ptr)
  {
    Counter *c = *ptr;
    INDENT(2*depth+1);
    __extension__
      fprintf(stderr, "COUNTER ctr=%p %s %ju %ju %ju\n",
	      (void *)c, c->def->name, c->ticks, c->value, c->peak);

    for (Resource *r = c->resources; r; r = r->nextlive)
    {
      INDENT(2*depth+2);
      __extension__
	fprintf(stderr, "RESOURCE res=%p (prev=%p next=%p) %ju %ju\n",
		(void *)r, (void *)r->prevlive, (void *)r->nextlive,
		(uintmax_t)r->hashslot->resource, r->size);
    }
  }

  for (Stack *kid = s->children; kid; kid = kid->sibling)
    debugDumpStack(kid, depth+1);
}

void
IgProfTrace::debugDump(void)
{
  fprintf(stderr, "TRACE BUFFER %p:\n", (void *)this);
  fprintf(stderr, " RESTABLE:  %p\n", (void *)restable_);
  fprintf(stderr, " CALLCACHE: %p\n", (void *)callcache_);

  debugDumpStack(stack_, 0);
  // debugDumpResources();
}
