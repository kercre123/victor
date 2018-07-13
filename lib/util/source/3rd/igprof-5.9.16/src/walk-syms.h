#ifndef WALK_SYMS_H
# define WALK_SYMS_H

# include "macros.h"

class HIDDEN IgHookTrace
{
public:
  static int          stacktrace(void **addresses, int nmax);
  static void *       tosymbol(void *address);
  static bool         symbol(void *address, const char *&sym,
			     const char *&lib, long &offset,
			     long &liboffset);
};

#endif // WALK_SYMS_H
