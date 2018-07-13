#include "buffer.h"
#include <sys/mman.h>
#include <memory.h>
#include <errno.h>

#if ! defined MAP_ANONYMOUS && defined MAP_ANON
# define MAP_ANONYMOUS MAP_ANON
#endif

static const unsigned int MEM_POOL_SIZE = 8*1024*1024;

/** Initialise a buffer.  */
IgProfBuffer::IgProfBuffer(void)
  : poolfirst_(0),
    poolcur_(0),
    freestart_(0),
    freeend_(0)
{
  initPool();
}

IgProfBuffer::~IgProfBuffer(void)
{
  freePools();
}

void
IgProfBuffer::initPool(void)
{
  // Get the first pool and initialise the chain pointer to null.
  void *pool = allocateRaw(MEM_POOL_SIZE);
  poolfirst_ = poolcur_ = (void **) pool;
  *poolfirst_ = 0;

  // Mark the rest free.
  freestart_ = (char *) pool + sizeof(void **);
  freeend_ = (char *) pool + MEM_POOL_SIZE;
}

void
IgProfBuffer::freePools(void)
{
  void **p = poolfirst_;
  while (p)
  {
    void **next = (void **) *p;
    munmap(p, MEM_POOL_SIZE);
    p = next;
  }
}

void
IgProfBuffer::unallocateRaw(void *p, size_t size)
{
  munmap(p, size);
}

void *
IgProfBuffer::allocateRaw(size_t size)
{
  void *data = mmap(0, size, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (data != MAP_FAILED)
    return data;
  else
  {
    igprof_debug("failed to allocate memory for profile buffer: %s (%d)\n",
                 strerror(errno), errno);
    igprof_abort();
  }
}

void
IgProfBuffer::allocatePool(void)
{
  // Get a pool.  If we don't have any pools yet, make this the first
  // one.  If we have an existing pool, chain the new one into the
  // last one.  Then make this the current and last pool and mark the
  // memory free.  Note the pool memory is allocated all-zeroes.
  void **pool = (void **) allocateRaw(MEM_POOL_SIZE);

  if (! poolfirst_)
    poolfirst_ = pool;

  if (poolcur_)
    *poolcur_ = pool;

  poolcur_ = pool;

  freestart_ = (char *) pool + sizeof(void **);
  freeend_ = (char *) pool + MEM_POOL_SIZE;
}
