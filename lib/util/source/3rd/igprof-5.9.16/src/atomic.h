#ifndef ATOMIC_H
# define ATOMIC_H

# if __i386__ || __x86_64__ || __ppc__ || __arm__ || __aarch64__
# else
#  error Sorry this platform is not supported.
# endif

typedef int IgProfAtomic; // correct for all supported platforms for now

HIDDEN
inline IgProfAtomic
IgProfAtomicInc (volatile IgProfAtomic *val)
{
# if __i386__ || __x86_64__
  IgProfAtomic result;
  __asm__ __volatile__
    ("   lock; xaddl %0, (%1); incl %0;"
     : "=r" (result)
     : "r" (val), "0" (1)
     : "cc", "memory");
  return result;
# elif __ppc__
  IgProfAtomic result;
  __asm__ __volatile__
    ("   lwsync\n"
     "1: lwarx %0, 0, %1\n"
     "   addic %0, %0, 1\n"
     "   stwcx. %0, 0, %1\n"
     "   bne- 1b\n"
     "   isync\n"
     : "=&r" (result)
     : "r" (val)
     : "cc", "memory");
  return result;
# elif __arm__
  IgProfAtomic result, modified = 0;
  __asm__ __volatile__ ("1: ldrex %0,[%1]     \n"
                        "   add   %0,%0,#1    \n"
                        "   strex %2,%0,[%1]  \n"
                        "   cmp   %2,#0       \n"
                        "   bne   1b          \n"
                      : "=&r" (result)
                      : "r" (val), "r" (modified)
                      : "cc", "memory");
  return result;
# elif __aarch64__
  IgProfAtomic result, modified = 0;
  __asm__ __volatile__ ("1: ldaxr %0,[%1]     \n"
                        "   add   %0,%0,#1    \n"
                        "   stlxr %w2,%0,[%1] \n"
                        "   cbnz  %w2,1b      \n"
                      : "=&r" (result)
                      : "r" (val), "r" (modified)
                      : "cc", "memory");
  return result;
# endif
}

HIDDEN
inline IgProfAtomic
IgProfAtomicDec (volatile IgProfAtomic *val)
{
# if __i386__ || __x86_64__
  IgProfAtomic result;
  __asm__ __volatile__
    ("lock; xaddl %0, (%1); decl %0;"
     : "=r" (result)
     : "r" (val), "0" (-1)
     : "cc", "memory");
  return result;
# elif __ppc__
  IgProfAtomic result;
  __asm__ __volatile__
    ("   lwsync\n"
     "1: lwarx %0, 0, %1\n"
     "   addic %0, %0, -1\n"
     "   stwcx. %0, 0, %1\n"
     "   bne- 1b\n"
     "   isync\n"
     : "=&r" (result)
     : "r" (val)
     : "cc", "memory");
  return result;
# elif __arm__
  IgProfAtomic result, modified = 0;
  __asm__ __volatile__ ("1: ldrex %0,[%1]     \n"
                        "   add   %0,%0,#1    \n"
                        "   strex %2,%0,[%1]  \n"
                        "   cmp   %2,#0       \n"
                        "   bne   1b          \n"
                      : "=&r" (result)
                      : "r" (val), "r" (modified)
                      : "cc", "memory");
  return result;
# elif __aarch64__
  IgProfAtomic result, modified = 0;
  __asm__ __volatile__ ("1: ldaxr %0,[%1]     \n"
                        "   add   %0,%0,#-1   \n"
                        "   stlxr %w2,%0,[%1] \n"
                        "   cbnz  %w2,1b      \n"
                      : "=&r" (result)
                      : "r" (val), "r" (modified)
                      : "cc", "memory");
  return result;
# endif
}

#endif // ATOMIC_H
