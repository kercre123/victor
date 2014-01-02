#ifndef _MV_STDDEF_H_
#define _MV_STDDEF_H_

#ifndef ASM
#ifndef __PTRDIFF_T__
  typedef int ptrdiff_t;
  #define __PTRDIFF_T__ 1
#endif

#ifndef __SIZE_T__
  typedef unsigned long size_t;
  #define __SIZE_T__ 1
#endif
#endif //ASM

#ifndef offsetof
  #define offsetof(type,member) ((size_t)&((type*)0)->member)
#endif

#ifndef NULL
  #define NULL 0
#endif

#endif //_MV_STDDEF_H_
