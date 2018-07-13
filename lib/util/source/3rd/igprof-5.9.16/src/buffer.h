#ifndef BUFFER_H
# define BUFFER_H

# include "macros.h"
# include "profile.h"
# include <stdlib.h>
# include <stdint.h>

/** Utility class for implementing private heap and hashed data structures.  */
class HIDDEN IgProfBuffer
{
protected:
  IgProfBuffer(void);
  ~IgProfBuffer(void);

protected:
  void initPool(void);
  void freePools(void);
  void unallocateRaw(void *p, size_t size);
  void *allocateRaw(size_t size);
  void *allocateSpace(size_t amount)
    {
      if (size_t(freeend_ - freestart_) < amount)
        allocatePool();

      ASSERT(size_t(freeend_ - freestart_) >= amount);
      void *p = freestart_;
      freestart_ += amount;
      return p;
    }
  template <class T> T *allocate(void)
    { return static_cast<T *>(allocateSpace(sizeof(T))); }

  static uint64_t hash(uintptr_t key, size_t shift)
    { return (key * 0x9e3779b97f4a7c16ULL) >> shift; }

private:
  void allocatePool(void);

  void                  **poolfirst_;            //< Pointer to first memory pool.
  void                  **poolcur_;              //< Pointer to current memory pool.
  char                  *freestart_;             //< Next free address.
  char                  *freeend_;               //< Last free address.

  // Unavailable copy constructor, assignment operator
  IgProfBuffer(IgProfBuffer &);
  IgProfBuffer &operator=(IgProfBuffer &);
};

#endif // BUFFER_H
